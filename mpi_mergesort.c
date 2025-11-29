#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

/* Comparator */
int compare_ints(const void *a, const void *b) {
    int ai = *(const int *)a;
    int bi = *(const int *)b;
    return ai - bi;
}

/* Merge two sorted arrays */
void merge(int *a, int na, int *b, int nb, int *out) {
    int i = 0, j = 0, k = 0;
    while (i < na && j < nb)
        out[k++] = (a[i] <= b[j]) ? a[i++] : b[j++];
    while (i < na) out[k++] = a[i++];
    while (j < nb) out[k++] = b[j++];
}

/* Check sorted */
int is_sorted(int *arr, int n) {
    for (int i = 1; i < n; i++)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

/* Generate quasi-sorted */
void make_quasi_sorted(int *arr, int64_t n) {
    for (int64_t i = 0; i < n; i++)
        arr[i] = (int)i;

    int64_t swaps = (int64_t)(n * 0.05);
    for (int64_t i = 0; i < swaps; i++) {
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

    /* --- Args --- */
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

    srand(seed);

    /* --- Local sizes (uneven distribution allowed) --- */
    int base = n / size;
    int extra = n % size;
    int local_n = base + (rank < extra ? 1 : 0);

    int *local_arr = malloc(local_n * sizeof(int));

    int *full_arr = NULL;
    int *sendcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        full_arr = malloc(n * sizeof(int));
        sendcounts = malloc(size * sizeof(int));
        displs = malloc(size * sizeof(int));

        int offset = 0;
        for (int r = 0; r < size; r++) {
            sendcounts[r] = base + (r < extra ? 1 : 0);
            displs[r] = offset;
            offset += sendcounts[r];
        }

        if (strcmp(pattern, "quasi") == 0) {
            make_quasi_sorted(full_arr, n);
        }
        else if (strcmp(pattern, "desc") == 0) {
            for (int64_t i = 0; i < n; i++)
                full_arr[i] = (int)(n - 1 - i);
        }
        else {  // random
            for (int64_t i = 0; i < n; i++)
                full_arr[i] = rand() % 100000;
        }
    }

    /* --- Scatterv --- */
    MPI_Scatterv(
        full_arr, sendcounts, displs, MPI_INT,
        local_arr, local_n, MPI_INT,
        0, MPI_COMM_WORLD
    );

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    /* Local sort */
    qsort(local_arr, local_n, sizeof(int), compare_ints);

    /* --- Merge in tree --- */
    int step = 1;
    while (step < size) {

        if (rank % (2 * step) == 0) {
            int partner = rank + step;

            if (partner < size) {
                int incoming_n;
                MPI_Recv(&incoming_n, 1, MPI_INT, partner, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int *incoming = malloc(incoming_n * sizeof(int));
                MPI_Recv(incoming, incoming_n, MPI_INT, partner, 1,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int *merged = malloc((local_n + incoming_n) * sizeof(int));
                merge(local_arr, local_n, incoming, incoming_n, merged);

                free(local_arr);
                free(incoming);

                local_arr = merged;
                local_n += incoming_n;
            }
        }
        else {
            int parent = rank - step;
            MPI_Send(&local_n, 1, MPI_INT, parent, 0, MPI_COMM_WORLD);
            MPI_Send(local_arr, local_n, MPI_INT, parent, 1, MPI_COMM_WORLD);

            free(local_arr);
            local_arr = NULL;
            break;
        }

        step *= 2;
    }

    double end = MPI_Wtime();

    /* --- Output (rank 0) --- */
    if (rank == 0) {
        double t = end - start;
        int ok = is_sorted(local_arr, local_n);

        printf("MPI MergeSort\n");
        printf("n = %" PRId64 "\n", n);
        printf("pattern = %s\n", pattern);
        printf("processes = %d\n", size);
        printf("time = %.6f s\n", t);
        printf("sorted = %s\n\n", ok ? "yes" : "no");

        FILE *fp = fopen("results_mpi_mergesort.csv", "a");
        if (fp) {
            if (ftell(fp) == 0)
                fprintf(fp, "n,pattern,time,sorted,processes\n");

            fprintf(fp, "%" PRId64 ",%s,%.6f,%s,%d\n",
                    n, pattern, t, ok ? "yes" : "no", size);
            fclose(fp);
        }
    }

    if (rank == 0) {
        free(full_arr);
        free(sendcounts);
        free(displs);
    }

    free(local_arr);
    MPI_Finalize();
    return 0;
}

