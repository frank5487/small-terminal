cmake_minimum_required(VERSION 3.24)
project(HW4)

set(CMAKE_CXX_STANDARD 14)

add_executable(HW4
        pipe_shell.cc
        sh.cc
        stdin_echo.cc multi_pipe.cc two_pipe.cc fail_pipe_shell.cc)
