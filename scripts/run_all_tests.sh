#!/bin/bash

./scripts/run_passing_tests.sh || exit 1
./scripts/run_failing_tests.sh || exit 1

total_tests=$(($(ls tests/pass/*.ee | wc -l) + $(ls tests/fail/*.ee | wc -l)))
echo "$total_tests/$total_tests tests passed"
