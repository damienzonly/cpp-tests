cmake_minimum_required (VERSION 3.0)
project (main)

find_library(SOUNDIO libsoundio.so)
add_executable(main main.cpp)
target_link_libraries(main soundio ${SOUNDIO})