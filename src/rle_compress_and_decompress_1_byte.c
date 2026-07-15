#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

/**
 * Реализация RLE-сжатия для массива байт (uint8_t).
 * 
 * @param src                указатель на исходный массив (интерпретируется как uint8_t)
 * @param length_of_src      длина исходного массива
 * @param length_of_destination указатель на переменную, куда будет записана длина сжатого массива
 * @return                   указатель на выделенный буфер со сжатыми данными,
 *                           или NULL в случае ошибки (память не выделена,
 *                           или входные данные некорректны).
 * 
 * Примечание: сжатый массив представляет собой последовательность пар
 * (счётчик, значение). Счётчик – это число повторений (1…255), значение –
 * сам байт. Если серия одинаковых байт длиннее 255, она разбивается на
 * несколько блоков по 255.
 */
char* rle_compress_1_byte(char* src, size_t length_of_src, size_t* length_of_destination) {
    /* Проверка корректности входных параметров */
    if (src == NULL || length_of_src == 0 || length_of_destination == NULL) {
        if (length_of_destination) *length_of_destination = 0;
        return NULL;
    }

    /* Первый проход: подсчёт количества пар (блоков) */
    size_t pairs = 0;
    size_t i = 0;
    while (i < length_of_src) {
        unsigned char val = (unsigned char)src[i];
        size_t run_len = 0;
        /* Подсчёт длины текущей серии одинаковых байт */
        while (i < length_of_src && (unsigned char)src[i] == val) {
            ++run_len;
            ++i;
        }
        /* Разбиваем серию на блоки по 255, каждый блок даёт одну пару */
        pairs += (run_len + 254) / 255;  // ceil(run_len / 255)
    }

    /* Длина сжатого массива в байтах */
    size_t dest_len = pairs * 2;
    *length_of_destination = dest_len;

    /* Выделение памяти под результат */
    char* dest = (char*)malloc(dest_len * sizeof(char));
    if (dest == NULL) {
        *length_of_destination = 0;
        return NULL;
    }

    /* Второй проход: заполнение сжатого массива */
    i = 0;
    size_t dest_idx = 0;
    while (i < length_of_src) {
        unsigned char val = (unsigned char)src[i];
        size_t run_len = 0;
        while (i < length_of_src && (unsigned char)src[i] == val) {
            ++run_len;
            ++i;
        }

        /* Запись блоков по 255 (или меньше для последнего) */
        while (run_len > 0) {
            size_t chunk = (run_len > 255) ? 255 : run_len;
            dest[dest_idx++] = (char)chunk;     /* счётчик */
            dest[dest_idx++] = (char)val;       /* значение */
            run_len -= chunk;
        }
    }

    /* Проверка, что записали ровно столько, сколько планировали */
    /* (можно убрать в релизной версии) */
    /* if (dest_idx != dest_len) { ... } */

    return dest;
}

/**
 * @brief Распаковывает массив, сжатый RLE (парой счётчик-значение).
 *
 * @param src                   указатель на сжатые данные (последовательность пар)
 * @param length_of_src         длина сжатых данных в байтах (должна быть чётной)
 * @param length_of_destination указатель на переменную для записи длины распакованных данных
 * @return                      указатель на выделенный буфер с распакованными данными,
 *                              или NULL при ошибке (некорректные параметры,
 *                              повреждённые данные, недостаток памяти).
 *                              Вызывающий должен освободить память через free().
 */
char* rle_decompress_1_byte(char* src, size_t length_of_src, size_t* length_of_destination) {
    /* Проверка входных параметров */
    if (src == NULL || length_of_src == 0 || length_of_destination == NULL) {
        if (length_of_destination) *length_of_destination = 0;
        return NULL;
    }

    /* Сжатые данные должны состоять из целого числа пар (счётчик, значение) */
    if (length_of_src % 2 != 0) {
        *length_of_destination = 0;
        return NULL;
    }

    /* Первый проход: вычисление общего размера распакованных данных */
    size_t total_len = 0;
    for (size_t i = 0; i < length_of_src; i += 2) {
        unsigned char count = (unsigned char)src[i];
        /* Счётчик должен быть в диапазоне 1..255 (по условию) */
        if (count == 0) {
            *length_of_destination = 0;
            return NULL;  /* Некорректные данные: нулевой счётчик */
        }
        /* Проверка переполнения при суммировании */
        if (total_len > SIZE_MAX - count) {
            *length_of_destination = 0;
            return NULL;
        }
        total_len += count;
    }

    *length_of_destination = total_len;

    /* Выделение памяти под результат */
    char* dest = (char*)malloc(total_len * sizeof(char));
    if (dest == NULL) {
        *length_of_destination = 0;
        return NULL;
    }

    /* Второй проход: заполнение выходного буфера */
    size_t dest_idx = 0;
    for (size_t i = 0; i < length_of_src; i += 2) {
        unsigned char count = (unsigned char)src[i];
        unsigned char value = (unsigned char)src[i + 1];
        for (size_t j = 0; j < count; ++j) {
            dest[dest_idx++] = (char)value;
        }
    }

    /* Проверка, что записали ровно столько, сколько планировали (можно убрать) */
    /* if (dest_idx != total_len) { ... } */

    return dest;
}

