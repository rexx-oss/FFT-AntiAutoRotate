cmake_minimum_required(VERSION 3.20)
project(fft_anti_autorotate CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master
)
FetchContent_MakeAvailable(minhook)

add_library(AntiAutoRotate SHARED main.cpp)
set_target_properties(AntiAutoRotate PROPERTIES 
    PREFIX "" 
    OUTPUT_NAME "AntiAutoRotate"
)
target_link_libraries(AntiAutoRotate PRIVATE minhook user32)
