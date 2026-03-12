#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULTS_DIR="${1:-$ROOT_DIR/test-results}"
BUILD_ROOT="${2:-$ROOT_DIR/build-tests}"

mkdir -p "$RESULTS_DIR" "$BUILD_ROOT"

mapfile -t test_pro_files < <(find "$ROOT_DIR/tests" -mindepth 2 -maxdepth 2 -name '*_test.pro' | sort)

if [ "${#test_pro_files[@]}" -eq 0 ]; then
  echo "No Qt test project (*.pro) found under tests/."
  exit 1
fi

echo "Discovered ${#test_pro_files[@]} Qt test projects."

for pro_file in "${test_pro_files[@]}"; do
  rel_pro="${pro_file#$ROOT_DIR/}"
  test_name="$(basename "${pro_file%.pro}")"
  build_dir="$BUILD_ROOT/$test_name"

  target_name="$(sed -n 's/^TARGET[[:space:]]*=[[:space:]]*//p' "$pro_file" | head -n 1 | tr -d '\r')"
  if [ -z "$target_name" ]; then
    target_name="$test_name"
  fi

  echo "\n=== Building $rel_pro (target: $target_name) ==="
  rm -rf "$build_dir"
  mkdir -p "$build_dir"

  (
    cd "$build_dir"
    qmake6 "$pro_file"
    make -j"$(nproc)"

    if [ ! -x "./$target_name" ]; then
      echo "Expected test executable ./$target_name was not generated in $build_dir."
      exit 1
    fi

    echo "--- Running $target_name ---"
    xvfb-run -a "./$target_name" \
      -o "$RESULTS_DIR/${target_name}.txt,txt" \
      -o "$RESULTS_DIR/${target_name}.xml,junitxml"
  )
done

echo "\nAll Qt tests completed. Results are available in: $RESULTS_DIR"
