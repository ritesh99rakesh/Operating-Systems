cmake_minimum_required(VERSION 3.14)
project(src C)

set(CMAKE_C_STANDARD 11)

include_directories(include)
include_directories(user)

add_executable(src
        include/apic.h
        include/context.h
        include/entry.h
        include/file.h
        include/fs.h
        include/idt.h
        include/init.h
        include/kbd.h
        include/lib.h
        include/memory.h
        include/pipe.h
        include/schedule.h
        include/serial.h
        include/types.h
        user/test_cases/testcase1.c
        user/test_cases/testcase2.c
        user/test_cases/testcase3.c
        user/test_cases/testcase4.c
        user/test_cases/testcase5.c
        user/init.c
        user/lib.c
        user/ulib.h
        apic.c
        context.c
        entry.c
        file.c
        fs.c
        idt.c
        kbd.c
        lib.c
        main.c
        memory.c
        pipe.c
        schedule.c
        serial.c
        shell.c)
