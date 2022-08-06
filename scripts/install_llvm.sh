#!/usr/bin/env bash

PREFIX=$HOME/offload_llvm
MAKE_JOBS=40

export LDFLAGS="-Wl,-O1"


git clone --branch shmem --depth 1 https://github.com/shiltian/llvm-project llvm_src
cmake -S llvm_src/llvm                              \
      -B llvm_objdir                                \
      -G Ninja                                      \
      -DCMAKE_INSTALL_PREFIX="$PREFIX"              \
      -DCMAKE_BUILD_TYPE=Release                    \
      -DCMAKE_C_COMPILER="$HOME/.local/bin/gcc"     \
      -DCMAKE_CXX_COMPILER="$HOME/.local/bin/g++"   \
      -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS"           \
      -DLLVM_BUILD_UTILS=OFF                        \
      -DLLVM_ENABLE_PROJECTS="clang"                \
      -DLLVM_ENABLE_RUNTIMES="openmp"               \
      -DLLVM_TARGETS_TO_BUILD="host;NVPTX"          \
      -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON              \
      -DGCC_INSTALL_PREFIX="$HOME/.local"           \
      -DCLANG_ENABLE_ARCMT=OFF                      \
      -DCLANG_ENABLE_STATIC_ANALYZER=OFF            \
      -DCLANG_OPENMP_NVPTX_DEFAULT_ARCH=sm_70       \
      -DLIBOMP_OMPT_SUPPORT=OFF                     \
      -DLIBOMP_INSTALL_ALIASES=OFF                  \
      -DLIBOMP_USE_HWLOC=ON                         \
      -DLIBOMP_HWLOC_INSTALL_DIR="$HOME/.local"     \
      -DLIBOMPTARGET_ENABLE_DEBUG=OFF               \
      -DLIBOMPTARGET_BUILD_AMDGPU_PLUGIN=OFF        \
      -DLIBOMPTARGET_AMDGCN_GFXLIST=""              \
      -DLIBOMPTARGET_BUILD_NVPTX_BCLIB=ON           \
      -DLIBOMPTARGET_NVPTX_COMPUTE_CAPABILITIES=70  \
      -DLIBOMPTARGET_NVPTX_ALTERNATE_HOST_COMPILER="$HOME/.local/bin/gcc"
ninja -C llvm_objdir -j $MAKE_JOBS install

rm -rf llvm_*
