#include <iostream>
#include <complex>
#include <cmath>
#include <ctime>

#include <shmem.h>


using real = float;
using cmplx = std::complex<real>;


long psync[SHMEM_ALLTOALL_SYNC_SIZE];


int reverse_bits(int m, const int max_n_bits) {
    int n = 0;

    for (int i = 0; i < max_n_bits; i++) {
        n <<= 1;
        n |= m & 1;
        m >>= 1;
    }

    return n;
}


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

    const int mype = shmem_my_pe();
    const int npes = shmem_n_pes();

    const int Nb = (1 << 12) / npes;    // Local block matrix size
    const int stripe_len = Nb * Nb * npes;

    // Fill one block first, then next (block-after-block, block-major row-major)
    auto f = (cmplx*)shmem_malloc(stripe_len * sizeof(cmplx));
    auto F = (cmplx*)shmem_malloc(stripe_len * sizeof(cmplx));

    #pragma omp target data map(tofrom:f[0:stripe_len]) map(alloc:F[0:stripe_len])
    {
        #pragma omp target teams distribute parallel for
        for (int c = 0; c < Nb; c++) {              // Column of the block matrix
            for (int r = 0; r < Nb; r++) {          // Row of the block matrix
                for (int p = 0; p < npes; p++) {    // ID of the block matrix (also destination PE)
                    const real x = 1.3 + std::sin(2 * M_PI * (Nb * p + c) / (Nb * npes));
                    f[p * Nb * Nb + r * Nb + c] = cmplx(x, 0.0);
                }
            }
        }

        timespec t0, t1;
        shmem_sync_all();
        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

        const int max_n_bits = std::log2(Nb * npes);

        #pragma omp target teams distribute parallel for
        for (int r = 0; r < Nb; r++) {
            for (int p = 0; p < npes; p++) {
                for (int c = 0; c < Nb; c++) {
                    const int idx = Nb * p + c;
                    const int idx_r = reverse_bits(idx, max_n_bits);
                    const int idx_flat = Nb * Nb * p + Nb * r + (idx_r % Nb);
                    F[idx_flat] = f[idx_flat];
                }
            }
        }

        #pragma omp target teams distribute parallel for
        for (int r = 0; r < Nb; r++) {
            for (int s = 1; s <= max_n_bits; s++) {
                // Size of the even/odd sub-transformation at this stage
                const int Ns = std::pow(2, s);

                // The twiddle factor for this step
                cmplx ws = std::polar<real>(1.0, -2 * M_PI / Ns);

                for (int i = 0; i < (Nb * npes); i += Ns) {
                    cmplx w = cmplx(1.0, 0.0);

                    for (int j = 0; j < (Ns / 2); j++) {
                        const int idx = Nb * Nb * (i / Nb) + r * Nb + (i % Nb);
                        const cmplx t = w * F[idx + j + Ns/2];
                        const cmplx u = F[idx + j];

                        F[idx + j] = u + t;
                        F[idx + j + Ns/2] = u - t;

                        w *= ws;
                    }
                }
            }
        }

        #pragma omp target update from(F[0:stripe_len])

        shmem_sync_all();

        shmem_alltoall32(f, F, Nb * Nb * sizeof(cmplx) / 32, 0, 0, npes, psync);

        #pragma omp target update to(f[0:stripe_len])

        #pragma omp target teams distribute parallel for
        for (int r = 0; r < Nb; r++) {
            for (int p = 0; p < npes; p++) {
                for (int c = 0; c < Nb; c++) {
                    const int idx = Nb * p + c;
                    const int idx_r = reverse_bits(idx, max_n_bits);
                    const int idx_flat = Nb * Nb * p + Nb * r + (idx_r % Nb);
                    F[idx_flat] = f[idx_flat];
                }
            }
        }

        #pragma omp target teams distribute parallel for
        for (int r = 0; r < Nb; r++) {
            for (int s = 1; s <= max_n_bits; s++) {
                // Size of the even/odd sub-transformation at this stage
                const int Ns = std::pow(2, s);

                // The twiddle factor for this step
                cmplx ws = std::polar<real>(1.0, -2 * M_PI / Ns);

                for (int i = 0; i < (Nb * npes); i += Ns) {
                    cmplx w = cmplx(1.0, 0.0);

                    for (int j = 0; j < (Ns / 2); j++) {
                        const int idx = Nb * Nb * (i / Nb) + r * Nb + (i % Nb);
                        const cmplx t = w * F[idx + j + Ns/2];
                        const cmplx u = F[idx + j];

                        F[idx + j] = u + t;
                        F[idx + j + Ns/2] = u - t;

                        w *= ws;
                    }
                }
            }
        }

        shmem_sync_all();
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
        if (mype == 0) {
            std::cout << "Standard SHMEM + OpenMP: " << timediff_us(t0, t1) << " us\n";
        }
    }

    shmem_free(F);
    shmem_free(f);

    shmem_finalize();
}
