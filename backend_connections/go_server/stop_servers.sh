#!/usr/bin/env bash
set -e

echo "Looking for go_srv processes on ports 8080–8087…"

# Find any processes matching "go_srv --port"
PIDS=$(pgrep -f "go_srv --port")

if [[ -z "$PIDS" ]]; then
  echo "No go_srv servers found."
    exit 0
    fi

    echo "Killing PIDs: $PIDS"
    kill $PIDS

    echo "Done."

