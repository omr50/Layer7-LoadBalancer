#!/usr/bin/env bash
set -e

# Usage: ./build.sh [--asan]
#
# If --asan is passed, build with AddressSanitizer and name the output lb_asan.
# Otherwise build without sanitizers and name it load_balancer.

# Default: no sanitizer
SAN_FLAGS=""
OUTPUT="load_balancer"

if [[ "${1:-}" == "--asan" ]]; then
  SAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
    OUTPUT="lb_asan"
    fi

    g++ -std=c++17 -O1 -g $SAN_FLAGS \
        src/Connection.cpp src/ConnectionPool.cpp src/Server.cpp src/main.cpp \
            -Iheaders -lhttp_parser -pthread -o "$OUTPUT"

            echo "Built 👉 ./$OUTPUT"

