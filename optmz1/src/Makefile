INCDIR = ../include

# ---------------------------------------------------------------
# SDL2 (bundled in external/, i686 = 32-bit mingw)
# ---------------------------------------------------------------
SDL2_DIR  = ../external/SDL2-2.30.0/i686-w64-mingw32
SDL2_INC  = -I"$(SDL2_DIR)/include"
SDL2_LIB  = -L"$(SDL2_DIR)/lib" -lSDL2main -lSDL2

# ---------------------------------------------------------------
# MS-MPI (SDK already installed, using x86 (32-bit) lib to match MinGW)
# ---------------------------------------------------------------
MSMPI_INC_DIR = C:/Program Files (x86)/Microsoft SDKs/MPI/Include
MSMPI_LIB_DIR = C:/Program Files (x86)/Microsoft SDKs/MPI/Lib/x86

MPI_INC = -I"$(MSMPI_INC_DIR)"
MPI_LIB = -L"$(MSMPI_LIB_DIR)" -lmsmpi

# ---------------------------------------------------------------
# Compiler
# ---------------------------------------------------------------
CXX = g++

# OpenMP is optional — enable with: mingw32-make all OPENMP=yes
# (requires a MinGW build with pthread support)
ifdef OPENMP
OMPFLAGS = -fopenmp
endif

CXXFLAGS = -std=c++17 -I$(INCDIR) $(SDL2_INC) $(MPI_INC) $(OMPFLAGS)

ifdef DEBUG
CXXFLAGS += -g -O0 -pedantic -D_GLIBCXX_DEBUG
else
CXXFLAGS += -O2 -march=native -Wall -Wextra
endif

# Windows needs -lmingw32 before SDL2main
LIB = -lmingw32 $(SDL2_LIB) $(MPI_LIB)

ALL = ant_simu.exe

default: help

all: $(ALL)

clean:
	@del /q *.o *.exe *~ 2>NUL

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $^ -o $@

ant_simu.exe: population.o fractal_land.o renderer.o window.o ant_simu.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIB)

help:
	@echo Available targets:
	@echo   all   - compile ant_simu.exe
	@echo   clean - remove built files
	@echo Add DEBUG=yes for debug build

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@
