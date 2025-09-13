#!/usr/bin/env bash

# Check for registries before attempting to compile, prevents confusion
if [ ! -f "include/registries.h" ]; then
  echo "Error: 'include/registries.h' is missing."
  echo "Please follow the 'Compilation' section of the README to generate it."
  exit 1
fi

# Figure out executable suffix (for MSYS compilation)
case "$OSTYPE" in
  msys*|cygwin*|win32*) exe=".exe" ;;
  *) exe="" ;;
esac

# mingw64-specific linker options
windows_linker=""
unameOut="$(uname -s)"
case "$unameOut" in
  MINGW64_NT*)
    windows_linker="-static -lws2_32 -pthread"
    ;;
esac

rm -f "bareiron$exe"
gcc src/*.c -O3 -Iinclude -o "bareiron$exe" $windows_linker
"./bareiron$exe"
