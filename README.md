# AntProblem: Parallel Computing for Ant Colony Optimization

This repository contains the complete implementation of a parallelized Ant Colony Optimization (ACO) simulation on fractal terrains. This project was developed as part of the OS02 course at ENSTA Paris.

## Project Overview

The goal of this project is to optimize the performance of an ACO simulation by progressively applying various parallel computing techniques. The simulation features ants navigating a fractal terrain, laying pheromones, and searching for food to bring back to their nest. 

The optimizations are layered incrementally across several subdirectories, allowing for clear performance comparisons between different paradigms: baseline object-oriented, data-oriented (vectorized), shared-memory (OpenMP), and distributed-memory (MPI) approaches.

## Repository Layout & Implementation Variants

- **`nonvectorized/`**: The baseline implementation featuring an object-per-ant design. Includes real-time SDL rendering and per-iteration timing exports to CSV.
- **`vectorized/`**: A data-oriented redesign of the baseline. Ants are managed in contiguous arrays to improve cache locality and enable compiler vectorization.
- **`vectorized_omp/`**: Shared-memory parallelization using OpenMP, built on top of the vectorized baseline. Threads share the global map and parallelize ant processing and pheromone updates.
- **`optmz1/` (MPI Population Decomposition)**: Distributed-memory parallelization where each MPI process contains the entire map but only manages a subset of the total ant population. Pheromones are synchronized globally via `MPI_Allreduce`.
- **`distributed_subdomain_mpi/` (MPI Domain Decomposition)**: Advanced distributed-memory parallelization using a 2D Cartesian topology. The terrain is divided into subdomains handled by separate MPI processes. Processes only communicate ghost/halo pheromone borders and migrating ants.
- **`distributed_subdomain_hybrid_mpi_omp/`**: A hybrid approach combining MPI domain decomposition across nodes with OpenMP shared-memory multithreading within each node.

## Setup and Requirements

### Linux
The project requires a C++17 compatible compiler, CMake, and the SDL2 library for rendering. For MPI features, an MPI implementation like OpenMPI or MPICH is required. OpenMP must also be supported by your compiler.

Install dependencies (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install -y build-essential libsdl2-dev cmake openmpi-bin libopenmpi-dev
```

### Windows (MinGW + UCRT64)
To build and run on Windows, you must install MSYS2 with the UCRT64 environment. Also, MS-MPI must be installed for the MPI variants.

1. Install MSYS2 and open the UCRT64 terminal.
2. Update packages and install dependencies:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-SDL2
pacman -S mingw-w64-ucrt-x86_64-cmake
pacman -S mingw-w64-ucrt-x86_64-toolchain
```
3. Install MS-MPI SDK and Runtime from the official Microsoft site to compile the MPI versions.

## Building and Running

You can compile each variant individually by navigating to its directory. Both `make` and `CMake` are configured differently depending on the subdirectory.

### Using CMake (Recommended)
```bash
cd <variant_directory> # e.g., cd vectorized
mkdir build && cd build
cmake -G "MinGW Makefiles" .. # On Windows
# OR
cmake .. # On Linux
cmake --build .
./ant_simu.exe
```

### Using Makefile (Linux / MinGW)
For projects without a `CMakeLists.txt`, or if using make directly:
```bash
cd <variant_directory>
make clean
make all CXXFLAGS2='-std=c++17 -O2 -march=native -Wall'
./ant_simu.exe
```

### Running MPI Versions
For MPI implementations (`optmz1`, `distributed_subdomain_mpi`), use `mpiexec` or `mpirun`:
```bash
mpiexec -n 4 ./ant_simu.exe
```
*Note: For the domain decomposition variants, passing `--headless` might be necessary depending on your environment to avoid multiple processes attempting to draw to the same window.*

## Command-Line Arguments & Profiling 

All variants support the following CLI flags to customize the execution and profile performance:
- `--headless`: Disable the SDL rendering output.
- `--max-iterations N`: Automatically terminate after `N` iterations.
- `--warmup-iterations N`: Skip the initial `N` iterations for timing statistics.
- `--timing-csv PATH`: Export detailed per-iteration timings to a specific file.
- `--summary-csv PATH`: Export the aggregated metrics summary to a specific file.

For example, a typical profiling run on the hybrid implementation:
```bash
mpiexec -n 4 ./ant_simu.exe --max-iterations 1000 --headless
```

Results are written by default to `results/iter.csv` and `results/summary.csv` inside the respective variant's folder. These logs track individual task timings like `ants_advance_ms`, `evaporation_ms`, `update_ms`, and communication overheads, which can be visualized using the provided python scripts.