/**
 * @brief Выполняет RLE-сжатие потока байтов из дескриптора в дескриптор.
 *
 * Читает данные из input_fd блоками по 4096 байт, обнаруживает серии
 * одинаковых значений и записывает в output_fd пары (счётчик, значение).
 * Длинные серии (>255) автоматически разбиваются на блоки по 255.
 * Сжатие происходит «на лету», без загрузки всего файла в память.
 *
 * @param input_fd  дескриптор для чтения (POSIX, открыт на чтение)
 * @param output_fd дескриптор для записи (POSIX, открыт на запись)
 * @return 0 при успешном завершении, -1 в случае ошибки чтения или записи.
 *         При возникновении ошибки процесс сжатия прерывается.
 *
 * @note Если read() возвращает 0 (конец файла), функция завершается успешно.
 *       Если read() возвращает -1 или write() не может записать все байты
 *       (записано меньше, чем ожидалось, или возвращено -1), функция
 *       немедленно возвращает -1.
 */
int rle_compress_stream(int input_fd, int output_fd) {
    unsigned char buffer[4096];
    ssize_t n;
    unsigned char current_byte = 0;
    unsigned int current_count = 0;   // счётчик текущей серии (1..255)

    while ((n = read(input_fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < n; ++i) {
            unsigned char b = buffer[i];

            if (current_count == 0) {
                // начинаем новую серию
                current_byte = b;
                current_count = 1;
            } else if (b == current_byte && current_count < 255) {
                // продолжаем текущую серию
                ++current_count;
            } else {
                // серия закончилась или достигнут лимит 255 — записываем пару
                unsigned char out[2] = { (unsigned char)current_count, current_byte };
                ssize_t written = 0;
                while (written < 2) {
                    ssize_t w = write(output_fd, out + written, 2 - written);
                    if (w <= 0) return -1;   // ошибка записи
                    written += w;
                }
                // начинаем новую серию с текущего байта
                current_byte = b;
                current_count = 1;
            }
        }
    }

    // если read() вернул -1, возвращаем ошибку
    if (n < 0) return -1;

    // записываем последнюю серию, если она не пуста
    if (current_count > 0) {
        unsigned char out[2] = { (unsigned char)current_count, current_byte };
        ssize_t written = 0;
        while (written < 2) {
            ssize_t w = write(output_fd, out + written, 2 - written);
            if (w <= 0) return -1;
            written += w;
        }
    }

    return 0;
}

/**
 * @brief Распаковывает RLE-сжатый поток из дескриптора в дескриптор.
 *
 * Читает из input_fd пары (счётчик, значение), где счётчик (1..255)
 * указывает, сколько раз нужно записать значение в выходной поток.
 * Распаковка происходит «на лету», без загрузки всего файла в память.
 * Используются внутренние буферы для эффективного ввода-вывода.
 *
 * @param input_fd  дескриптор для чтения сжатых данных (POSIX, открыт на чтение)
 * @param output_fd дескриптор для записи распакованных данных (POSIX, открыт на запись)
 * @return 0 при успешном завершении, -1 в случае ошибки ввода-вывода.
 *         Если прочитан счётчик = 0, это считается ошибкой (некорректный формат)
 *         и функция возвращает -1. Также возвращает -1 при преждевременном
 *         окончании потока (неполная пара).
 */
int rle_decompress_stream(int input_fd, int output_fd) {
    unsigned char read_buf[4096];   // буфер для чтения
    unsigned char write_buf[4096];  // буфер для записи
    size_t read_pos = 0;            // текущая позиция в read_buf
    size_t read_count = 0;          // количество байт в read_buf
    size_t write_pos = 0;           // текущая позиция в write_buf

    // Вспомогательная функция: получить следующий байт из входного потока
    // возвращает байт или -1 при ошибке/конце файла
    int next_byte() {
        if (read_pos < read_count) {
            // ещё есть данные в буфере
            return read_buf[read_pos++];
        }
        // буфер пуст – читаем новую порцию
        ssize_t n = read(input_fd, read_buf, sizeof(read_buf));
        if (n < 0) return -1;        // ошибка чтения
        if (n == 0) return -2;       // конец файла (недостаточно данных)
        read_count = (size_t)n;
        read_pos = 1;                // первый байт новой порции
        return read_buf[0];
    }

    // Вспомогательная функция: записать буфер в output_fd
    int flush_write() {
        if (write_pos == 0) return 0;
        ssize_t written = 0;
        while ((size_t)written < write_pos) {
            ssize_t n = write(output_fd, write_buf + written, write_pos - written);
            if (n <= 0) return -1;    // ошибка записи
            written += n;
        }
        write_pos = 0;
        return 0;
    }

    // Основной цикл распаковки
    while (1) {
        // Читаем счётчик
        int cnt = next_byte();
        if (cnt == -1) return -1;      // ошибка чтения
        if (cnt == -2) break;          // конец файла – нормально, выходим
        if (cnt == 0) return -1;       // невалидный счётчик

        // Читаем значение
        int val = next_byte();
        if (val < 0) return -1;        // ошибка чтения или неполная пара

        // Записываем значение cnt раз в выходной буфер
        while (cnt > 0) {
            if (write_pos >= sizeof(write_buf)) {
                // буфер полон – сбрасываем
                if (flush_write() != 0) return -1;
            }
            write_buf[write_pos++] = (unsigned char)val;
            --cnt;
        }
    }

    // Сбрасываем остаток выходного буфера
    if (flush_write() != 0) return -1;

    return 0;
}