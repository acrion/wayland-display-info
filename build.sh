#!/usr/bin/env bash
set -e

gcc  -c zwlr-output-management-unstable-v1-protocol.c -o zwlr_output_management.o

g++ -std=c++17 -O2 wayland-display-info.cpp \
    zwlr_output_management.o \
    -o wayland-display-info $(pkg-config --cflags --libs wayland-client)
