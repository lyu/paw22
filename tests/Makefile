LLVM_PATH ?= ${HOME}/offload_llvm

CXXFLAGS = -Wall -Wextra -std=c++14 -pedantic -O3 -mcpu=native -mtune=native
LDFLAGS = -Wl,-O1

LLVM_CXXFLAGS = -I${LLVM_PATH}/include -fopenmp --offload-arch=sm_70
LLVM_LDFLAGS = -L${LLVM_PATH}/lib -Wl,-rpath,${LLVM_PATH}/lib

SHMEM_CXXFLAGS = -Wno-c99-extensions

SOURCES = $(wildcard *.cpp)
TARGETS = $(patsubst %.cpp,%.x,${SOURCES})

.PHONY: all clean

# Use -gdwarf-4 to get debug info compatible with Summit's OS when using clang
shmem_%.x: shmem_%.cpp
	-oshcxx ${SHMEM_CXXFLAGS} ${CXXFLAGS} ${LLVM_CXXFLAGS} ${LDFLAGS} ${LLVM_LDFLAGS} -fno-pie -no-pie -lucc -o $@ $^

mpi_%.x: mpi_%.cpp
	-mpicxx ${CXXFLAGS} ${LLVM_CXXFLAGS} ${LDFLAGS} ${LLVM_LDFLAGS} -o $@ $^

all: ${TARGETS}

clean:
	rm -f *.x
