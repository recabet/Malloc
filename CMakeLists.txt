cmake_minimum_required(VERSION 3.29)
project(Malloc C)

set(CMAKE_C_STANDARD 23)

add_executable(Malloc
        heap.c
        heap.h)
