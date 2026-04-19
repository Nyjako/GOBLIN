#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$ROOT_DIR/tests"
BUILD_DIR="$ROOT_DIR/build/tests"

mkdir -p "$BUILD_DIR"

for src in "$TEST_DIR"/*.c; do
    [ -e "$src" ] || continue

    base="$(basename "$src")"
    name="${base%.c}"
    std="c99"

    if [[ "$name" =~ \.c([0-9]+)$ ]]; then
        std="c${BASH_REMATCH[1]}"
        name="${name%.*}"
    fi

    exe="$BUILD_DIR/$name"

    echo "==> Building $name (std=$std)"
    cc -Wall -Wextra -pedantic -std="$std" -I"$ROOT_DIR" "$src" -o "$exe"

    echo "==> Running $name"
    "$exe"
done

echo "All tests passed."