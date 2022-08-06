#!/usr/bin/env bash

#BSUB -P CSC401
#BSUB -W 2:00
#BSUB -alloc_flags smt1
#BSUB -nnodes 2
#BSUB -q debug
#BSUB -J omp_shmem
#BSUB -o out_%J.log
#BSUB -e err_%J.log


# echo $LS_SUBCWD
# echo $LSB_MCPU_HOSTS
# module list

# module load spectrum-mpi
# export OMPI_CC=$HOME/offload_llvm/bin/clang
# export OMPI_CXX=$HOME/offload_llvm/bin/clang++

# make -j > /dev/null 2>&1


### Point-to-point
### 2 nodes, 1 GPU on each node
# jsrun -b rs -r 1 -c 1 -g 1 ./shmem_bench_micro.x
# jsrun -b rs -r 1 -c 1 -g 1 ./shmem_bench_micro_ext.x
# jsrun -b rs -r 1 -c 1 -g 1 --smpiargs="-gpu" ./mpi_bench_micro.x

### All-to-all
### 8 nodes, all 6 GPUs on each node
# jsrun -b rs -K 3 -c 1 -g 1 ./shmem_bench_micro.x
# jsrun -b rs -K 3 -c 1 -g 1 ./shmem_bench_micro_ext.x
# jsrun -b rs -K 3 -c 1 -g 1 --smpiargs="-gpu" ./mpi_bench_micro.x

### GUPs
### Power-of-two GPUs, use 2 GPUs per socket => 1, 1, 2, 4, 8, 16, 32 nodes
### Standard SHMEM must use 2 CPUs for the polling thread
### MPI message truncated: use larger chunk size for large runs
# jsrun -b rs -K 2 -c 2 -g 1 -n 2 ./shmem_bench_gups.x
# jsrun -b rs -K 2 -c 1 -g 1 -n 2 ./shmem_bench_gups_ext.x
# jsrun -b rs -K 2 -c 1 -g 1 -n 2 --smpiargs="-gpu" ./mpi_bench_gups.x

### FFT
### Power-of-two GPUs, use 2 GPUs per socket => 1, 1, 2, 4, 8, 16, 32 nodes
### Fix matrix size 2^12 * 2^12, use r1 - K1 - K2 - K3
# jsrun -b rs -r 1 -c 1 -g 1 -n 2   ./shmem_bench_fft.x
# jsrun -b rs -r 1 -c 1 -g 1 -n 4   ./shmem_bench_fft.x
# jsrun -b rs -r 1 -c 1 -g 1 -n 8   ./shmem_bench_fft.x
# jsrun -b rs -r 1 -c 1 -g 1 -n 16  ./shmem_bench_fft.x
# jsrun -b rs -r 1 -c 1 -g 1 -n 32  ./shmem_bench_fft.x
# jsrun -b rs -K 1 -c 1 -g 1 -n 64  ./shmem_bench_fft.x
# jsrun -b rs -K 2 -c 1 -g 1 -n 128 ./shmem_bench_fft.x

### 3D Heat equation
### Use 6 GPUs stepping, 1 ~ 10 nodes
### Remove residual calculation since it's only for verification and causes allocation/sync overhead
### We want more mesh-internal cross-GPU communication, so split x-y-z as evenly as possible
### z-axis is the slowest one, finer splits lead to crazy super-linear scaling, so keep it 3
### Inter-node as much as possible, so use r1 - K1 - K2 - K3
# jsrun -b rs -r 1 -c 1 -g 1 -n 6  ./shmem_bench_heat3d.x -x 1 -y 2 -z 3 -M 420 -I 10000
# jsrun -b rs -K 1 -c 1 -g 1 -n 12 ./shmem_bench_heat3d.x -x 2 -y 2 -z 3 -M 420 -I 10000
# jsrun -b rs -K 1 -c 1 -g 1 -n 18 ./shmem_bench_heat3d.x -x 2 -y 3 -z 3 -M 420 -I 10000
# jsrun -b rs -K 2 -c 1 -g 1 -n 24 ./shmem_bench_heat3d.x -x 2 -y 4 -z 3 -M 420 -I 10000
# jsrun -b rs -K 2 -c 1 -g 1 -n 30 ./shmem_bench_heat3d.x -x 2 -y 5 -z 3 -M 420 -I 10000
# jsrun -b rs -K 2 -c 1 -g 1 -n 36 ./shmem_bench_heat3d.x -x 3 -y 4 -z 3 -M 420 -I 10000
# jsrun -b rs -K 3 -c 1 -g 1 -n 42 ./shmem_bench_heat3d.x -x 2 -y 7 -z 3 -M 420 -I 10000
# jsrun -b rs -K 3 -c 1 -g 1 -n 48 ./shmem_bench_heat3d.x -x 4 -y 4 -z 3 -M 420 -I 10000
# jsrun -b rs -K 3 -c 1 -g 1 -n 54 ./shmem_bench_heat3d.x -x 3 -y 6 -z 3 -M 420 -I 10000
# jsrun -b rs -K 3 -c 1 -g 1 -n 60 ./shmem_bench_heat3d.x -x 4 -y 5 -z 3 -M 420 -I 10000

### Matrix multiplication
### Use 6 GPUs stepping, 1 ~ 10 nodes, fix matrices size to be 30240 * 30240
### The rest is same as above
