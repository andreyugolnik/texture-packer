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

    # Invariant (checked even when updating): every input sprite is packed
    # exactly once and all sprite ids are unique.
    local n_input n_packed n_unique
    n_input=$(find "$SPRITES" -type f -name '*.png' | wc -l | tr -d ' ')
    n_packed=$(grep -c 'texture=' "$xml" || true)
    n_unique=$(grep 'texture=' "$xml" | grep -oE '<[^ ]+' | sort -u | wc -l | tr -d ' ')
    if [ "$n_packed" -ne "$n_input" ]; then
        echo "  FAIL  $name (packed $n_packed of $n_input sprites)"
        failed=$((failed + 1))
        return
    fi
    if [ "$n_unique" -ne "$n_packed" ]; then
        echo "  FAIL  $name (duplicate sprite ids)"
        failed=$((failed + 1))
        return
    fi

    if $UPDATE; then
        for f in "$OUTPUT"/$name*.png; do
            [ -f "$f" ] && cp "$f" "$REFERENCE/"
        done
        normalize_xml "$xml" > "$REFERENCE/$name.xml"
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

# Assert texpacker exits with a specific status (error-path contracts).
expect_exit() {
    local name="$1" expected="$2"
    shift 2
    total=$((total + 1))
    local code=0
    (cd "$VERIFY_DIR" && "$TEXPACKER" "$@") >/dev/null 2>&1 || code=$?
    if [ "$code" -eq "$expected" ]; then
        echo "  OK    $name (exit $code)"
        passed=$((passed + 1))
    else
        echo "  FAIL  $name (exit $code, expected $expected)"
        failed=$((failed + 1))
    fi
}

# Smoke-test an output format: the atlas is created, non-empty, right magic.
check_format() {
    local name="$1" ext="$2"
    total=$((total + 1))
    local atlas="$OUTPUT/$name.$ext"
    if ! (cd "$VERIFY_DIR" && "$TEXPACKER" sprites --atlas="$atlas") >/dev/null 2>&1 || [ ! -s "$atlas" ]; then
        echo "  FAIL  $name (no $ext output)"
        failed=$((failed + 1))
        return
    fi
    if [ "$ext" = "bmp" ] && [ "$(head -c 2 "$atlas")" != "BM" ]; then
        echo "  FAIL  $name (bad bmp magic)"
        failed=$((failed + 1))
        return
    fi
    echo "  OK    $name ($ext)"
    passed=$((passed + 1))
}

# --trim-sprite shrinks a transparent-bordered sprite to its opaque bounds.
check_trim() {
    total=$((total + 1))
    local u="$OUTPUT/untrim.xml" t="$OUTPUT/trim.xml"
    (cd "$VERIFY_DIR" && "$TEXPACKER" sprites-trim --atlas="$OUTPUT/u.png" --xml="$u") >/dev/null 2>&1
    (cd "$VERIFY_DIR" && "$TEXPACKER" sprites-trim --atlas="$OUTPUT/t.png" --xml="$t" --trim-sprite) >/dev/null 2>&1
    if grep -q 'rect="1 1 20 20"' "$u" && grep -q 'rect="1 1 10 10"' "$t"; then
        echo "  OK    trimsprite (20x20 -> 10x10)"
        passed=$((passed + 1))
    else
        echo "  FAIL  trimsprite (untrimmed/trimmed rects wrong)"
        failed=$((failed + 1))
    fi
}

# --trim-id strips leading path characters from every sprite id.
check_trimid() {
    total=$((total + 1))
    local x="$OUTPUT/trimid.xml"
    (cd "$VERIFY_DIR" && "$TEXPACKER" --trim-id=8 sprites --atlas="$OUTPUT/ti.png" --xml="$x") >/dev/null 2>&1
    if grep -q '<background ' "$x" && ! grep -q 'sprites_' "$x"; then
        echo "  OK    trimid"
        passed=$((passed + 1))
    else
        echo "  FAIL  trimid (ids not trimmed)"
        failed=$((failed + 1))
    fi
}

# Assert every sprite is packed and its rect lies fully inside the atlas.
# Guards the KDTree border-underflow and the right/bottom trim off-by-one:
# both produced an atlas smaller than the rects referencing it. Single-atlas
# only (all rects share one texture).
png_dim() { # file byte-offset -> big-endian uint32
    od -An -tu1 -j"$2" -N4 "$1" | awk '{for(i=1;i<=NF;i++)v=v*256+$i} END{print v+0}'
}
check_bounds() {
    local name="$1" dir="$2"
    shift 2
    total=$((total + 1))

    local atlas="$OUTPUT/$name.png" xml="$OUTPUT/$name.xml"
    if ! (cd "$VERIFY_DIR" && "$TEXPACKER" "$dir" --atlas="$atlas" --xml="$xml" "$@") >/dev/null 2>&1; then
        echo "  FAIL  $name (texpacker exited with error)"
        failed=$((failed + 1))
        return
    fi

    local n_input n_packed
    n_input=$(find "$VERIFY_DIR/$dir" -type f -name '*.png' | wc -l | tr -d ' ')
    n_packed=$(grep -c 'texture=' "$xml" || true)
    if [ "$n_packed" -ne "$n_input" ]; then
        echo "  FAIL  $name (packed $n_packed of $n_input sprites)"
        failed=$((failed + 1))
        return
    fi

    local aw ah rects bad=0
    aw=$(png_dim "$atlas" 16)
    ah=$(png_dim "$atlas" 20)
    rects=$(grep -oE 'rect="[0-9]+ [0-9]+ [0-9]+ [0-9]+"' "$xml" \
        | sed -E 's/rect="([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)"/\1 \2 \3 \4/' || true)
    while read -r l t w h; do
        [ -n "$l" ] || continue
        if [ "$((l + w))" -gt "$aw" ] || [ "$((t + h))" -gt "$ah" ]; then
            bad=1
        fi
    done <<< "$rects"

    if [ "$bad" -eq 0 ]; then
        echo "  OK    $name (rects within ${aw}x${ah})"
        passed=$((passed + 1))
    else
        echo "  FAIL  $name (rect exceeds atlas ${aw}x${ah})"
        failed=$((failed + 1))
    fi
}

