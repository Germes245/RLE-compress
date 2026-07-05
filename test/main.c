#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rle_compress_and_decompress_1_byte.h"

/* Вспомогательная функция для сравнения массивов */
static int arrays_equal(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

/* Печать массива для отладки */
static void print_array(const char* arr, size_t len, const char* name) {
    printf("%s [", name);
    for (size_t i = 0; i < len; ++i) {
        printf("%d", (unsigned char)arr[i]);
        if (i < len - 1) printf(", ");
    }
    printf("]\n");
}

/* Проверка: сжать -> распаковать -> сравнить с исходным */
static void test_compress_decompress(const char* original, size_t orig_len) {
    size_t compressed_len;
    char* compressed = rle_compress_1_byte((char*)original, orig_len, &compressed_len);
    assert(compressed != NULL);

    size_t decompressed_len;
    char* decompressed = rle_decompress_1_byte(compressed, compressed_len, &decompressed_len);
    assert(decompressed != NULL);

    assert(decompressed_len == orig_len);
    assert(arrays_equal(original, decompressed, orig_len));

    free(compressed);
    free(decompressed);
    printf("Тест пройден: длина %zu\n", orig_len);
}

int main() {
    /* Пример из условия */
    unsigned char test1[] = {100, 100, 100};
    size_t len1 = sizeof(test1);
    test_compress_decompress((char*)test1, len1);

    unsigned char test2[] = {255, 255, 255, 255, 255, 90, 90, 90, 90};
    size_t len2 = sizeof(test2);
    test_compress_decompress((char*)test2, len2);

    /* Неповторяющиеся данные */
    unsigned char test3[] = {35, 64, 12};
    size_t len3 = sizeof(test3);
    test_compress_decompress((char*)test3, len3);

    /* Длинная серия > 255 */
    unsigned char* test4 = malloc(1000);
    for (int i = 0; i < 1000; ++i) test4[i] = 42;
    size_t len4 = 1000;
    test_compress_decompress((char*)test4, len4);
    free(test4);

    /* Смешанные данные */
    unsigned char test5[] = {1,1,2,2,2,3,3,3,3,4,5,5,5,5,5,5};
    size_t len5 = sizeof(test5);
    test_compress_decompress((char*)test5, len5);

    /* Проверка обработки ошибок */
    printf("\nПроверка обработки ошибок:\n");

    size_t dummy_len;
    // NULL на входе
    char* res = rle_compress_1_byte(NULL, 10, &dummy_len);
    assert(res == NULL);
    printf("NULL-вход сжатия: OK\n");

    res = rle_decompress_1_byte(NULL, 10, &dummy_len);
    assert(res == NULL);
    printf("NULL-вход распаковки: OK\n");

    // Нулевая длина
    res = rle_compress_1_byte((char*)test1, 0, &dummy_len);
    assert(res == NULL);
    printf("Нулевая длина сжатия: OK\n");

    res = rle_decompress_1_byte((char*)test1, 0, &dummy_len);
    assert(res == NULL);
    printf("Нулевая длина распаковки: OK\n");

    // Некорректная длина сжатых данных (нечётная)
    unsigned char bad_data[] = {1, 100, 2}; // три байта
    res = rle_decompress_1_byte((char*)bad_data, 3, &dummy_len);
    assert(res == NULL);
    printf("Нечётная длина распаковки: OK\n");

    // Нулевой счётчик в сжатых данных
    unsigned char bad_count[] = {0, 100, 1, 200}; // пара (0,100) недопустима
    res = rle_decompress_1_byte((char*)bad_count, 4, &dummy_len);
    assert(res == NULL);
    printf("Нулевой счётчик распаковки: OK\n");

    printf("\nВсе тесты успешно пройдены.\n");
    return 0;
}