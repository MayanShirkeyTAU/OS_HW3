cmake_minimum_required(VERSION 3.19.2)
project(OS_HW3 C)
set(CMAKE_C_STANDARD 11)
include_directories(/usr/src/linux-headers-5.0.0-23/include)
add_executable(OS_HW3 main.c ex3_tester.c message_slot.c message_slot.h message_reader.c message_sender.c)
