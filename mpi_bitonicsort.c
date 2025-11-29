#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>

/* Compare for qsort */
int compare_ints(const void *a, const void *b) {
    int ai = *(const int *)a;
    int bi = *(const int *)b;
    return ai - bi;
}

/* Merge two sorted halves */
void merge_two(const int *a, int na, const int *b, int nb, int *out) {
    int i = 0, j = 0, k = 0;
    while (i < na && j < nb)
        out[k++] = (a[i] <= b[j]) ? a[i++] : b[j++];
    while (i < na) out[k++] = a[i++];
    while (j < nb) out[k++] = b[j++];
}

/* Check sorted */
int is_sorted(const int *arr, int64_t n) {
    for (int64_t i = 1; i < n; i++)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

int is_power_of_two(int x) {
    return (x > 0) && ((x & (x - 1)) == 0);
}

/* Generate quasi-sorted pattern */
void make_quasi_sorted(int *arr, int64_t n, unsigned int seed) {
    srand(seed);
    for (int64_t i = 0; i < n; i++)
        arr[i] = (int)i;

    int64_t swaps = n * 0.05;
    for (int64_t i = 0; i < swaps; i++) {
        int i1 = rand() % n;
        int i2 = rand() % n;
        int t = arr[i1];
        arr[i1] = arr[i2];
        arr[i2] = t;
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
        if (rank == 0) fprintf(stderr, "Error: n must be positive\n");
        MPI_Finalize();
        return 1;
    }

    const char *pattern = (argc >= 3) ? argv[2] : "random";

    unsigned int seed = (argc >= 4)
                        ? (unsigned int)strtoul(argv[3], NULL, 10)
                        : (unsigned int)time(NULL);

    if (!is_power_of_two(size)) {
        if (rank == 0)
            fprintf(stderr, "Error: number of processes must be power of two.\n");
        MPI_Finalize();
        return 1;
    }

    if (n % size != 0) {
        if (rank == 0)
            fprintf(stderr, "Error: n must be divisible by number of processes.\n");
        MPI_Finalize();
        return 1;
    }

    int local_n = n / size;

    /* Global array (only rank 0) */
    int *global_arr = NULL;
    if (rank == 0) {
        global_arr = malloc(n * sizeof(int));

        if (strcmp(pattern, "quasi") == 0) {
            make_quasi_sorted(global_arr, n, seed);
        }
        else if (strcmp(pattern, "desc") == 0) {
            for (int64_t i = 0; i < n; i++)
                global_arr[i] = (int)(n - 1 - i);
        }
        else {  /* random */
            srand(seed);
            for (int64_t i = 0; i < n; i++)
                global_arr[i] = rand() % 100000;
        }
    }

    /* Local buffer */
    int *local_arr = malloc(local_n * sizeof(int));

    MPI_Scatter(global_arr, local_n, MPI_INT,
                local_arr, local_n, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        free(global_arr);
        global_arr = NULL;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    /* Local sort */
    qsort(local_arr, local_n, sizeof(int), compare_ints);

    /* Buffers */
    int *recv_buf = malloc(local_n * sizeof(int));
    int *merged   = malloc(2 * local_n * sizeof(int));

    /* Bitonic merge phases */
    for (int k = 2; k <= size; k <<= 1) {
        for (int j = k >> 1; j > 0; j >>= 1) {

            int partner = rank ^ j;

            MPI_Sendrecv(local_arr, local_n, MPI_INT, partner, 0,
                         recv_buf,  local_n, MPI_INT, partner, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            merge_two(local_arr, local_n, recv_buf, local_n, merged);

            int up  = ((rank & k) == 0);
            int low = ((rank & j) == 0);

            int keep_low = (up && low) || (!up && !low);

            if (keep_low) {
                for (int i = 0; i < local_n; i++)
                    local_arr[i] = merged[i];
            } else {
                for (int i = 0; i < local_n; i++)
                    local_arr[i] = merged[i + local_n];
            }
        }
    }

    double end = MPI_Wtime();

    /* Final gather */
    int *final_arr = NULL;
    if (rank == 0)
        final_arr = malloc(n * sizeof(int));

    MPI_Gather(local_arr, local_n, MPI_INT,
               final_arr, local_n, MPI_INT,
               0, MPI_COMM_WORLD);

    /* Output */
    if (rank == 0) {

        double t = end - start;
        int ok = is_sorted(final_arr, n);

        printf("MPI Bitonic Sort\n");
        printf("n = %" PRId64 "\n", n);
        printf("pattern = %s\n", pattern);
        printf("processes = %d\n", size);
        printf("time = %.6f s\n", t);
        printf("sorted = %s\n\n", ok ? "yes" : "no");

        FILE *fp = fopen("results_mpi_bitonic.csv", "a");
        if (fp) {
            if (ftell(fp) == 0)
                fprintf(fp, "n,pattern,time,sorted,processes\n");

            fprintf(fp, "%" PRId64 ",%s,%.6f,%s,%d\n",
                n, pattern, t, ok ? "yes" : "no", size);

            fclose(fp);
        }

        free(final_arr);
    }

    free(local_arr);
    free(recv_buf);
    free(merged);

    MPI_Finalize();
    return 0;
}

