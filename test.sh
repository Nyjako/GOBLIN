#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$ROOT_DIR/tests"
BUILD_DIR="$ROOT_DIR/build/tests"

mkdir -p "$BUILD_DIR"

for src in "$TEST_DIR"/*.c; do
    [ -e "$src" ] || continue

    name="$(basename "$src" .c)"
    exe="$BUILD_DIR/$name"

    echo "==> Building $name"
    cc -Wall -Wextra -pedantic -std=c99 -I"$ROOT_DIR" "$src" -o "$exe"

    echo "==> Running $name"
    "$exe"
done

echo "All tests passed."