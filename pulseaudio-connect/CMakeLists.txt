cmake_minimum_required (VERSION 3.0)
project (main)

# find_library(SOUNDIO libsoundio.so)
file(GLOB SRC_FILES src/**.hpp src/**.cpp)
add_executable(main ${SRC_FILES})
target_link_libraries(main pulse)