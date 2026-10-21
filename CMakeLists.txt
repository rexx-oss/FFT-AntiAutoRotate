cmake_minimum_required(VERSION 3.20)
project(fft_freecam CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master
)
FetchContent_MakeAvailable(minhook)

add_library(freecam SHARED main.cpp)
set_target_properties(freecam PROPERTIES SUFFIX ".asi")
target_link_libraries(freecam PRIVATE minhook user32)
