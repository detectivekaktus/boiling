#!/usr/bin/env bash
CC=gcc
CFLAGS=("-Wall -Werror -std=c99 -pedantic")

SOURCES=("src/main.c")

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

case $1 in
  build)
    build
    ;;
  debug)
    CFLAGS+=("-ggdb")
    build
    ;;
  *)
    build ...
    ;;
esac
