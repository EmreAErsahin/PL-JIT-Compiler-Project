#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 TEST_FILE"
  exit 1
fi

./build/interpreter/interpreter --debug "$1"
