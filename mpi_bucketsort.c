#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

/* Comparator for qsort */
static int compare_ints(const void *a, const void *b) {
    int ai = *(const int *)a;
    int bi = *(const int *)b;
    return (ai > bi) - (ai < bi);
}

/* Check if sorted */
static int is_sorted(const int *arr, size_t n) {
    for (size_t i = 1; i < n; ++i)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

/* Safe malloc */
static void* xmalloc(size_t nbytes) {
    void *p = malloc(nbytes);
    if (!p) {
        fprintf(stderr, "malloc failed (%zu bytes)\n", nbytes);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return p;
}

/* Quasi-sorted generator (5% swaps) */
static void make_quasi_sorted(int *arr, int64_t n) {
    for (int64_t i = 0; i < n; i++)
        arr[i] = (int)i;

    int64_t swaps = (int64_t)(n * 0.05);
    for (int64_t k = 0; k < swaps; k++) {
        int i1 = rand() % n;
        int i2 = rand() % n;
        int tmp = arr[i1];
        arr[i1] = arr[i2];
        arr[i2] = tmp;
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Args */
    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr,
                    "Usage: %s <n> [pattern=random|quasi|desc] [seed]\n",
                    argv[0]);
        MPI_Finalize();
        return 1;
    }

    int64_t n = atoll(argv[1]);
    if (n <= 0) {
        if (rank == 0) fprintf(stderr, "n must be positive\n");
        MPI_Finalize();
        return 1;
    }

    const char *pattern = (argc >= 3) ? argv[2] : "random";

    unsigned int seed = (argc >= 4)
                        ? (unsigned int)strtoul(argv[3], NULL, 10)
                        : (unsigned int)time(NULL);

    /* Block distribution (allow uneven) */
    int base  = (int)(n / size);
    int extra = (int)(n % size);
    int local_n = base + (rank < extra ? 1 : 0);

    int *local = (int *)xmalloc((size_t)local_n * sizeof(int));

    int *global     = NULL;
    int *sendcounts = NULL;
    int *displs     = NULL;

    /* Rank 0 generates data with pattern */
    if (rank == 0) {
        global     = (int *)xmalloc((size_t)n * sizeof(int));
        sendcounts = (int *)xmalloc(size * sizeof(int));
        displs     = (int *)xmalloc(size * sizeof(int));

        int offset = 0;
        for (int r = 0; r < size; r++) {
            sendcounts[r] = base + (r < extra ? 1 : 0);
            displs[r]     = offset;
            offset       += sendcounts[r];
        }

        srand(seed);
        if (strcmp(pattern, "quasi") == 0) {
            make_quasi_sorted(global, n);
        }
        else if (strcmp(pattern, "desc") == 0) {
            for (int64_t i = 0; i < n; i++)
                global[i] = (int)(n - 1 - i);
        }
        else { /* random */
            for (int64_t i = 0; i < n; i++)
                global[i] = rand() % 100000;
        }
    }

    /* Scatterv uneven blocks */
    MPI_Scatterv(global, sendcounts, displs, MPI_INT,
                 local, local_n, MPI_INT,
                 0, MPI_COMM_WORLD);

    /* Compute local min/max */
    int local_min = INT_MAX;
    int local_max = INT_MIN;
    for (int i = 0; i < local_n; ++i) {
        if (local[i] < local_min) local_min = local[i];
        if (local[i] > local_max) local_max = local[i];
    }

    int gmin, gmax;
    MPI_Allreduce(&local_min, &gmin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(&local_max, &gmax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    int range = gmax - gmin;
    if (range == 0) range = 1;

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    /* Count elements per bucket */
    int *send_cnt = calloc((size_t)size, sizeof(int));
    for (int i = 0; i < local_n; i++) {
        int val = local[i];
        double pos = (double)(val - gmin) / (double)range;
        int dest = (int)(pos * size);
        if (dest >= size) dest = size - 1;
        send_cnt[dest]++;
    }

    /* Prefix sum -> send displacements */
    int *sdispls = (int *)xmalloc(size * sizeof(int));
    sdispls[0] = 0;
    for (int i = 1; i < size; i++)
        sdispls[i] = sdispls[i - 1] + send_cnt[i - 1];

    int total_send = sdispls[size - 1] + send_cnt[size - 1];

    int *send_buf = (int *)xmalloc((size_t)total_send * sizeof(int));
    int *cursor   = (int *)calloc((size_t)size, sizeof(int));

    for (int i = 0; i < local_n; i++) {
        int val = local[i];
        double pos = (double)(val - gmin) / (double)range;
        int dest = (int)(pos * size);
        if (dest >= size) dest = size - 1;
        int idx = sdispls[dest] + cursor[dest]++;
        send_buf[idx] = val;
    }

    free(cursor);
    free(local);

    /* Alltoall counts */
    int *recv_cnt = (int *)xmalloc(size * sizeof(int));
    MPI_Alltoall(send_cnt, 1, MPI_INT,
                 recv_cnt, 1, MPI_INT,
                 MPI_COMM_WORLD);

    int *rdispls = (int *)xmalloc(size * sizeof(int));
    rdispls[0] = 0;
    for (int i = 1; i < size; i++)
        rdispls[i] = rdispls[i - 1] + recv_cnt[i - 1];

    int total_recv = rdispls[size - 1] + recv_cnt[size - 1];
    int *recv_buf  = (int *)xmalloc((size_t)total_recv * sizeof(int));

    MPI_Alltoallv(send_buf, send_cnt, sdispls, MPI_INT,
                  recv_buf, recv_cnt, rdispls, MPI_INT,
                  MPI_COMM_WORLD);

    free(send_cnt);
    free(sdispls);
    free(send_buf);
    free(recv_cnt);
    free(rdispls);

    /* Sort local bucket */
    qsort(recv_buf, (size_t)total_recv, sizeof(int), compare_ints);

    /* Gather sizes */
    int *final_counts = NULL;
    if (rank == 0)
        final_counts = (int *)xmalloc(size * sizeof(int));

    MPI_Gather(&total_recv, 1, MPI_INT,
               final_counts, 1, MPI_INT,
               0, MPI_COMM_WORLD);

    int *final_displs = NULL;
    int  final_total  = 0;
    int *final_arr    = NULL;

    if (rank == 0) {
        final_displs = (int *)xmalloc(size * sizeof(int));
        final_displs[0] = 0;

        for (int i = 1; i < size; i++)
            final_displs[i] = final_displs[i - 1] + final_counts[i - 1];

        final_total = final_displs[size - 1] + final_counts[size - 1];
        final_arr   = (int *)xmalloc((size_t)final_total * sizeof(int));
    }

    /* Gather all buckets to rank 0 */
    MPI_Gatherv(recv_buf, total_recv, MPI_INT,
                final_arr, final_counts, final_displs, MPI_INT,
                0, MPI_COMM_WORLD);

    double t1 = MPI_Wtime();

    if (rank == 0) {
        int ok = is_sorted(final_arr, (size_t)final_total);
        double t = t1 - t0;

        printf("MPI Bucket Sort\n");
        printf("n = %" PRId64 "\n", n);
        printf("pattern = %s\n", pattern);
        printf("processes = %d\n", size);
        printf("time = %.6f s\n", t);
        printf("sorted = %s\n\n", ok ? "yes" : "no");

        FILE *fp = fopen("results_mpi_bucketsort.csv", "a");
        if (fp) {
            if (ftell(fp) == 0)
                fprintf(fp, "n,pattern,time,sorted,processes\n");

            fprintf(fp, "%" PRId64 ",%s,%.6f,%s,%d\n",
                    n, pattern, t, ok ? "yes" : "no", size);

            fclose(fp);
        }

        free(final_arr);
        free(final_counts);
        free(final_displs);
        free(global);
        free(sendcounts);
        free(displs);
    }

    free(recv_buf);

    MPI_Finalize();
    return 0;
}

