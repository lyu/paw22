#!/usr/bin/env bash

PREFIX=$HOME/opt
OPT_FLAGS="-O3 -fno-plt -fno-semantic-interposition"
UARCH_FLAGS="-mcpu=native -mtune=native"

export CFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export CXXFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export LDFLAGS="$OPT_FLAGS $UARCH_FLAGS -Wl,-O1"


git clone --depth 1 https://github.com/openucx/ucc
cd ucc
./autogen.sh
./configure --prefix=$PREFIX                \
            --enable-silent-rules           \
            --enable-optimizations          \
            --enable-option-checking        \
            --disable-debug                 \
            --disable-gtest                 \
            --disable-static                \
            --disable-profiling             \
            --disable-frame-pointer         \
            --disable-dependency-tracking   \
            --without-sharp                 \
            --with-ucx=$PREFIX              \
            --with-mpi="$HOME/.local"       \
            --with-cuda=$CUDA_PATH          \
            --with-nccl=$CUDA_PATH          \
            --with-nvcc-gencode="-gencode=arch=compute_70,code=sm_70"
make -j && make install
cd ..
rm -rf ucc

### Host tests

MPI_TEST_ARGS="--colls barrier,allreduce,allgather,allgatherv,bcast,alltoall,alltoallv,reduce,reduce_scatter,reduce_scatterv,gather,gatherv,scatter,scatterv"
MPI_TEST_ARGS="$MPI_TEST_ARGS --teams world --mtypes host --inplace 2 --msgsize 1:8192:2 --root all --iter 10"

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_test_mpi $MPI_TEST_ARGS \
      --dtypes uint8,uint16,uint32,uint64,int8,int16,int32,int64 \
      --ops avg,sum,prod,max,min,land,lor,lxor,band,bor,bxor

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_test_mpi $MPI_TEST_ARGS \
      --dtypes float32,float32_complex,float64,float64_complex,float128,float128_complex --ops avg,sum,prod,max,min

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -c barrier
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m host -c bcast
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m host -c reduce
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m host -c allreduce
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m host -c alltoall

### CUDA tests

MPI_TEST_ARGS="--colls barrier,allgather,bcast,alltoall,alltoallv,gather,gatherv,scatter,scatterv"
MPI_TEST_ARGS="$MPI_TEST_ARGS --teams world --mtypes cuda --inplace 2 --msgsize 1:8192:2 --root all --iter 10"

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_test_mpi $MPI_TEST_ARGS

# Data types & OPs only matter for reductions
# No reductions for (u)int8
MPI_TEST_ARGS="--colls allreduce,reduce,reduce_scatter,reduce_scatterv"
MPI_TEST_ARGS="$MPI_TEST_ARGS --teams world --mtypes cuda --inplace 2 --msgsize 1:8192:2 --root all --iter 10"

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_test_mpi $MPI_TEST_ARGS \
      --dtypes uint16,uint32,uint64,int16,int32,int64 \
      --ops avg,sum,prod,max,min,land,lor,lxor,band,bor,bxor

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_test_mpi $MPI_TEST_ARGS \
      --dtypes float32,float32_complex,float64,float64_complex --ops avg,sum,prod,max,min

# No inplace allgatherv
MPI_TEST_ARGS="--colls allgatherv"
MPI_TEST_ARGS="$MPI_TEST_ARGS --teams world --mtypes cuda --inplace 0 --msgsize 1:8192:2 --root all --iter 10"

jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_test_mpi $MPI_TEST_ARGS


jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m cuda -c bcast
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m cuda -c reduce
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m cuda -c allreduce
jsrun -b rs -K 3 -c 7 -g 1 ~/opt/bin/ucc_perftest -F -n 10000 -b 1 -e 65536 -d int64 -o max -m cuda -c alltoall
