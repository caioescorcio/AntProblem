cmake_minimum_required(VERSION 3.16)

project(AntProblem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(MPI_HOME "C:/Program Files (x86)/Microsoft SDKs/MPI")
    set(MSMPI_INC "${MPI_HOME}/Include")
    set(MSMPI_LIB "${MPI_HOME}/Lib/x64/msmpi.lib")
endif()

find_package(MPI REQUIRED)
find_package(OpenMP)

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

target_link_libraries(ant_simu PRIVATE MPI::MPI_CXX)

# SDL2 dependency
# Assuming standard find_package works or using specific paths if needed
# On Windows/MSYS2 this usually works if installed
find_package(SDL2 REQUIRED)

if(TARGET SDL2::SDL2main AND TARGET SDL2::SDL2)
    target_link_libraries(ant_simu PRIVATE SDL2::SDL2main SDL2::SDL2)
elseif(TARGET SDL2::SDL2)
    target_link_libraries(ant_simu PRIVATE SDL2::SDL2)
else() 
    # Fallback to variables if targets not defined
    target_include_directories(ant_simu PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ant_simu PRIVATE ${SDL2_LIBRARIES})
endif()

if(MINGW)
    target_link_libraries(ant_simu PRIVATE mingw32)
endif()

if(OpenMP_CXX_FOUND)
    target_link_libraries(ant_simu PRIVATE OpenMP::OpenMP_CXX)
endif()

if(MSVC)
    target_compile_options(ant_simu PRIVATE /W4)
else()
    target_compile_options(ant_simu PRIVATE -Wall -Wextra)
endif()
