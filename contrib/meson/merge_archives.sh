#!/bin/sh
# Helper script to merge static archives
# Usage: merge_archives.sh <output.a> <input1.a> [input2.a ...]

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <output.a> <input1.a> [input2.a ...]"
    exit 1
fi

OUTPUT="$1"
shift

# Save current directory
START_DIR=$(pwd)

# Convert output to absolute path if relative
case "$OUTPUT" in
    /*) ABS_OUTPUT="$OUTPUT" ;;
    *) ABS_OUTPUT="$START_DIR/$OUTPUT" ;;
esac

# Create temp directory
TEMP_DIR=$(mktemp -d)
trap "rm -rf '$TEMP_DIR'" EXIT

# Extract all input archives
cd "$TEMP_DIR"
for lib in "$@"; do
    # Convert relative paths to absolute
    case "$lib" in
        /*) ABS_LIB="$lib" ;;
        *) ABS_LIB="$START_DIR/$lib" ;;
    esac
    ar x "$ABS_LIB"
done

# Create output archive
ar rcs "$ABS_OUTPUT" *.o
