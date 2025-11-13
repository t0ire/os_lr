cmake_minimum_required(VERSION 3.10)
project(MaxMinParallel VERSION 1.0.0 LANGUAGES CXX)

# Установка стандарта C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включение поддержки потоков
find_package(Threads REQUIRED)

# Настройка компилятора
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -pthread)
endif()

# Исходные файлы
set(SOURCES
    main.cpp
    maxmin.cpp
    thread.cpp
)

# Заголовочные файлы
set(HEADERS
    maxmin.h
    thread.h
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Подключение библиотеки потоков
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Установка свойств для Windows
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Установка выходной директории
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)