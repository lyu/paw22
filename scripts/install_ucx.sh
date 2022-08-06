#!/usr/bin/env bash

PREFIX=$HOME/opt
OPT_FLAGS="-O3 -fno-plt -fno-semantic-interposition"
UARCH_FLAGS="-mcpu=native -mtune=native"

export CFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export CXXFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export LDFLAGS="$OPT_FLAGS $UARCH_FLAGS -Wl,-O1"


git clone --branch v1.12.x --depth 1 https://github.com/openucx/ucx
cd ucx
./autogen.sh
./configure --prefix=$PREFIX                \
            --enable-mt                     \
            --enable-cma                    \
            --enable-numa                   \
            --enable-silent-rules           \
            --enable-optimizations          \
            --enable-builtin-memcpy         \
            --enable-compiler-opt=3         \
            --enable-option-checking        \
            --disable-debug                 \
            --disable-gtest                 \
            --disable-stats                 \
            --disable-static                \
            --disable-tuning                \
            --disable-logging               \
            --disable-examples              \
            --disable-profiling             \
            --disable-assertions            \
            --disable-debug-data            \
            --disable-params-check          \
            --disable-fault-injection       \
            --disable-frame-pointer         \
            --disable-dependency-tracking   \
            --without-go                    \
            --without-bfd                   \
            --without-mpi                   \
            --without-knem                  \
            --without-java                  \
            --without-valgrind              \
            --with-cache-line-size=128      \
            --with-cuda=$CUDA_PATH          \
            --with-gdrcopy=/sw/summit/gdrcopy/2.0
make -j && make install
cd ..
rm -rf ucx
