#!/usr/bin/env bash
set -e

cd "$(dirname "$0")"

# CPPFLAGS may add an include directory for doctest.
# shellcheck disable=SC2086
g++ -std=c++17 -O2 ${CPPFLAGS:-} -I. display-metrics.test.cpp -o display-metrics-test
./display-metrics-test
