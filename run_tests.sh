#!/usr/bin/env bash
set -euo pipefail

HUNNU="${HUNNU:-./build/hunnu}"
PASS=0
FAIL=0

green() { printf "\033[32m%s\033[0m\n" "$1"; }
red()   { printf "\033[31m%s\033[0m\n" "$1"; }

run_test() {
    local name="$1"
    local cmd="$2"
    local expected="$3"

    if output=$($cmd 2>&1); then
        if [ -n "$expected" ]; then
            if echo "$output" | grep -qF "$expected"; then
                green "  PASS  $name"
                PASS=$((PASS + 1))
            else
                red "  FAIL  $name (expected output not found)"
                echo "    Expected to contain: $expected"
                echo "    Got: $output"
                FAIL=$((FAIL + 1))
            fi
        else
            green "  PASS  $name"
            PASS=$((PASS + 1))
        fi
    else
        red "  FAIL  $name (exit code $?)"
        echo "    Output: $output"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== Hunnu Compiler Test Suite ==="
echo ""

echo "--- Basic Smoke Tests ---"
run_test "version" "$HUNNU -v" "hunnu"

echo ""
echo "--- C Unit Tests ---"
"${HUNNU}_tests" && green "  PASS  C unit tests" || red "  FAIL  C unit tests"

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
exit $FAIL