# The texture attribute value is XML-escaped, so a path with & < > " stays valid.
check_xml_escape() {
    total=$((total + 1))
    local xml="$OUTPUT/esc.xml"
    (cd "$VERIFY_DIR" && "$TEXPACKER" sprites --atlas="$OUTPUT/a&b.png" --xml="$xml") >/dev/null 2>&1 || true
    if [ -f "$xml" ] && grep -q 'texture="[^"]*&amp;' "$xml"; then
        echo "  OK    xmlescape (& -> &amp;)"
        passed=$((passed + 1))
    else
        echo "  FAIL  xmlescape (texture value not escaped)"
        failed=$((failed + 1))
    fi
}

# The success log must report the actual saved atlas size, not the pre-trim one.
check_reported_size() {
    total=$((total + 1))
    local atlas="$OUTPUT/rep.png" out rep
    out=$( (cd "$VERIFY_DIR" && "$TEXPACKER" sprites --border=64 --atlas="$atlas") 2>/dev/null || true )
    rep=$(echo "$out" | grep -oE '\([0-9]+ x [0-9]+,' | grep -oE '[0-9]+ x [0-9]+' | head -1 || true)
    local rep_w="${rep% x *}" rep_h="${rep#* x }"
    local real_w real_h
    real_w=$(png_dim "$atlas" 16)
    real_h=$(png_dim "$atlas" 20)
    if [ -n "$rep" ] && [ "$rep_w" = "$real_w" ] && [ "$rep_h" = "$real_h" ]; then
        echo "  OK    reportedsize (${real_w}x${real_h})"
        passed=$((passed + 1))
    else
        echo "  FAIL  reportedsize (reported '${rep}', actual ${real_w}x${real_h})"
        failed=$((failed + 1))
    fi
}

# A fully transparent atlas (all source sprites empty) warns but still succeeds.
check_transparent_warn() {
    total=$((total + 1))
    local out atlas="$OUTPUT/tw.png"
    out=$( (cd "$VERIFY_DIR" && "$TEXPACKER" sprites-empty --atlas="$atlas") 2>&1 || true )
    if echo "$out" | grep -qi 'transparent' && [ -s "$atlas" ]; then
        echo "  OK    transparentwarn"
        passed=$((passed + 1))
    else
        echo "  FAIL  transparentwarn (missing warning or output)"
        failed=$((failed + 1))
    fi
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
run_test multi      --multi-atlas --atlas-size=128
run_test pot        --pot
run_test bordered   --border=2 --padding=2
run_test overlay    --overlay
run_test kdtree     --algorithm=kdtree

if ! $UPDATE; then
    # Output formats (smoke: created, non-empty, valid magic)
    check_format tga tga
    check_format bmp bmp

    # Error-path contracts (exit code; -1 from main becomes 255)
    mkdir -p "$OUTPUT/emptydir"
    expect_exit noatlas   255 sprites --xml="$OUTPUT/na.xml"
    expect_exit emptydir  255 "$OUTPUT/emptydir" --atlas="$OUTPUT/e.png"
    expect_exit nopath    255 /no/such/path --atlas="$OUTPUT/np.png"
    expect_exit badnumber 255 sprites --atlas="$OUTPUT/bn.png" --padding=abc
    expect_exit zerosize  255 sprites --atlas="$OUTPUT/zs.png" --atlas-size=0
    expect_exit oversized 255 sprites --atlas="$OUTPUT/ov.png" --atlas-size=8
    expect_exit autoalg   0   sprites --atlas="$OUTPUT/au.png" --algorithm=auto

    # Feature behaviors
    check_trim
    check_trimid

    # Regression: rects must stay inside the atlas (border underflow / trim off-by-one)
    check_bounds kdtreeborder sprites-border --algorithm=kdtree --border=8
    check_bounds autoborder   sprites-border --algorithm=auto   --border=8
    check_bounds padbounds    sprites-pad    --padding=0

    # Regression: XML escaping and accurate reported atlas size
    check_xml_escape
    check_reported_size
    check_transparent_warn
fi

echo ""
if $UPDATE; then
    echo "Reference files updated ($total test cases)."
else
    echo "Results: $passed passed, $failed failed out of $total tests."
    if [ "$failed" -gt 0 ]; then
        exit 1
    fi
fi
