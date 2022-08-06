// Hypercube-style synchronous alltoall, must use power-of-2 number of procs
#include <iostream>
#include <cstdint>
#include <ctime>

#include <mpi.h>

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* RNG */
#define POLY 7
#define PERIOD 1317624576693539401L


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
    const bool verify = false;

    const int table_len_log = 19;
    const int64_t table_len = 1L << table_len_log;
    const int64_t table_len_m1 = table_len - 1; // for index computation
    const int64_t num_updates = 4 * table_len;
    const int chunk = 32; // Message could (randomly) exceed length 1

    int mype, npes;
    MPI_Init(nullptr, nullptr);
    MPI_Comm_rank(MPI_COMM_WORLD, &mype);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);

    const int64_t table_len_tot = table_len * npes;
    const int64_t num_updates_tot = 4 * table_len_tot;

    auto table = new uint64_t[table_len]();
    auto data  = new uint64_t[chunk]();
    auto send  = new uint64_t[chunk]();

    // Globally, table[i] = i
    for (int64_t i = 0; i < table_len; i++)
        table[i] = i + table_len * mype;

    int npes_log = 0;
    while ((1 << npes_log) < npes)
        npes_log++;

    int64_t ran = get_seed(4 * table_len * mype);

    timespec t0, t1;

    int64_t ndata_max  = 0;
    int64_t nfinal_max = 0;
    int64_t nexcess   = 0;
    int64_t nbad      = 0;

    #pragma omp target data map(tofrom:data[0:chunk],send[0:chunk])
    {
        MPI_Barrier(MPI_COMM_WORLD);
        clock_gettime(CLOCK_MONOTONIC_RAW, &t0);

        // Generate one random value per proc
        // Communicate datums to correct processor via hypercube routing
        // Use received values to update local table
        for (int64_t i = 0; i < num_updates; i++) {
            ran     = (ran << 1) ^ (ran < 0 ? POLY : 0);
            data[0] = ran;
            int ndata = 1;

            for (int j = 0; j < npes_log; j++) {
                int nsend = 0;
                int nkeep = 0;
                const int remote_pe = (1 << j) ^ mype;
                const uint64_t procmask = 1UL << (table_len_log + j);
                if (remote_pe > mype) {
                    for (int64_t k = 0; k < ndata; k++) {
                        if (data[k] & procmask)
                            send[nsend++] = data[k];
                        else
                            data[nkeep++] = data[k];
                    }
                } else {
                    for (int64_t k = 0; k < ndata; k++) {
                        if (data[k] & procmask)
                            data[nkeep++] = data[k];
                        else
                            send[nsend++] = data[k];
                    }
                }

                MPI_Status status;

                #pragma omp target data use_device_ptr(data,send)
                MPI_Sendrecv(send, nsend, MPI_UINT64_T, remote_pe, 0, &data[nkeep], chunk, MPI_UINT64_T, remote_pe, 0, MPI_COMM_WORLD, &status);

                int nrecv;
                MPI_Get_count(&status, MPI_UINT64_T, &nrecv);
                ndata = nkeep + nrecv;
                ndata_max = MAX(ndata_max, ndata);
            }
            nfinal_max = MAX(nfinal_max, ndata);
            if (ndata > 1)
                nexcess += ndata - 1;

            for (int64_t j = 0; j < ndata; j++) {
                const uint64_t datum = data[j];
                const uint64_t index = datum & table_len_m1;
                table[index] ^= datum;
            }

            if (verify) {
                const uint64_t procmask = ((uint64_t)(npes - 1)) << table_len_log;
                for (int64_t j = 0; j < ndata; j++)
                    if ((data[j] & procmask) >> table_len_log != (uint64_t)mype)
                        nbad++;
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
    }

    float T_sum;
    float T = timediff_us(t0, t1) / 1.0e6;
    MPI_Allreduce(&T, &T_sum, 1, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
    T = T_sum / npes;

    int64_t tmp_i64;

    tmp_i64 = ndata_max;
    MPI_Allreduce(&tmp_i64, &ndata_max, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);

    tmp_i64 = nfinal_max;
    MPI_Allreduce(&tmp_i64, &nfinal_max, 1, MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD);

    int64_t nexcess_tot;
    MPI_Allreduce(&nexcess, &nexcess_tot, 1, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD);

    int64_t nbad_tot;
    MPI_Allreduce(&nbad, &nbad_tot, 1, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD);

    const double GUPs     = num_updates_tot / T / 1.0e9;

    if (mype == 0) {
        printf("Number of procs: %d\n", npes);
        printf("Vector size: %ld\n", table_len_tot);
        printf("Max datums during comm: %ld\n", ndata_max);
        printf("Max datums after comm: %ld\n", nfinal_max);
        printf("Excess datums (frac): %ld (%g)\n", nexcess_tot, (double)nexcess_tot / num_updates_tot);
        printf("Bad locality count: %ld\n", nbad_tot);
        printf("Update time (secs): %9.3f\n", T);
        printf("GUPs: %9.6f\n", GUPs);
    }

    delete [] table;
    delete [] data;
    delete [] send;

    MPI_Finalize();
}
