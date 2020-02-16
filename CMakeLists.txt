cmake_minimum_required(VERSION 3.15)
project(edJSON C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-pg -Wall)

add_executable(edJSON main.c edJSON/edJSON.c edJSON/edjson_processors.c)