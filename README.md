# Parallel Sorting Algorithms – HPC Project

Este proyecto implementa y compara algoritmos de ordenamiento en contextos secuenciales y paralelos utilizando **C**, **MPI** y, posteriormente, **OpenMP**.  
Cada algoritmo acepta diferentes patrones de entrada para evaluar su comportamiento bajo distintos escenarios.

---

## Algoritmos Implementados

### 1. Quicksort Secuencial

- Implementado en C.
- Utiliza partición aleatoria para evitar el peor caso.
- Patrones soportados:
  - `random` (por defecto)
  - `quasi`
  - `desc`

---

### 2. Parallel Merge Sort (MPI)

- Distribución irregular usando `MPI_Scatterv`.
- Ordenamiento local con `qsort`.
- Combinación jerárquica (tree-based merge).
- Patrones soportados:
  - `random`
  - `quasi`
  - `desc`

---

### 3. Bitonic Sort (MPI)

- Requiere que el número de procesos sea **potencia de 2** (2, 4, 8, …).
- Comunicación entre procesos con `MPI_Sendrecv`.
- Patrones soportados:
  - `random`
  - `quasi`
  - `desc`

---

### 4. Bucket Sort (MPI)

- Distribución irregular (con `MPI_Scatterv`).
- Min/max global para definir rangos.
- Redistribución con `MPI_Alltoallv`.
- Patrones soportados:
  - `random`
  - `quasi`
  - `desc`

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

---

## Ejecución (n = 1,000,000)

Todos los programas aceptan la misma interfaz:

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

### MPI Bitonic Sort (4 procesos, válido)

```
mpirun -np 4 ./mpi_bitonicsort 1000000
```

### MPI Bucket Sort (4 procesos)

```
mpirun -np 4 ./mpi_bucketsort 1000000
```

---

## Resultados y Reportes

Cada ejecución imprime:

- `n`
- `pattern`
- `processes` (solo MPI)
- `time`
- `sorted`

Y genera los siguientes archivos CSV:

- `results_seq.csv`
- `results_mpi_mergesort.csv`
- `results_mpi_bitonic.csv`
- `results_mpi_bucketsort.csv`

Estos se utilizarán más adelante para graficar:

- Execution Time
- Speedup
- Efficiency
- Scalability
