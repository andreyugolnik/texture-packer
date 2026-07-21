#!/bin/bash

# Verification test for texpacker.
# Runs texpacker with different options and compares output
# against committed reference files (atlas PNGs + XML).
#
# Usage:
#   ./tests/verify.sh              Run tests
#   ./tests/verify.sh --update     Regenerate reference files

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
VERIFY_DIR="$SCRIPT_DIR/verify"
SPRITES="$VERIFY_DIR/sprites"
REFERENCE="$VERIFY_DIR/reference"

# Resolve texpacker binary: next to Makefile, two levels up from VERIFY_DIR
ROOT_DIR="$(cd "$VERIFY_DIR/../.." && pwd)"
TEXPACKER="$ROOT_DIR/texpacker"

if [ ! -x "$TEXPACKER" ]; then
    echo "Error: $TEXPACKER not found. Run 'make release' first."
    exit 1
fi

UPDATE=false
if [[ "${1:-}" == "--update" ]]; then
    UPDATE=true
fi

OUTPUT=$(mktemp -d)
trap 'rm -rf "$OUTPUT"' EXIT

passed=0
failed=0
total=0

# Normalize XML for comparison: strip directory from texture= attribute,
# leaving only the filename. This makes comparison path-independent.
normalize_xml() {
    sed 's|texture="[^"]*/|texture="|g' "$1"
}

run_test() {
    local name="$1"
    shift
    total=$((total + 1))

    local atlas="$OUTPUT/$name.png"
    local xml="$OUTPUT/$name.xml"

    # Run texpacker from verify dir so sprite IDs are consistent (sprites/...)
    if ! (cd "$VERIFY_DIR" && "$TEXPACKER" sprites --atlas="$atlas" --xml="$xml" "$@") >/dev/null 2>&1; then
        echo "  FAIL  $name (texpacker exited with error)"
        failed=$((failed + 1))
        return
    fi

    if $UPDATE; then
        for f in "$OUTPUT"/$name*.png; do
            [ -f "$f" ] && cp "$f" "$REFERENCE/"
        done
        cp "$xml" "$REFERENCE/"
        echo "  UPDATED  $name"
        return
    fi

    # --- Compare atlas PNGs ---
    local ref_count=0
    for ref_png in "$REFERENCE"/$name*.png; do
        [ -f "$ref_png" ] || continue
        ref_count=$((ref_count + 1))
        local base
        base=$(basename "$ref_png")
        if [ ! -f "$OUTPUT/$base" ]; then
            echo "  FAIL  $name (missing $base)"
            failed=$((failed + 1))
            return
        fi
        if ! cmp -s "$ref_png" "$OUTPUT/$base"; then
            echo "  FAIL  $name ($base differs)"
            failed=$((failed + 1))
            return
        fi
    done

    # Count output PNGs to catch unexpected extra atlases
    local out_count=0
    for out_png in "$OUTPUT"/$name*.png; do
        [ -f "$out_png" ] || continue
        out_count=$((out_count + 1))
    done
    if [ "$ref_count" -ne "$out_count" ]; then
        echo "  FAIL  $name (expected $ref_count atlas(es), got $out_count)"
        failed=$((failed + 1))
        return
    fi

    # --- Compare XML ---
    local ref_xml="$REFERENCE/$name.xml"
    if [ ! -f "$ref_xml" ]; then
        echo "  FAIL  $name (missing reference XML)"
        failed=$((failed + 1))
        return
    fi
    if [ ! -f "$xml" ]; then
        echo "  FAIL  $name (missing output XML)"
        failed=$((failed + 1))
        return
    fi

    local norm_ref norm_out
    norm_ref=$(normalize_xml "$ref_xml")
    norm_out=$(normalize_xml "$xml")
    if [ "$norm_ref" != "$norm_out" ]; then
        echo "  FAIL  $name (XML differs)"
        diff <(echo "$norm_ref") <(echo "$norm_out") || true
        failed=$((failed + 1))
        return
    fi

    echo "  OK    $name"
    passed=$((passed + 1))
}

if $UPDATE; then
    echo "Updating reference files..."
else
    echo "Running verification tests..."
fi
echo ""

# Test cases
run_test single
run_test anchoronly --anchor-only
run_test keepfloat  --keep-float
run_test classic   --algorithm=classic
run_test multi     --multi-atlas --atlas-size=128
run_test pot       --pot
run_test bordered  --border=2 --padding=2

echo ""
if $UPDATE; then
    echo "Reference files updated ($total test cases)."
else
    echo "Results: $passed passed, $failed failed out of $total tests."
    if [ "$failed" -gt 0 ]; then
        exit 1
    fi
fi
