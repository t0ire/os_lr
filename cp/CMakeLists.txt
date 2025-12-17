cmake_minimum_required(VERSION 3.10)
project(bulls_and_cows C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Флаги компиляции
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pthread")

# Требуемые библиотеки для Linux
find_package(Threads REQUIRED)

# Исполняемые файлы
add_executable(server 
    server.c
    game_logic.c
    os_linux.c  # ДОБАВИТЬ ЭТУ СТРОКУ
)

add_executable(client 
    client.c
    game_logic.c
    os_linux.c  # ДОБАВИТЬ ЭТУ СТРОКУ
)

# Подключение библиотек
target_link_libraries(server 
    PRIVATE 
    ${CMAKE_THREAD_LIBS_INIT}
    rt
)

target_link_libraries(client 
    PRIVATE 
    ${CMAKE_THREAD_LIBS_INIT}
    rt
)

# Установка для удобства
install(TARGETS server client 
        RUNTIME DESTINATION bin)