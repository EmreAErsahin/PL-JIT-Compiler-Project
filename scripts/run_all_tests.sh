#!/bin/bash

temporary_error_file=""

cleanup() {
  if [ -n "$temporary_error_file" ]; then
    rm -f "$temporary_error_file"
    temporary_error_file=""
  fi
}

trap cleanup EXIT

passing_test_files=(tests/pass/*.ee)
failing_test_files=(tests/fail/*.ee)
total_tests=$((${#passing_test_files[@]} + ${#failing_test_files[@]}))
ran_tests=0

echo "Running passing tests..."
for test_file in "${passing_test_files[@]}"; do
  ./scripts/run_single_test.sh "$test_file" > /dev/null || exit 1
  ran_tests=$((ran_tests + 1))
  echo "All tests: $ran_tests/$total_tests"
done

echo "Running failing tests..."
for test_file in "${failing_test_files[@]}"; do
  expected_error_file="${test_file%.ee}.err"

  cleanup
  temporary_error_file="${test_file%.ee}.tmp.err"

  if ./build/interpreter "$test_file" 2> "$temporary_error_file"; then
    exit 1
  fi

  if ! diff "$temporary_error_file" "$expected_error_file" > /dev/null; then
    exit 1
  fi

  cleanup
  ran_tests=$((ran_tests + 1))
  echo "All tests: $ran_tests/$total_tests"
done

echo "All tests passed!"
