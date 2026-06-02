#!/bin/bash

./scripts/language_passing.sh || exit 1
./scripts/language_failing.sh || exit 1
./scripts/embedding_passing.sh || exit 1
./scripts/embedding_failing.sh || exit 1

total_tests=$(($(ls tests/language_tests/pass/*.ee | wc -l) + $(ls tests/language_tests/fail/*.ee | wc -l) + $(ls tests/embedding_tests/pass/*.cpp | wc -l) + $(ls tests/embedding_tests/fail/*.cpp | wc -l)))
echo "$total_tests/$total_tests tests passed"
