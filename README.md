# Parallel Sorting Algorithms – HPC Project

Este proyecto implementa y compara algoritmos de ordenamiento en contextos secuenciales y paralelos utilizando **C**, **MPI** y **OpenMP**.  
Cada algoritmo acepta diferentes patrones de entrada para evaluar su comportamiento bajo distintos escenarios.

---

## Algoritmos Implementados

### 1. Quicksort Secuencial

- Implementado en C.
- Utiliza partición aleatoria para evitar el peor caso.
- Patrones soportados: `random`, `quasi`, `desc`.

### 2. Parallel Merge Sort (MPI)

- Distribución irregular con `MPI_Scatterv`.
- Orden local con `qsort`.
- Combinación jerárquica tipo árbol.
- Patrones soportados: `random`, `quasi`, `desc`.

### 3. Bitonic Sort (MPI)

- Requiere número de procesos potencia de 2.
- Comunicación con `MPI_Sendrecv`.
- Patrones soportados: `random`, `quasi`, `desc`.

### 4. Bucket Sort (MPI)

- Distribución irregular (con `MPI_Scatterv`).
- Obtiene min/max global.
- Redistribuye con `MPI_Alltoallv`.
- Patrones soportados: `random`, `quasi`, `desc`.

### 5. OpenMP Quicksort

- Paralelización basada en tareas (`#pragma omp task`).
- Patrones soportados: `random`, `quasi`, `desc`.

---

## Compilación

Asegúrate de tener instalado `gcc`, `mpicc` y OpenMPI.

### Quicksort Secuencial

```
gcc -O2 quicksort_seq.c -o quicksort_seq
```

### MPI MergeSort

```
mpicc -O2 mpi_mergesort.c -o mpi_mergesort
```

### MPI Bitonic Sort

```
mpicc -O2 mpi_bitonicsort.c -o mpi_bitonicsort
```

### MPI Bucket Sort

```
mpicc -O2 mpi_bucketsort.c -o mpi_bucketsort
```

### OpenMP QuickSort

```
gcc -O2 -fopenmp omp_quicksort.c -o omp_quicksort
```

---

## Ejecución (n = 1,000,000)

Todos usan la interfaz:

```
<n> [pattern=random|quasi|desc] [seed]
```

Si no se especifica `pattern`, el valor por defecto es **random**.

### Quicksort Secuencial

```
./quicksort_seq 1000000
```

### MPI MergeSort (4 procesos)

```
mpirun -np 4 ./mpi_mergesort 1000000
```

### MPI Bitonic Sort (4 procesos)

```
mpirun -np 4 ./mpi_bitonicsort 1000000
```

### MPI Bucket Sort (4 procesos)

```
mpirun -np 4 ./mpi_bucketsort 1000000
```

### OpenMP QuickSort (4 hilos)

```
OMP_NUM_THREADS=4 ./omp_quicksort 1000000
```

---

## Resultados y CSV

Cada algoritmo genera un archivo:

- `results_seq.csv`
- `results_mpi_mergesort.csv`
- `results_mpi_bitonic.csv`
- `results_mpi_bucketsort.csv`
- `results_omp.csv`

Con columnas:

- Secuencial: `n,pattern,time,sorted`
- MPI: `n,pattern,time,sorted,processes`
- OpenMP: `n,pattern,time,sorted,threads`

---

## Visualización y Análisis

El notebook:

```
charts.ipynb
```

genera gráficos de:

- Tiempo de ejecución
- Speedup
- Eficiencia
- Escalabilidad
- Comparaciones entre Seq, MPI y OpenMP

---

## Estructura del Proyecto

```
HPC_Project/
├── quicksort_seq.c
├── mpi_mergesort.c
├── mpi_bitonicsort.c
├── mpi_bucketsort.c
├── omp_quicksort.c
│
├── quicksort_seq
├── mpi_mergesort
├── mpi_bitonicsort
├── mpi_bucketsort
├── omp_quicksort
│
├── run_all.sh
├── charts.ipynb
│
├── results_seq.csv
├── results_mpi_mergesort.csv
├── results_mpi_bitonic.csv
├── results_mpi_bucketsort.csv
├── results_omp.csv
│
└── README.md
```
