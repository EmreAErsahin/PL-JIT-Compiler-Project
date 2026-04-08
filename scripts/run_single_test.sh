#!/bin/bash

temporary_result_file=""

cleanup() {
  if [ -n "$temporary_result_file" ]; then
    rm -f "$temporary_result_file"
    temporary_result_file=""
  fi
}

trap cleanup EXIT

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 TEST_FILE"
  exit 1
fi

test_file="$1"
test_name="$(basename "$test_file" .ee)"
expected_output_file="${test_file%.ee}.out"
expected_error_file="${test_file%.ee}.err"

if [ -f "$expected_output_file" ]; then
  temporary_result_file="${test_file%.ee}.tmp.out"

  if ./build/interpreter "$test_file" > "$temporary_result_file"; then
    if diff -u "$expected_output_file" "$temporary_result_file"; then
      echo "$test_name: PASSED"
      exit 0
    fi

    echo "$test_name: FAILED"
    exit 1
  fi

  echo "$test_name: FAILED"
  echo "Expected passing test but interpreter exited with an error."
  exit 1
fi

if [ -f "$expected_error_file" ]; then
  temporary_result_file="${test_file%.ee}.tmp.err"

  if ./build/interpreter "$test_file" 2> "$temporary_result_file"; then
    echo "$test_name: FAILED"
    echo "Expected failing test but interpreter succeeded."
    exit 1
  fi

  if diff -u "$expected_error_file" "$temporary_result_file"; then
    echo "$test_name: PASSED"
    exit 0
  fi

  echo "$test_name: FAILED"
  exit 1
fi

echo "$test_name: FAILED"
echo "No expected .out or .err file found for $test_file"
exit 1
