#!/bin/bash

temporary_output_file=""

cleanup() {
  if [ -n "$temporary_output_file" ]; then
    rm -f "$temporary_output_file"
    temporary_output_file=""
  fi
}

trap cleanup EXIT

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 TEST_FILE"
  exit 1
fi

test_file="$1"
expected_output_file="${test_file%.ee}.out"
temporary_output_file="${test_file%.ee}.tmp.out"

if ./build/interpreter "$test_file" > "$temporary_output_file"; then
  if diff "$temporary_output_file" "$expected_output_file" > /dev/null; then
    echo "Test Passed!"
    exit 0
  fi

  echo "Test Failed!"
  exit 1
else
  echo "Test Failed!"
  exit 1
fi
