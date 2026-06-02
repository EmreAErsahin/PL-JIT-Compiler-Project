#!/bin/bash

cmake --build build --target embedding_tests || exit 1

test_files=(tests/embedding_tests/pass/*.cpp)
total_tests=${#test_files[@]}
ran_tests=0

for test_file in "${test_files[@]}"; do
  ./scripts/embedding_single.sh "$test_file" || exit 1
  ran_tests=$((ran_tests + 1))
done

echo "$ran_tests/$total_tests passing embedding tests passed"
