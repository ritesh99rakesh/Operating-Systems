cmake_minimum_required(VERSION 3.15)
project(src C)

set(CMAKE_C_STANDARD 11)

include_directories(include)
include_directories(user)

add_executable(src
        include/apic.h
        include/cfork.h
        include/context.h
        include/entry.h
        include/idt.h
        include/init.h
        include/kbd.h
        include/lib.h
        include/memory.h
        include/mmap.h
        include/page.h
        include/schedule.h
        include/serial.h
        include/types.h
        user/Testcases/Task-1/testcase1.c
        user/Testcases/Task-1/testcase2.c
        user/Testcases/Task-1/testcase3.c
        user/Testcases/Task-1/testcase4.c
        user/Testcases/Task-1/testcase5.c
        user/Testcases/Task-1/testcase6.c
        user/Testcases/Task-2/testcase1.c
        user/Testcases/Task-2/testcase2.c
        user/Testcases/Task-2/testcase3.c
        user/Testcases/Task-3/testcase1.c
        user/Testcases/Task-3/testcase2.c
        user/Testcases/Task-3/testcase3.c
        user/Testcases/Task-3/testcase4.c
        user/Testcases/Task-3/testcase5.c
        user/Testcases/Task-3/testcase6.c
        user/Testcases/Task-3/testcase7.c
        user/Testcases/Task-3/testcase8.c
        user/Testcases/Task-3/testcase9.c
        user/init.c
        user/init_args.c
        user/init_clone.c
        user/lib.c
        user/ulib.h
        apic.c
        cfork.c
        context.c
        entry.c
        idt.c
        kbd.c
        lib.c
        main.c
        memory.c
        mmap.c
        page.c
        schedule.c
        serial.c
        shell.c)
