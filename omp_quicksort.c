#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <omp.h>

/* ============================
     Swap
   ============================ */
static inline void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

/* ============================
     Randomized partition
   ============================ */
static int partition(int *arr, int low, int high) {
    int pivot_index = low + rand() % (high - low + 1);
    swap(&arr[pivot_index], &arr[high]);

    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }

    swap(&arr[i + 1], &arr[high]);
    return i + 1;
}

/* ============================
     Parallel QuickSort (tasks)
   ============================ */
static void quicksort_omp(int *arr, int low, int high, int cutoff) {

    while (low < high) {
        int p = partition(arr, low, high);

        /* Crear tarea solo si el subproblema es suficientemente grande */
        if ((p - 1 - low) > cutoff) {
            #pragma omp task
            quicksort_omp(arr, low, p - 1, cutoff);
        } else {
            quicksort_omp(arr, low, p - 1, cutoff);
        }

        /* Avanzar por la otra mitad */
        low = p + 1;
    }
}

/* ============================
      Check sorted
   ============================ */
static int is_sorted(const int *arr, int64_t n) {
    for (int64_t i = 1; i < n; i++)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

/* ============================
     Quasi-sorted generator
   ============================ */
void make_quasi_sorted(int *arr, int64_t n, unsigned int seed) {
    srand(seed);

    for (int64_t i = 0; i < n; i++)
        arr[i] = (int)i;

    int64_t swaps = n * 0.05;

    for (int64_t i = 0; i < swaps; i++) {
        int a = rand() % n;
        int b = rand() % n;
        int temp = arr[a];
        arr[a] = arr[b];
        arr[b] = temp;
    }
}

/* ============================
             MAIN
   ============================ */
int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr,
            "Usage: %s <n> [pattern=random|quasi|desc] [seed]\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    int64_t n = atoll(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Error: n must be positive\n");
        return EXIT_FAILURE;
    }

    const char *pattern = "random";
    if (argc >= 3)
        pattern = argv[2];

    unsigned int seed = (argc >= 4)
                          ? (unsigned int)strtoul(argv[3], NULL, 10)
                          : (unsigned int)time(NULL);

    srand(seed);

    int *arr = malloc(n * sizeof(int));
    if (!arr) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    /* Generate data */
    if (strcmp(pattern, "quasi") == 0) {
        make_quasi_sorted(arr, n, seed);

    } else if (strcmp(pattern, "desc") == 0) {
        for (int64_t i = 0; i < n; i++)
            arr[i] = (int)(n - 1 - i);

    } else {
        for (int64_t i = 0; i < n; i++)
            arr[i] = rand() % 100000;
    }

    int threads = omp_get_max_threads();
    int cutoff = 10000; // tamaño mínimo para crear tareas
    double start, end;

    start = omp_get_wtime();

    #pragma omp parallel
    {
        #pragma omp single nowait
        quicksort_omp(arr, 0, (int)n - 1, cutoff);
    }

    end = omp_get_wtime();

    double t = end - start;
    int ok = is_sorted(arr, n);

    printf("OpenMP QuickSort\n");
    printf("n = %" PRId64 "\n", n);
    printf("pattern = %s\n", pattern);
    printf("threads = %d\n", threads);
    printf("time = %.6f s\n", t);
    printf("sorted = %s\n\n", ok ? "yes" : "no");

    /* Save to CSV */
    FILE *fp = fopen("results_omp_quicksort.csv", "a");
    if (fp) {
        if (ftell(fp) == 0)
            fprintf(fp, "n,pattern,threads,time,sorted\n");

        fprintf(fp, "%" PRId64 ",%s,%d,%.6f,%s\n",
                n, pattern, threads, t, ok ? "yes" : "no");

        fclose(fp);
    }

    free(arr);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}