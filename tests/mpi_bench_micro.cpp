#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstring>
#include <ctime>

#include <mpi.h>


float timediff_us(const timespec& t_start, const timespec& t_end)
{
    return (t_end.tv_sec - t_start.tv_sec) * 1.0e6 + (t_end.tv_nsec - t_start.tv_nsec) / 1.0e3;
}


int main()
{
    MPI_Init(nullptr, nullptr);

    const size_t N = 1UL << 31;
    auto sbuf = new uint8_t[N];
    auto rbuf = new uint8_t[N];
    memset(sbuf, 0, N);
    memset(rbuf, 0, N);

    #pragma omp target data map(alloc:sbuf[0:N],rbuf[0:N])
    {
        timespec t0, t1;
        const size_t max_iters = 1e5;
        int mype, npes;
        MPI_Comm_rank(MPI_COMM_WORLD, &mype);
        MPI_Comm_size(MPI_COMM_WORLD, &npes);

        #pragma omp target teams distribute parallel for
        for (size_t i = 0; i < N; i++) {
            sbuf[i] = 0;
            rbuf[i] = 0;
        }

        MPI_Barrier(MPI_COMM_WORLD);

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
                    #pragma omp target data use_device_ptr(sbuf)
                    MPI_Send(sbuf, sz, MPI_BYTE, 1, i, MPI_COMM_WORLD);
                }
                clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

                std::cout << std::setw(9) << sz << ' '
                          << std::scientific << std::setprecision(3)
                          << timediff_us(t0, t1) / n_iters
                          << " us" << std::endl;
            }
        } else if ((mype == 1) && (npes == 2)) {
            for (size_t sz = 1; sz <= N; sz <<= 1) {
                const size_t n_iters = [&] {
                    if (sz > (1L << 20)) {
                        return max_iters / (sz >> 20);
                    }
                    return max_iters;
                } ();

                const size_t n_warmup = n_iters / 100;

                for (size_t i = 0; i < n_iters + n_warmup; i++) {
                    #pragma omp target data use_device_ptr(rbuf)
                    MPI_Recv(rbuf, sz, MPI_BYTE, 0, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

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
                MPI_Alltoall(sbuf, sz, MPI_BYTE, rbuf, sz, MPI_BYTE, MPI_COMM_WORLD);
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

    delete [] sbuf;
    delete [] rbuf;

    MPI_Finalize();
}
