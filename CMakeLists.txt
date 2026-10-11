cmake_minimum_required(VERSION 3.24)
project(bonus_dm_october_homework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bonus main.cpp)

enable_testing()
add_test(NAME examples COMMAND bonus --demo)
