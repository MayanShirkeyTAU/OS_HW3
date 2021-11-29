cmake_minimum_required(VERSION 3.19.2)
project(OS_HW3 C)

set(CMAKE_C_STANDARD 11)

add_executable(OS_HW3 main.c message_slot.c message_slot.h message_reader.c message_sender.c)
