#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ctime>

#include "coll.hpp"


extern "C" int __tgt_set_device_allocator(int64_t, void*, void*);
extern "C" int __tgt_reset_device_allocator(int64_t);


float timediff_us(const timespec& t_start, const timespec& t_end)
{
    return (t_end.tv_sec - t_start.tv_sec) * 1.0e6 + (t_end.tv_nsec - t_start.tv_nsec) / 1.0e3;
}


int main()
{
    setenv("SHMEM_MAX_PARTITIONS", "1", 1);
    setenv("SHMEM_SYMMETRIC_PARTITION0", "SIZE=8G:KIND=LIBOMP", 1);

    shmem_init();
    coll_init();
    assert(0 == __tgt_set_device_allocator(0, (void*)shmem_partition_malloc, (void*)shmem_partition_free));

    const size_t N = 1UL << 31;
    auto sbuf = (uint8_t*)shmem_malloc(N);
    auto rbuf = (uint8_t*)shmem_malloc(N);
    memset(sbuf, 0, N);
    memset(rbuf, 0, N);

    #pragma omp target data map(alloc:sbuf[0:N],rbuf[0:N])
    {
        timespec t0, t1;
        const size_t max_iters = 1e5;
        const int mype = shmem_my_pe();
        const int npes = shmem_n_pes();

        #pragma omp target teams distribute parallel for
        for (size_t i = 0; i < N; i++) {
            sbuf[i] = 0;
            rbuf[i] = 0;
        }

        shmem_barrier_all();

        if ((mype == 0) && (npes == 2)) {
            for (size_t sz = 1; sz <= N; sz <<= 1) {
                const size_t n_iters = [&] {
                    if (sz > (1L << 20)) {
                        return max_iters / (sz >> 20);
                    }
                    return max_iters;
                } ();

                const size_t n_warmup = n_iters / 100;

                for (size_t i = 0; i < n_iters + n_warmup; i++) {
                    if (i == n_warmup) {
                        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
                    }
                    #pragma omp target data use_device_ptr(sbuf,rbuf)
                    shmem_putmem(rbuf, sbuf, sz, 1);
                    shmem_quiet();
                }
                clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

                std::cout << std::setw(9) << sz << ' '
                          << std::scientific << std::setprecision(3)
                          << timediff_us(t0, t1) / n_iters
                          << " us" << std::endl;
            }
        }

        shmem_barrier_all();

        for (size_t sz = 32; (sz * npes) <= N; sz <<= 1) {
            const size_t n_iters = [&] {
                if (sz > (1L << 16)) {
                    return max_iters / (sz >> 16);
                }
                return max_iters;
            } ();

            const size_t n_warmup = n_iters / 100;

            for (size_t i = 0; i < n_iters + n_warmup; i++) {
                if (i == n_warmup) {
                    clock_gettime(CLOCK_MONOTONIC_RAW, &t0);
                }

                #pragma omp target data use_device_ptr(sbuf,rbuf)
                shmemx_alltoall(rbuf, sbuf, sz, UCC_MEMORY_TYPE_CUDA);
            }
            clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

            if (mype == 0) {
                std::cout << std::setw(9) << sz << ' '
                          << std::scientific << std::setprecision(3)
                          << timediff_us(t0, t1) / n_iters
                          << " us" << std::endl;
            }
        }
    }

    shmem_free(sbuf);
    shmem_free(rbuf);

    assert(0 == __tgt_reset_device_allocator(0));
    coll_finalize();
    shmem_finalize();
}
