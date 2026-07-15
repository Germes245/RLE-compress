#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "rle_compress_and_decompress_1_byte.h"

/* Проверка расширения .rle_1_byte */
static bool has_rle_extension(const char* filename) {
    const char* ext = ".rle_1_byte";
    size_t len = strlen(filename);
    size_t ext_len = strlen(ext);
    if (len < ext_len) return false;
    return strcmp(filename + len - ext_len, ext) == 0;
}

/* Вывод справки */
static void print_usage(const char* progname) {
    printf("Использование: %s [файлы...]\n", progname);
    printf("Сжимает файлы, добавляя расширение .rle_1_byte (потоковое сжатие)\n");
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
        if (access(output_filename, F_OK) == 0) {
            fprintf(stderr, "Предупреждение: файл '%s' уже существует, будет перезаписан.\n", output_filename);
        }

        // Открываем входной файл
        int input_fd = open(input_filename, O_RDONLY);
        if (input_fd < 0) {
            perror("Не удалось открыть входной файл");
            free(output_filename);
            exit_code = 1;
            continue;
        }

        // Открываем выходной файл
        int output_fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            perror("Не удалось создать выходной файл");
            close(input_fd);
            free(output_filename);
            exit_code = 1;
            continue;
        }

        int ret = 0;
        if (is_compressed) {
            ret = rle_decompress_stream(input_fd, output_fd);
        } else {
            ret = rle_compress_stream(input_fd, output_fd);
        }

        close(input_fd);
        close(output_fd);

        if (ret == 0) {
            printf("Обработан: %s -> %s\n", input_filename, output_filename);
        } else {
            fprintf(stderr, "Ошибка обработки файла '%s' (ошибка ввода-вывода или повреждённые данные).\n", input_filename);
            // При желании можно удалить неполный выходной файл:
            // unlink(output_filename);
            exit_code = 1;
        }

        free(output_filename);
    }

    return exit_code;
}