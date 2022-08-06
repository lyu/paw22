#!/usr/bin/env bash

# To get the correct version of PMIX that works with jrun:
# module load spectrum-mpi => which mpirun => check pmix_version.h in the include directory


PREFIX=$HOME/.local
OPT_FLAGS="-O3 -fno-plt -fno-semantic-interposition"
UARCH_FLAGS="-mcpu=native -mtune=native"

export CFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export CXXFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export LDFLAGS="$OPT_FLAGS $UARCH_FLAGS -s -Wl,-O1,--as-needed"


wget https://github.com/openpmix/openpmix/releases/download/v3.2.1/pmix-3.2.1.tar.bz2
tar xf pmix-3.2.1.tar.bz2
cd pmix-3.2.1
./configure --prefix=$PREFIX                \
            --enable-silent-rules           \
            --enable-option-checking        \
            --disable-dlopen                \
            --disable-man-pages             \
            --disable-pmix-binaries         \
            --disable-dependency-tracking   \
            --with-zlib="$HOME/.local"      \
            --with-hwloc="$HOME/.local"     \
            --with-libevent="$HOME/.local"  \
            --with-platform=optimized
make -j && make install
cd ..
rm -rf pmix*
