#include <iostream>
#include <utility>
#include <cassert>
#include <ctime>

#include <shmem.h>


extern "C" int __tgt_set_device_allocator(int64_t, void*, void*);
extern "C" int __tgt_reset_device_allocator(int64_t);


void print_matrix(const float* mat, const int Is, const int Js)
{
    for (int i = 0; i < Is; i++) {
        for (int j = 0; j < Js; j++)
            std::cout << mat[i * Js + j] << ' ';
        std::cout << '\n';
    }
}


float timediff_us(const timespec& t_start, const timespec& t_end)
{
    return (t_end.tv_sec - t_start.tv_sec) * 1.0e6 + (t_end.tv_nsec - t_start.tv_nsec) / 1.0e3;
}


int main()
{
    setenv("SHMEM_MAX_PARTITIONS", "1", 1);
    setenv("SHMEM_SYMMETRIC_PARTITION0", "SIZE=8G:KIND=LIBOMP", 1);

    shmem_init();
    assert(0 == __tgt_set_device_allocator(0, (void*)shmem_partition_malloc, (void*)shmem_partition_free));

    const int mype = shmem_my_pe();
    const int npes = shmem_n_pes();

    const int Ns = 2000;        // Width of the stripes
    const int N = Ns * npes;    // Size of the matrices
    const size_t stripe_size = N * Ns * sizeof(float);

    if (mype == 0)
        std::cout << "Matrix stripe: " << N << 'x' << Ns << ", " << stripe_size << " bytes\n";

    auto As = (float*)shmem_align(4096, stripe_size);   // Horizontal stripes of A
    auto Bs = (float*)shmem_align(4096, stripe_size);   // Vertical stripes of B
    auto Cs = (float*)shmem_align(4096, stripe_size);   // Horizontal stripes of C
    auto Bn = (float*)shmem_align(4096, stripe_size);   // Next stripe of B

    // Initialize the matrices
    for(int i = 0; i < N * Ns; i++) {
        As[i] = (i + mype) % 11 + 7;
        Bs[i] = (i + mype) % 13 + 5;
        Cs[i] = 0;
        Bn[i] = 0;
    }

    #pragma omp target data map(to:As[0:N*Ns],Bs[0:N*Ns]) map(tofrom:Cs[0:N*Ns]) map(alloc:Bn[0:N*Ns])
    {
        timespec t0, t1;

        // Make sure all the stripes are initialized
        shmem_sync_all();
        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

        for (int s = 0; s < npes; s++) {
            const int block_num = (mype + s) % npes;

            #pragma omp target data use_device_ptr(Bs,Bn)
            shmem_getmem_nbi(Bn, Bs, stripe_size, (mype + 1) % npes);

            float* const Cb = Cs + block_num * Ns;

            #pragma omp target teams distribute parallel for
            for (int k = 0; k < N; k++) {
                for (int j = 0; j < Ns; j++) {
                    const float b_kj = Bs[k * Ns + j];
                    for (int i = 0; i < Ns; i++) {
                        Cb[i * N + j] += As[i * N + k] * b_kj;
                    }
                }
            }

            shmem_quiet();

            std::swap(Bs, Bn);

            shmem_sync_all();
        }

        shmem_sync_all();
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);

        if (mype == 0) {
            std::cout << "Extended SHMEM + OpenMP: " << timediff_us(t0, t1) << " us\n";
        }
    }

    // Collect and print the matrix product
    if ((mype == 0) && (N < 32)) {
        auto C = new float[N * N];

        for (int i = 0; i < Ns * N; i++)
            C[i] = Cs[i];

        for (int i = 1; i < npes; i++)
            shmem_getmem_nbi(C + i * Ns * N, Cs, stripe_size, i);

        shmem_quiet();

        print_matrix(C, N, N);

        delete[] C;
    }

    shmem_barrier_all();

    shmem_free(Bn);
    shmem_free(Cs);
    shmem_free(Bs);
    shmem_free(As);

    assert(0 == __tgt_reset_device_allocator(0));
    shmem_finalize();
}
