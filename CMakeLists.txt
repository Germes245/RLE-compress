cmake_minimum_required(VERSION 3.10)
project(RLE_Compression VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

# Собираем все исходные файлы библиотеки из src
file(GLOB SOURCES "src/*.c")
add_library(rle_lib SHARED ${SOURCES})

# Добавляем тестовый исполняемый файл
add_executable(test_rle test/main.c)
target_link_libraries(test_rle rle_lib)
target_include_directories(test_rle PRIVATE include)

# Установка (опционально)
install(TARGETS rle_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES include/rle_compress_and_decompress_1_byte.h DESTINATION include)

add_subdirectory(terminal_utile)