#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "rle_compress_and_decompress_1_byte.h"
//SAF
/* Проверка расширения файла */
static bool has_rle_extension(const char* filename) {
    const char* ext = ".rle_1_byte";
    size_t len = strlen(filename);
    size_t ext_len = strlen(ext);
    if (len < ext_len) return false;
    return strcmp(filename + len - ext_len, ext) == 0;
}

/* Чтение файла в память (бинарный режим) */
static unsigned char* read_file(const char* filename, size_t* out_len) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("Ошибка открытия файла");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        perror("Ошибка определения размера файла");
        fclose(f);
        return NULL;
    }

    unsigned char* buffer = (unsigned char*)malloc(fsize);
    if (!buffer) {
        perror("Ошибка выделения памяти");
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, fsize, f);
    if (read_bytes != (size_t)fsize) {
        perror("Ошибка чтения файла");
        free(buffer);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *out_len = (size_t)fsize;
    return buffer;
}

/* Запись буфера в файл (бинарный режим) */
static bool write_file(const char* filename, const unsigned char* data, size_t len) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Ошибка создания файла");
        return false;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        perror("Ошибка записи файла");
        return false;
    }
    return true;
}

/* Вывод справки */
static void print_usage(const char* progname) {
    printf("Использование: %s [файлы...]\n", progname);
    printf("Сжимает файлы, добавляя расширение .rle_1_byte\n");
    printf("Распаковывает файлы с расширением .rle_1_byte, удаляя его.\n");
    printf("Пример:\n");
    printf("  %s документ.txt картинка.bin архив.rle_1_byte\n", progname);
    printf("  -> создаст документ.txt.rle_1_byte, картинка.bin.rle_1_byte и распакует архив в файл архив\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    int exit_code = 0;

    for (int i = 1; i < argc; ++i) {
        const char* input_filename = argv[i];
        bool is_compressed = has_rle_extension(input_filename);

        // Определяем имя выходного файла
        char* output_filename = NULL;
        if (is_compressed) {
            // Удаляем расширение .rle_1_byte
            size_t len = strlen(input_filename);
            size_t ext_len = strlen(".rle_1_byte");
            output_filename = (char*)malloc(len - ext_len + 1);
            if (!output_filename) {
                perror("Ошибка памяти");
                exit_code = 1;
                continue;
            }
            strncpy(output_filename, input_filename, len - ext_len);
            output_filename[len - ext_len] = '\0';
        } else {
            // Добавляем расширение
            output_filename = (char*)malloc(strlen(input_filename) + strlen(".rle_1_byte") + 1);
            if (!output_filename) {
                perror("Ошибка памяти");
                exit_code = 1;
                continue;
            }
            sprintf(output_filename, "%s.rle_1_byte", input_filename);
        }

        // Проверяем, существует ли выходной файл
        struct stat st;
        if (stat(output_filename, &st) == 0) {
            fprintf(stderr, "Предупреждение: файл '%s' уже существует, будет перезаписан.\n", output_filename);
        }

        // Чтение входного файла
        size_t in_len;
        unsigned char* in_data = read_file(input_filename, &in_len);
        if (!in_data) {
            free(output_filename);
            exit_code = 1;
            continue;
        }

        // Обработка
        unsigned char* out_data = NULL;
        size_t out_len = 0;
        bool success = false;

        if (is_compressed) {
            // Распаковка
            char* decompressed = rle_decompress_1_byte((char*)in_data, in_len, &out_len);
            if (decompressed) {
                out_data = (unsigned char*)decompressed;
                success = true;
            } else {
                fprintf(stderr, "Ошибка распаковки файла '%s' (данные повреждены или неверный формат).\n", input_filename);
            }
        } else {
            // Сжатие
            char* compressed = rle_compress_1_byte((char*)in_data, in_len, &out_len);
            if (compressed) {
                out_data = (unsigned char*)compressed;
                success = true;
            } else {
                fprintf(stderr, "Ошибка сжатия файла '%s'.\n", input_filename);
            }
        }

        if (success) {
            if (write_file(output_filename, out_data, out_len)) {
                printf("Обработан: %s -> %s\n", input_filename, output_filename);
            } else {
                exit_code = 1;
            }
            free(out_data);
        } else {
            exit_code = 1;
        }

        free(in_data);
        free(output_filename);
    }

    return exit_code;
}