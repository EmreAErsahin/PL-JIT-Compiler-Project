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
test_directory="$(dirname "$test_file")"
test_kind="$(basename "$test_directory")"
test_name="$(basename "$test_file" .cpp)"
test_executable="$(pwd)/build/embedding_tests/$test_kind/$test_name"
expected_output_file="${test_file%.cpp}.out"
expected_error_file="${test_file%.cpp}.err"

if [ -f "$expected_output_file" ]; then
  temporary_result_file="${test_file%.cpp}.tmp.out"

  if (cd "$test_directory" && "$test_executable") > "$temporary_result_file"; then
    if diff -u "$expected_output_file" "$temporary_result_file"; then
      echo "$test_name: PASSED"
      exit 0
    fi

    echo "$test_name: FAILED"
    exit 1
  fi

  echo "$test_name: FAILED"
  echo "Expected passing embedding test but executable exited with an error."
  exit 1
fi

if [ -f "$expected_error_file" ]; then
  temporary_result_file="${test_file%.cpp}.tmp.err"

  if (cd "$test_directory" && "$test_executable") 2> "$temporary_result_file"; then
    echo "$test_name: FAILED"
    echo "Expected failing embedding test but executable succeeded."
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
