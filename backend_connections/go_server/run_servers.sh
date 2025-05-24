#!/usr/bin/env bash
set -e

BINARY="./go_srv"

for port in {8080..8087}; do
  echo "Starting server on port $port"
    "$BINARY" --port "$port" &
    done

    echo
    echo "All servers launched. PIDs:"
    ps -f | grep "$(basename $BINARY)" | grep -v grep

