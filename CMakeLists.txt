cmake_minimum_required(VERSION 3.8)

project(AnyRef CXX)

add_executable(example example.cpp )

target_compile_options(example PRIVATE
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
    $<$<CXX_COMPILER_ID:GNU>:-Wall>
    $<$<CXX_COMPILER_ID:Clang>:-Wall>
    $<$<CXX_COMPILER_ID:MSVC>:-W4 -Zc:__cplusplus -utf-8>
)
target_compile_features(example PRIVATE cxx_std_17)