#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

doxygen Doxyfile

echo "Documentation generated in: $ROOT_DIR/doc_output/html/index.html"
