cmake_minimum_required(VERSION 2.8)
project(ydotool)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        ydotool.cpp CommonIncludes.hpp Commands.hpp Commands.cpp
        Commands/Type.cpp Commands/Key.cpp)


add_executable(ydotool ${SOURCE_FILES})
target_link_libraries(ydotool boost_program_options uInputPlus)