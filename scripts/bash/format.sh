#!/bin/bash
#
# format.sh - C/C++ code style checker using Artistic Style (AStyle)
#
# Usage:
#   ./scripts/format.sh
#
# Description:
#   Scans the src/ and include/ directories for all *.cpp and *.h files and
#   runs AStyle against them using the project style configuration stored in
#   scripts/.astyle.cfg.
#
#   If any file would be reformatted the script prints the list of offending
#   files, shows the command needed to fix them, restores the originals via
#   'git checkout .' and exits with code 1.
#
#   If all files are already correctly formatted the script exits with code 0.
#
# Requirements:
#   - astyle must be installed and available on PATH
#   - Script must be run from the repository root
#   - scripts/.astyle.cfg must exist
#
# Script to check C/C++ code formatting using astyle

astyle --version
          
# Find all C/C++ files
FILES=$(find src include -type f \( -name "*.cpp" -o -name "*.h" \))
          
if [ -z "$FILES" ]; then
    echo "No C/C++ files found to check"
    exit 0
fi
          
# Run astyle and check if any files would be formatted
echo "$FILES" | xargs astyle --options=scripts/config/.astyle.cfg
          
# Check if any files were modified
if ! git diff --quiet; then
    echo "??  Code style issues found. The following files need formatting:"
    git diff --name-only
    echo ""
    echo "To fix formatting, run:"
    echo "  find src include -type f \( -name '*.cpp' -o -name '*.h' \) | xargs astyle --options=scripts/config/.astyle.cfg"
    git checkout .
    exit 1
else
    echo "? All files are properly formatted!"
fi
