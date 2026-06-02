#!/bin/bash

test_files=(tests/language_tests/fail/*.ee)
total_tests=${#test_files[@]}
ran_tests=0

for test_file in "${test_files[@]}"; do
  ./scripts/language_single.sh "$test_file" || exit 1
  ran_tests=$((ran_tests + 1))
done

echo "$ran_tests/$total_tests failing language tests passed"
