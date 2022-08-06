#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ctime>

#include <shmem.h>


long psync[SHMEM_ALLTOALL_SYNC_SIZE];


float timediff_us(const timespec& t_start, const timespec& t_end)
{
    return (t_end.tv_sec - t_start.tv_sec) * 1.0e6 + (t_end.tv_nsec - t_start.tv_nsec) / 1.0e3;
}


int main()
{
    for (int i = 0; i < SHMEM_ALLTOALL_SYNC_SIZE; i++) {
        psync[i] = SHMEM_SYNC_VALUE;
    }

    shmem_init();

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
                    #pragma omp target update from(sbuf[0:sz])
                    shmem_putmem(rbuf, sbuf, sz, 1);
                    shmem_quiet();
                    #pragma omp target update to(rbuf[0:sz])
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

                #pragma omp target update from(sbuf[0:sz*npes])
                shmem_alltoall32(rbuf, sbuf, sz >> 5, 0, 0, npes, psync);
                #pragma omp target update to(rbuf[0:sz*npes])
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

    shmem_finalize();
}
