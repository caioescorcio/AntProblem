# AntProblem
Parallel computing approach for the Ant Problem - OS02 course at ENSTA Paris


# MingW + UCRT64 execution

How to execute the code on Windows:

## Setup

First, assure that the correct libs are installed for the code execution (SDL2 + CMake for compilation).

After installing UCRT64 on Windows, update its current libraries:

```bash
pacman -Syu
```

Then install the packages:

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2
pacman -S mingw-w64-ucrt-x86_64-cmake
```

## Execution

In this project you may use Makefile (if on Linux or Windows's MingW32 Makefile) with:

 ```bash
 cd {PROJET FOLDER}/src
 make
 ant_simu.exe
 ```

For executing the project with CMake, you may use:

```bash
cd {PROJET FOLDER}
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
ant_simu.exe
```

