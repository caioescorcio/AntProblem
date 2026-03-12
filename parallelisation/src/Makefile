#include Make_linux.inc
include Make_msys2.inc
#include Make_osx.inc

INCDIR = ../include

CXXFLAGS = -std=c++17 -I$(INCDIR)

ifdef DEBUG
CXXFLAGS += -g -O0 -fopenmp fbounds-check -pedantic -D_GLIBCXX_DEBUG # using the OpenMP but using the same -OX optmisation flags from the other codeas
CXXFLAGS2 = CXXFLAGS
else
CXXFLAGS2 = ${CXXFLAGS} -O2 -march=native -Wall 
CXXFLAGS += -O3 -march=native -Wall
endif

ALL= ant_simu.exe 

default:	help

all: $(ALL)

clean:
	@del /q *.o *.exe *~

.cpp.o:
	$(CXX) $(CXXFLAGS2) -c $^ -o $@	

ant_simu.exe : population.o fractal_land.o renderer.o window.o ant_simu.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CXXFLAGS :    $(CXXFLAGS)"

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@
