#!/usr/bin/env bash
set -euo pipefail

# Run a single numbered test through the provided check harness.
# Usage: run-test.sh 25   # will run tests/lexer/025.*.in

usage() {
  echo "Usage: $(basename "$0") <test-number>"
  echo "Example: $(basename "$0") 25   (will run tests/lexer/025.*.in)"
  exit 2
}

if [ "$#" -ne 1 ]; then
  usage
fi

TEST_NO_RAW="$1"

# Accept numbers like 1, 25, 025 and normalize to 3-digit zero-padded form
if ! [[ "$TEST_NO_RAW" =~ ^[0-9]+$ ]]; then
  echo "Error: test number must be a non-negative integer." >&2
  usage
fi

TEST_NO=$(printf "%03d" "$TEST_NO_RAW")

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TEST_GLOB="$ROOT_DIR/tests/lexer/${TEST_NO}.*.in"

# shell globbing: expand matches into array
matches=( $TEST_GLOB )

if [ ${#matches[@]} -eq 0 ]; then
  echo "No test file found for number ${TEST_NO} (glob: ${TEST_GLOB})." >&2
  exit 3
fi

if [ ${#matches[@]} -gt 1 ]; then
  echo "Multiple matches found for test ${TEST_NO}, using the first:"
  for m in "${matches[@]}"; do
    echo "  $m"
  done
  echo
fi

TEST_FILE="${matches[0]}"

echo "Running test ${TEST_NO} -> ${TEST_FILE}"

# Path to built lexer binary (CMake places it in ../build/lexer)
LEXER_BIN="$ROOT_DIR/build/lexer"

if [ -x "$LEXER_BIN" ]; then
  echo "\n--- Lexer output (stdout) ---"
  if ! "$LEXER_BIN" < "$TEST_FILE"; then
    echo "Lexer exited with non-zero status." >&2
  fi
  echo "--- end lexer output ---\n"
else
  echo "Warning: lexer binary not found at ${LEXER_BIN}; skipping direct lexer run." >&2
fi

# Ensure the check harness exists in tools
if [ ! -f "$(dirname "$0")/check" ]; then
  echo "Warning: tools/check not found. You may need to run from the repo root where tools/check exists." >&2
fi

# Run the harness by executing the embedded script from the check archive while providing the test file on stdin.
pushd "$(dirname "$0")" > /dev/null
unzip -p check me | bash --noprofile --norc < "$TEST_FILE"
ret=$?
popd > /dev/null

exit $ret
