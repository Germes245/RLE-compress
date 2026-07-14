# Сборка утилиты rle_1_byte
cmake_minimum_required(VERSION 3.10)
project(rle_1_byte_util LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Путь к заголовкам библиотеки
include_directories(${CMAKE_SOURCE_DIR}/include)

# Исполняемый файл
add_executable(rle_1_byte main.c)

# Линковка с библиотекой rle_lib (собирается в корне)
target_link_libraries(rle_1_byte rle_lib)

# Установка исполняемого файла
install(TARGETS rle_1_byte
    RUNTIME DESTINATION bin
)