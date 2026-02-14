cmake_minimum_required(VERSION 3.16)

# Prefer MPI wrapper as compiler for this target.
find_program(MPI_CXX_WRAPPER NAMES mpicxx mpic++)
if(NOT DEFINED CMAKE_CXX_COMPILER)
    if(MPI_CXX_WRAPPER)
        set(CMAKE_CXX_COMPILER "${MPI_CXX_WRAPPER}" CACHE FILEPATH "MPI C++ compiler wrapper" FORCE)
    endif()
endif()

project(AntProblem LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ANT_MPI_NP "4" CACHE STRING "Total MPI ranks for run_ant_simu (rank 0 is UI, ranks 1..N-1 are workers)")

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

# If the current compiler does not expose mpi.h directly, inject compile/link
# flags from the MPI wrapper so existing non-MPI build directories still work.
include(CheckIncludeFileCXX)
check_include_file_cxx("mpi.h" HAVE_MPI_HEADER)
if(NOT HAVE_MPI_HEADER)
    if(MPI_CXX_WRAPPER)
        execute_process(
            COMMAND ${MPI_CXX_WRAPPER} --showme:incdirs
            OUTPUT_VARIABLE MPI_INC_DIRS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${MPI_CXX_WRAPPER} --showme:libdirs
            OUTPUT_VARIABLE MPI_LIB_DIRS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${MPI_CXX_WRAPPER} --showme:libs
            OUTPUT_VARIABLE MPI_LIBS
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        separate_arguments(MPI_INC_DIRS NATIVE_COMMAND "${MPI_INC_DIRS}")
        separate_arguments(MPI_LIB_DIRS NATIVE_COMMAND "${MPI_LIB_DIRS}")
        separate_arguments(MPI_LIBS NATIVE_COMMAND "${MPI_LIBS}")
        target_include_directories(ant_simu PRIVATE ${MPI_INC_DIRS})
        target_link_directories(ant_simu PRIVATE ${MPI_LIB_DIRS})
        target_link_libraries(ant_simu PRIVATE ${MPI_LIBS})
    else()
        message(FATAL_ERROR "mpi.h not found and no MPI wrapper (mpicxx/mpic++) is available.")
    endif()
endif()

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

if(MSVC)
    target_compile_options(ant_simu PRIVATE /W4)
else()
    target_compile_options(ant_simu PRIVATE -Wall -Wextra)
endif()

find_program(MPIEXEC_EXECUTABLE NAMES mpiexec mpirun)
set(MPIEXEC_NUMPROC_FLAG "-n" CACHE STRING "Flag used by mpiexec/mpirun to set process count")

if(MPIEXEC_EXECUTABLE)
    add_custom_target(run_ant_simu
        COMMAND ${MPIEXEC_EXECUTABLE}
                ${MPIEXEC_NUMPROC_FLAG} ${ANT_MPI_NP}
                $<TARGET_FILE:ant_simu>
        DEPENDS ant_simu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
        COMMENT "Running ant_simu with ${ANT_MPI_NP} MPI ranks"
    )
else()
    message(WARNING "mpiexec/mpirun not found: run_ant_simu target will not be available.")
endif()
