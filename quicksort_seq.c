#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>

/* ============================
   Swap
   ============================ */
static void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

/* ============================
   RANDOMIZED PARTITION 
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
   Recursive QuickSort
   ============================ */
static void quicksort(int *arr, int low, int high) {
    while (low < high) {
        int p = partition(arr, low, high);

        if (p - low < high - p) {
            quicksort(arr, low, p - 1);
            low = p + 1;
        } else {
            quicksort(arr, p + 1, high);
            high = p - 1;
        }
    }
}

/* ============================
   Check sorted
   ============================ */
static int is_sorted(const int *arr, size_t n) {
    for (size_t i = 1; i < n; i++)
        if (arr[i - 1] > arr[i])
            return 0;
    return 1;
}

/* ============================
   Timing helper
   ============================ */
static double elapsed_seconds(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec)
         + (end.tv_nsec - start.tv_nsec) / 1e9;
}

/* ============================
   Generate quasi-sorted data
   ============================ */
void make_quasi_sorted(int *arr, int64_t n, unsigned int seed) {
    (void)seed;  // seed ya se usa en main con srand()

    for (int64_t i = 0; i < n; i++)
        arr[i] = (int)i;

    int64_t swaps = (int64_t)(n * 0.05);

    for (int64_t i = 0; i < swaps; i++) {
        int idx1 = rand() % n;
        int idx2 = rand() % n;
        int tmp = arr[idx1];
        arr[idx1] = arr[idx2];
        arr[idx2] = tmp;
    }
}

/* ============================
            MAIN
   ============================ */
int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr,
        "Usage: %s <n> [pattern=random|quasi|desc] [seed]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int64_t n = atoll(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Error: n must be positive\n");
        return EXIT_FAILURE;
    }

    const char *pattern = "random";
    if (argc >= 3) pattern = argv[2];

    unsigned int seed = (argc >= 4)
                          ? (unsigned int)strtoul(argv[3], NULL, 10)
                          : (unsigned int)time(NULL);

    srand(seed);

    int *arr = malloc(n * sizeof(int));
    if (!arr) {
        fprintf(stderr, "Error allocating memory\n");
        return EXIT_FAILURE;
    }

    if (strcmp(pattern, "quasi") == 0) {

        make_quasi_sorted(arr, n, seed);

    } else if (strcmp(pattern, "desc") == 0) {

        for (int64_t i = 0; i < n; i++)
            arr[i] = (int)(n - 1 - i);

    } else { // random (default)

        for (int64_t i = 0; i < n; i++)
            arr[i] = rand() % 100000;
    }

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    quicksort(arr, 0, (int)n - 1);

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    double t = elapsed_seconds(t_start, t_end);
    int ok = is_sorted(arr, (size_t)n);

    printf("QuickSort Sequential\n");
    printf("n = %" PRId64 "\n", n);
    printf("pattern = %s\n", pattern);
    printf("time = %.6f s\n", t);
    printf("sorted = %s\n\n", ok ? "yes" : "no");

    FILE *fp = fopen("results_seq.csv", "a");
    if (!fp) {
        fprintf(stderr, "Error opening results_seq.csv\n");
        free(arr);
        return EXIT_FAILURE;
    }

    if (ftell(fp) == 0)
        fprintf(fp, "n,pattern,time,sorted\n");

    fprintf(fp, "%" PRId64 ",%s,%.6f,%s\n",
            n, pattern, t, ok ? "yes" : "no");

    fclose(fp);
    free(arr);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
