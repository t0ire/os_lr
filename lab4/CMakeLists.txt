cmake_minimum_required(VERSION 3.10)
project(Lab4_DynamicLibraries)

set(CMAKE_CXX_STANDARD 11)

# Настройки для динамических библиотек
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Библиотека 1
add_library(lib1 SHARED lib1.cpp)
target_compile_definitions(lib1 PRIVATE MATHLIB_EXPORTS)

# Библиотека 2  
add_library(lib2 SHARED lib2.cpp)
target_compile_definitions(lib2 PRIVATE MATHLIB_EXPORTS)

# Программа 1 (статическое связывание) - ИСПРАВЛЕНО ИМЯ ФАЙЛА
add_executable(program1 prog1.cpp)
target_link_libraries(program1 lib1)

# Программа 2 (динамическая загрузка) - ИСПРАВЛЕНО ИМЯ ФАЙЛА
add_executable(program2 prog2.cpp)

# Копирование библиотек в выходную директорию
if(WIN32)
    add_custom_command(TARGET program2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lib1> ${CMAKE_BINARY_DIR}/lib1.dll
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lib2> ${CMAKE_BINARY_DIR}/lib2.dll
    )
else()
    add_custom_command(TARGET program2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lib1> ${CMAKE_BINARY_DIR}/liblib1.so
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:lib2> ${CMAKE_BINARY_DIR}/liblib2.so
    )
endif()