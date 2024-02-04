#!/bin/bash

for i in {1..100}
do
   echo "Run $i"
   ./thread_test >> ./results/output_thread_test.txt
   ./thread_test_malloc_free >> ./results/output_thread_test_malloc_free.txt
   ./thread_test_malloc_free_change_thread >> ./results/output_thread_test_malloc_free_change_thread.txt
   ./thread_test_measurement >> ./results/output_thread_test_measurement.txt

   ./thread_test_non-lock >> ./results/output_thread_test_non-lock.txt
   ./thread_test_malloc_free_non-lock >> ./results/output_thread_test_malloc_free_non-lock.txt
   ./thread_test_malloc_free_change_thread_non-lock >> ./results/output_thread_test_malloc_free_change_thread_non-lock.txt
   ./thread_test_measurement_non-lock >> ./results/output_thread_test_measurement_non-lock.txt
done