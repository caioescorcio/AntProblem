SDL2_DIR  = ../optmz1/external/SDL2-2.30.0/i686-w64-mingw32
SDL2_INC  = -I"$(SDL2_DIR)/include"
SDL2_LIB  = -L"$(SDL2_DIR)/lib" -lmingw32 -lSDL2main -lSDL2

MSMPI_INC_DIR = C:/Program Files (x86)/Microsoft SDKs/MPI/Include
MSMPI_LIB_DIR = C:/Program Files (x86)/Microsoft SDKs/MPI/Lib/x86
MPI_INC = -I"$(MSMPI_INC_DIR)"
MPI_LIB = -L"$(MSMPI_LIB_DIR)" -lmsmpi

CXX = g++ $(SDL2_INC)
CC  = gcc 
MPICXX = g++ $(SDL2_INC) $(MPI_INC)
LIB = $(SDL2_LIB) $(MPI_LIB)
