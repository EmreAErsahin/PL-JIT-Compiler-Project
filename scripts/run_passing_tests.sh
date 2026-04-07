#!/bin/bash

cleanup() {
  :
}

trap cleanup EXIT

quiet=false
if [ "$#" -eq 1 ] && [ "$1" = "--quiet" ]; then
  quiet=true
fi

test_files=(tests/pass/*.ee)
total_tests=${#test_files[@]}
ran_tests=0

for test_file in "${test_files[@]}"; do
  ran_tests=$((ran_tests + 1))

  if [ "$quiet" = false ]; then
    echo "Passing tests: $ran_tests/$total_tests"
  fi

  ./scripts/run_single_test.sh "$test_file" > /dev/null || exit 1
done

if [ "$quiet" = false ]; then
  echo "All passing tests passed!"
fi
