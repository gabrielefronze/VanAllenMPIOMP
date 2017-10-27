#compiler definition
CC=clang++
MPICC=clang-omp++

#flags declaration
CFLAGS=-lncurses -O3
CFLAGSSEQ=`root-config --cflags --libs` -std=c++11  -O3
MPICFLAGS= -I/usr/local/Cellar/open-mpi/2.0.1_1/include -L/usr/local/opt/libevent/lib -L/usr/local/Cellar/open-mpi/2.0.1_1/lib -lmpi -fopenmp -I /usr/local/opt/libiomp/include/libiomp `root-config --cflags --libs` -std=c++11 -O3

all: VanAllen Monitor VanAllenSeq

VanAllen: MainWithFunc.cpp FunctionMaster.cpp FunctionWorker.cpp FunctionFinalizer.cpp FunctionMasterFinalizer.cpp socketcomm.cpp
	$(MPICC) $(MPICFLAGS) MainWithFunc.cpp FunctionMaster.cpp FunctionWorker.cpp FunctionFinalizer.cpp FunctionMasterFinalizer.cpp socketcomm.cpp -o VanAllen

Monitor: VanAllenMonitor.cpp socketcomm.cpp
	$(CC) $(CFLAGS) VanAllenMonitor.cpp socketcomm.cpp -o Monitor

VanAllenSeq: Sequential.cpp
	$(CC) $(CFLAGSSEQ) Sequential.cpp -o VanAllenSeq

clean:
	rm VanAllen Monitor VanAllenSeq
