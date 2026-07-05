cmake_minimum_required(VERSION 3.10)
project(RLE_Compression VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

# Собираем все исходные файлы из src
file(GLOB SOURCES "src/*.c")

# Создаём динамическую библиотеку (shared)
add_library(rle_lib SHARED ${SOURCES})

# Если нужна статическая библиотека, замените SHARED на STATIC
# add_library(rle_lib STATIC ${SOURCES})

# (Опционально) Тестовая программа
# add_executable(test_rle src/test.c)
# target_link_libraries(test_rle rle_lib)

install(TARGETS rle_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES include/rle_compress_and_decompress.h DESTINATION include)