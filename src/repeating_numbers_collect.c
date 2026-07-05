#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

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
char* repeating_numbers_collect(char* src, size_t length_of_src, size_t* length_of_destination) {
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