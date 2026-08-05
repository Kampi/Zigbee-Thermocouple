#!/usr/bin/env bash
# =============================================================================
# Script: format.sh
# Description: Run AStyle code formatter on C/C++ source files.
#              In dry-run mode (default), reports files that need formatting
#              without modifying them.
# Usage: ./format.sh [--fix]
#   --fix   Apply formatting changes (default is dry-run / check only)
# =============================================================================

set -euo pipefail

DRY_RUN=true

for arg in "$@"; do
    case "$arg" in
        --fix)
            DRY_RUN=false
            ;;
    esac
done

ASTYLE_OPTIONS=(
    "--style=allman"
    "--indent=spaces=4"
    "--indent-switches"
    "--indent-preproc-block"
    "--pad-oper"
    "--pad-header"
    "--unpad-paren"
    "--align-pointer=name"
    "--align-reference=name"
    "--add-braces"
    "--convert-tabs"
    "--max-code-length=120"
    "--lineend=linux"
)

FILES=$(find . \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
    -not -path "./.git/*" \
    -not -path "./build/*" \
    -not -path "./astyle-*/*")

if [ -z "$FILES" ]; then
    echo "No C/C++ source files found."
    exit 0
fi

if [ "$DRY_RUN" = true ]; then
    echo "Running AStyle in dry-run mode (--dry-run)..."
    RESULT=0
    # shellcheck disable=SC2086
    astyle "${ASTYLE_OPTIONS[@]}" --dry-run $FILES | grep -E "Formatted" && RESULT=1 || true
    if [ "$RESULT" -ne 0 ]; then
        echo ""
        echo "The above files need formatting. Run './scripts/bash/format.sh --fix' to apply fixes."
        exit 1
    else
        echo "All files are correctly formatted."
    fi
else
    echo "Running AStyle (applying formatting)..."
    # shellcheck disable=SC2086
    astyle "${ASTYLE_OPTIONS[@]}" $FILES
fi
