cmake_minimum_required(VERSION 3.15)
project(Assignment4 C)

set(CMAKE_C_STANDARD 11)

include_directories(part-1)
include_directories(part-2)
include_directories(part-3)

add_executable(Assignment4
        part-1/common.h
        part-1/main.c
        part-1/thread_func.c
        part-2/common.h
        part-2/main.c
        part-2/rwlock.c
        part-2/rwlock1.c
        part-3/common.h
        part-3/inputgen.c
        part-3/main.c
        part-3/parallel_hash.c
        part-3/parallel_hash1.c
        part-3/serial_hash.c
        rwlock.c
        thread_func.c)
