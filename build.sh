#!/usr/bin/env bash
CC=gcc
CFLAGS=("-Wall -Wextra -Werror -std=c99 -pedantic -ggdb3")

SOURCES=("main.c")

OBJECTS=("bin/main.o")

TARGET=boiling
BUILD_DIR=bin/

build() {
  mkdir -p $BUILD_DIR 
  for ((i = 0; i < "${#SOURCES[@]}"; i++)); do
    $CC $CFLAGS -c "${SOURCES[i]}" -o "${OBJECTS[i]}"
  done
  $CC $CFLAGS $OBJECTS -o $TARGET
}

debug() {
  build
}

case $1 in
  build)
    build
    ;;
  debug)
    debug
    ;;
  *)
    if [ "$1" = "" ]; then
      debug
    else
      echo "Unknown command ${1}"
    fi
    ;;
esac
