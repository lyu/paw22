## Installation

All install scripts are under the `scripts` folder

System must have GPUDirectRDMA and GDRCopy support

Install the software in the following in order:

 * Install PMIx
 * Install UCX
 * Install UCC
 * Install LLVM
 * Install OSSS-UCX

## Running Benchmarks

Go to the tests folder and build all the tests using `make`

The job script for Summit is provided as `tests/eval.sh`

### Required Environment Variables

`export UCX_UNIFIED_MODE=y`

`export UCX_MEM_CUDA_HOOK_MODE=reloc`

`export UCX_PROTO_ENABLE=y`

`export UCX_ZCOPY_THRESH=0`

`export LIBOMPTARGET_MEMORY_MANAGER_THRESHOLD=0`

For OSSS-UCX export `OSH_CC=$HOME/offload_llvm/bin/clang` and `OSH_CXX=$HOME/offload_llvm/bin/clang++`

For Spectrum MPI export `OMPI_CC=$HOME/offload_llvm/bin/clang` and `OMPI_CXX=$HOME/offload_llvm/bin/clang++`
