#!/usr/bin/env bash

PREFIX=$HOME/opt
OPT_FLAGS="-O3 -fno-plt -fno-semantic-interposition"
UARCH_FLAGS="-mcpu=native -mtune=native"

export CFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export CXXFLAGS="$OPT_FLAGS $UARCH_FLAGS"
export LDFLAGS="$OPT_FLAGS $UARCH_FLAGS -Wl,-O1"


cd ../osss-ucx
./autogen.sh
./configure --prefix=$PREFIX                    \
            --enable-threads                    \
            --enable-option-checking            \
            --disable-debug                     \
            --disable-static                    \
            --disable-logging                   \
            --disable-aligned-addresses         \
            --with-ucx="$PREFIX"                \
            --with-pmix="$HOME/.local"          \
            --with-libomp="$HOME/offload_llvm"  \
            --with-heap-size=512M
bear make -j && make install
