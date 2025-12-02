#!/bin/bash

set -e

echo "=== Compiling programs ==="

gcc -O2 quicksort_seq.c -o quicksort_seq
mpicc -O2 mpi_mergesort.c   -o mpi_mergesort
mpicc -O2 mpi_bitonicsort.c -o mpi_bitonicsort
mpicc -O2 mpi_bucketsort.c  -o mpi_bucketsort
gcc -O2 -fopenmp omp_quicksort.c -o omp_quicksort

echo "=== Deleting previous CSV files ==="
rm -f results_seq.csv \
      results_mpi_mergesort.csv \
      results_mpi_bitonic.csv \
      results_mpi_bucketsort.csv \
      results_omp_quicksort.csv

# Tamaños de problema y patrones a probar
NS=("1000000" "2000000" "3000000" "4000000" "5000000" "6000000" "7000000" "8000000" "9000000" "10000000" "20000000")
PATTERNS=("random" "quasi")

# procesos para todos los algoritmos MPI
MPI_PROCS=("1" "2" "4" "8")

# Hilos para OpenMP
OMP_THREADS=("1" "2" "4" "8")

echo
echo "=== 1) Sequential QuickSort ==="
for n in "${NS[@]}"; do
  for pat in "${PATTERNS[@]}"; do
    echo "Seq: n=${n}, pattern=${pat}"
    ./quicksort_seq "$n" "$pat"
  done
done

echo
echo "=== 2) MPI MergeSort ==="
for p in "${MPI_PROCS[@]}"; do
  for n in "${NS[@]}"; do
    for pat in "${PATTERNS[@]}"; do
      echo "MPI MergeSort: procs=${p}, n=${n}, pattern=${pat}"
      mpirun -np "$p" ./mpi_mergesort "$n" "$pat"
    done
  done
done

echo
echo "=== 3) MPI Bitonic Sort ==="
for p in "${MPI_PROCS[@]}"; do
  for n in "${NS[@]}"; do
    for pat in "${PATTERNS[@]}"; do
      echo "MPI Bitonic: procs=${p}, n=${n}, pattern=${pat}"
      mpirun -np "$p" ./mpi_bitonicsort "$n" "$pat"
    done
  done
done

echo
echo "=== 4) MPI Bucket Sort ==="
for p in "${MPI_PROCS[@]}"; do
  for n in "${NS[@]}"; do
    for pat in "${PATTERNS[@]}"; do
      echo "MPI Bucket: procs=${p}, n=${n}, pattern=${pat}"
      mpirun -np "$p" ./mpi_bucketsort "$n" "$pat"
    done
  done
done

echo
echo "=== 5) OpenMP QuickSort ==="
for t in "${OMP_THREADS[@]}"; do
  export OMP_NUM_THREADS="$t"
  for n in "${NS[@]}"; do
    for pat in "${PATTERNS[@]}"; do
      echo "OpenMP QuickSort: threads=${t}, n=${n}, pattern=${pat}"
      ./omp_quicksort "$n" "$pat"
    done
  done
done

echo
echo "=== Experiments completed ==="
echo "CSV files generated:"
ls -1 results_*.csv
