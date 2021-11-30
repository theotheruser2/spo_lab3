cmake_minimum_required(VERSION 3.5)
project(Lab3 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab3 main.c client.c client.h server.c server.h opts.c opts.h msg.h msg.c)

target_link_libraries(Lab3 pthread)
target_link_libraries(Lab3 ncurses)
