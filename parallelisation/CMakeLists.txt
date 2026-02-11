cmake_minimum_required(VERSION 3.16)

project(AntProblem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_OPENMP "Enable OpenMP if available" ON)

set(SOURCES
    src/population.cpp
    src/ant_simu.cpp
    src/fractal_land.cpp
    src/renderer.cpp
    src/window.cpp
)

add_executable(ant_simu ${SOURCES})

target_include_directories(ant_simu
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

find_package(SDL2 REQUIRED)

if(TARGET SDL2::SDL2main AND TARGET SDL2::SDL2)
    target_link_libraries(ant_simu PRIVATE SDL2::SDL2main SDL2::SDL2)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(ant_simu PRIVATE SDL2::SDL2)
endif()

if(MINGW)
    target_link_libraries(ant_simu PRIVATE mingw32)
endif()

find_package(Threads REQUIRED)
target_link_libraries(ant_simu PRIVATE Threads::Threads)

if(ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(ant_simu PRIVATE OpenMP::OpenMP_CXX)
    endif()
endif()

if(MSVC)
    target_compile_options(ant_simu PRIVATE /W4)
else()
    target_compile_options(ant_simu PRIVATE -Wall -Wextra)
endif()