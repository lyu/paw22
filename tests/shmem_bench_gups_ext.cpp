/*
 * GUPS (Giga UPdates per Second) is a measurement that profiles the memory architecture of a system
 * and is a measure of performance similar to FLOPS. It is intended to exercise the GUPS capability
 * of a system. In each case, we would expect these benchmarks to achieve close to the "peak"
 * capability of the memory system.
 *
 * GUPS is calculated by identifying the number of memory locations that can be randomly updated in
 * one second, divided by 1 billion (1e9). The term "randomly" means that there is little
 * relationship between one address to be updated and the next, except that they occur in the space
 * of one half the total system memory.  An update is a read-modify-write operation on a table of
 * 64-bit words. An address is generated, the value at that address read from memory, modified by an
 * integer operation (add, and, or, xor) with a literal value, and that new value is written back to
 * memory.
 *
 * Basic requirements:
 * 1. Size of the table is a power of two and approximately half the global memory
 * 2. Look-ahead and storage constraints being followed
 *
 * The entire table is allocated via shmem_malloc and all update locations are ensured to be both
 * random and remote.
 */
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <ctime>
#include <cmath>

#include <shmem.h>

/* RNG */
#define POLY 7
#define PERIOD 1317624576693539401L


extern "C" int __tgt_set_device_allocator(int64_t, void*, void*);
extern "C" int __tgt_reset_device_allocator(int64_t);

float timediff_us(const timespec& t_start, const timespec& t_end)
{
    return (t_end.tv_sec - t_start.tv_sec) * 1.0e6 + (t_end.tv_nsec - t_start.tv_nsec) / 1.0e3;
}


/* Utility routine to start random number generator at Nth step */
int64_t get_seed(uint64_t n)
{
    int i;
    uint64_t m2[64];
    int64_t temp, ran;

    while (n > PERIOD)
        n -= PERIOD;
    if (n == 0)
        return 1;

    temp = 1;
    for (i = 0; i < 64; i++) {
        m2[i] = temp;
        temp  = (temp << 1) ^ (temp < 0 ? POLY : 0);
        temp  = (temp << 1) ^ (temp < 0 ? POLY : 0);
    }

    for (i = 62; i >= 0; i--)
        if ((n >> i) & 1)
            break;

    ran = 2;

    while (i > 0) {
        temp = 0;
        for (int j = 0; j < 64; j++)
            if ((ran >> j) & 1)
                temp ^= m2[j];
        ran = temp;
        i -= 1;
        if ((n >> i) & 1)
            ran = (ran << 1) ^ (ran < 0 ? POLY : 0);
    }

    return ran;
}


int main()
{
    setenv("SHMEM_MAX_PARTITIONS", "1", 1);
    setenv("SHMEM_SYMMETRIC_PARTITION0", "SIZE=8G:KIND=LIBOMP", 1);

    const bool verify = false;

    const int table_len_log = 19;
    const int64_t table_len = 1L << table_len_log;
    const int64_t num_updates = 4 * table_len;

    shmem_init();
    assert(0 == __tgt_set_device_allocator(0, (void*)shmem_partition_malloc, (void*)shmem_partition_free));

    const int npes = shmem_n_pes();
    const int mype = shmem_my_pe();

    const int64_t table_len_tot = table_len * npes;
    const int64_t num_updates_tot = 4 * table_len_tot;

    auto table       = (uint64_t*)shmem_malloc(sizeof(uint64_t) * table_len);
    auto updates     = (int64_t*)shmem_malloc(sizeof(int64_t) * npes);
    auto all_updates = (int64_t*)shmem_malloc(sizeof(int64_t));
    auto pSync       = (long*)shmem_malloc(sizeof(long) * SHMEM_REDUCE_SYNC_SIZE);
    auto pWrk        = (long*)shmem_malloc(sizeof(long) * SHMEM_REDUCE_MIN_WRKDATA_SIZE);

    for (int64_t i = 0; i < table_len; i++)
        table[i] = mype;

    for (int i = 0; i < npes; i++) {
        updates[i]   = 0;
        *all_updates = 0;
    }

    for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    if (mype == 0) {
        printf("PE table length = %ld words\n", table_len);
        printf("Total table length = %ld words\n", table_len_tot);
        printf("Number of updates = %ld\n", num_updates_tot);
    }

    int64_t ran = get_seed(4 * table_len * mype);

    timespec t0, t1;

    #pragma omp target data map(to:table[0:table_len])
    {
        shmem_barrier_all();
        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

        for (int64_t i = 0; i < num_updates; i++) {
            ran           = (ran << 1) ^ (ran < 0 ? POLY : 0);
            int remote_pe = (ran >> table_len_log) & (npes - 1);

            // Forces updates to remote PE only
            if (remote_pe == mype)
                remote_pe = (mype + 1) % npes;

            #pragma omp target data use_device_ptr(table)
            {
                const uint64_t v = shmem_uint64_g(&table[ran & (table_len - 1)], remote_pe) ^ ran;
                shmem_uint64_p(&table[ran & (table_len - 1)], v, remote_pe);
            }
            shmem_quiet();

            if (verify)
                shmem_long_atomic_inc(&updates[mype], remote_pe);
        }

        shmem_barrier_all();
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
    }

    const float T = timediff_us(t0, t1) / 1.0e6;

    if (mype == 0) {
        const double GUPs = 1e-9 * num_updates_tot / T;
        printf("Real time used = %.6f seconds\n", T);
        printf("%.9f Billion(10^9) Updates    per second [GUP/s]\n", GUPs);
        printf("%.9f Billion(10^9) Updates/PE per second [GUP/s]\n", GUPs / npes);
    }

    if (verify) {
        for (int i = 1; i < npes; i++)
            updates[0] += updates[i];
        printf("PE %d updates:%ld\n", mype, updates[0]);

        shmem_long_sum_to_all(all_updates, updates, 1, 0, 0, npes, pWrk, pSync);
        if (mype == 0) {
            if (num_updates_tot == *all_updates)
                printf("Verification passed!\n");
            else
                printf("Verification failed!\n");
        }
    }

    shmem_free(updates);
    shmem_free(all_updates);
    shmem_free(table);
    shmem_free(pSync);
    shmem_free(pWrk);

    assert(0 == __tgt_reset_device_allocator(0));
    shmem_finalize();
}
