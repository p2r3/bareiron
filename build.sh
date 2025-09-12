#!/usr/bin/env bash

if [ ! -f "include/registries.h" ]; then
  echo "Error: 'include/registries.h' is missing."
  echo "Please follow the 'Compilation' section of the README to generate it."
  exit 1
fi

rm bareiron
gcc src/*.c -O3 -Iinclude -o bareiron
./bareiron
