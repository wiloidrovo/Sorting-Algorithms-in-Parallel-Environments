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
   Partition (pivot fijo)
   ============================ */
static int partition(int *arr, int low, int high) {
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
   QuickSort secuencial
   ============================ */
static void quicksort_seq(int *arr, int low, int high) {
    while (low < high) {
        int p = partition(arr, low, high);

        /* tail-recursion minimization */
        if (p - low < high - p) {
            quicksort_seq(arr, low, p - 1);
            low = p + 1;
        } else {
            quicksort_seq(arr, p + 1, high);
            high = p - 1;
        }
    }
}

/* ============================
   QuickSort paralelo (OpenMP tasks)
   ============================ */
static void quicksort_omp(int *arr, int low, int high, int cutoff) {

    if (high - low <= cutoff) {
        /* Subproblema pequeño -> versión secuencial */
        quicksort_seq(arr, low, high);
        return;
    }

    int p = partition(arr, low, high);

    /* Lado izquierdo en una tarea */
    #pragma omp task shared(arr) firstprivate(low, p, cutoff)
    {
        quicksort_omp(arr, low, p - 1, cutoff);
    }

    /* Lado derecho en otra tarea */
    #pragma omp task shared(arr) firstprivate(p, high, cutoff)
    {
        quicksort_omp(arr, p + 1, high, cutoff);
    }

    /* Esperar a ambas tareas antes de regresar */
    #pragma omp taskwait
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
    int cutoff  = 50000;

    double start = 0.0, end = 0.0;

    #pragma omp parallel
    {
        #pragma omp single
        {
            start = omp_get_wtime();
            quicksort_omp(arr, 0, (int)n - 1, cutoff);
            #pragma omp taskwait
            end = omp_get_wtime();
        }
    }

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
