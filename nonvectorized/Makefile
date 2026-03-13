include Make_linux.inc
#include Make_msys2.inc
#include Make_osx.inc

CXXFLAGS_BASE = -std=c++17 -ffp-contract=off
ifeq ($(DEBUG),yes)
CXXFLAGS2 ?= $(CXXFLAGS_BASE) -g -O0 -Wall -fbounds-check -pedantic -D_GLIBCXX_DEBUG
else
CXXFLAGS2 ?= $(CXXFLAGS_BASE) -O2 -march=native -Wall
endif

ALL= ant_simu.exe 

default:	help

all: $(ALL)

clean:
	@rm -fr *.o *.exe *~

.cpp.o:
	$(CXX) $(CXXFLAGS2) -c $^ -o $@	

ant_simu.exe : ant.o fractal_land.o renderer.o window.o ant_simu.o
	$(CXX) $(CXXFLAGS2) $^ -o $@ $(LIB)	

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CXXFLAGS :    $(CXXFLAGS2)"

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@
