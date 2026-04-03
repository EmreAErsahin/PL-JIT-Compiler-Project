#!/bin/bash

temporary_error_file=""

cleanup() {
  if [ -n "$temporary_error_file" ]; then
    rm -f "$temporary_error_file"
    temporary_error_file=""
  fi
}

trap cleanup EXIT

quiet=false
if [ "$#" -eq 1 ] && [ "$1" = "--quiet" ]; then
  quiet=true
fi

test_files=(tests/fail/*.ee)
total_tests=${#test_files[@]}
ran_tests=0

for test_file in "${test_files[@]}"; do
  ran_tests=$((ran_tests + 1))
  expected_error_file="${test_file%.ee}.err"

  if [ "$quiet" = false ]; then
    echo "Failing tests: $ran_tests/$total_tests"
  fi

  cleanup
  temporary_error_file="${test_file%.ee}.tmp.err"

  if ./build/interpreter "$test_file" 2> "$temporary_error_file"; then
    exit 1
  fi

  if ! diff "$temporary_error_file" "$expected_error_file" > /dev/null; then
    exit 1
  fi

  cleanup
done

if [ "$quiet" = false ]; then
  echo "All failing tests passed!"
fi
