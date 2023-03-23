cmake_minimum_required(VERSION 3.24)
project(little_compiler C)

set(CMAKE_C_STANDARD 11)

add_executable(little_compiler main.c parse.c tokenize.c type.c codegen.c)
