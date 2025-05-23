#!/usr/bin/env bash
set -e

g++ -std=c++17 -O2 \
    src/Connection.cpp \
    src/Server.cpp \
    src/main.cpp \
    -Iheaders \
    -o load_balancer \
    -lhttp_parser \
    -pthread

echo "Built 👉 ./load_balancer"

