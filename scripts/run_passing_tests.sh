#!/bin/bash

test_files=(tests/pass/*.ee)
total_tests=${#test_files[@]}
ran_tests=0

for test_file in "${test_files[@]}"; do
  ./scripts/run_single_test.sh "$test_file" || exit 1
  ran_tests=$((ran_tests + 1))
done

echo "$ran_tests/$total_tests passing tests passed"
