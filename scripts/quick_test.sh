#!/bin/bash
# Quick test script for LTLf synthesis
# Usage: ./scripts/quick_test.sh "<formula>" "<inputs>" "<outputs>"

# Don't exit on grep failures
set +e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
COSY_BIN="/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/Cosy"

FORMULA="$1"
INPUTS="$2"
OUTPUTS="$3"

if [ -z "$FORMULA" ]; then
    echo "Usage: $0 \"<formula>\" \"<inputs>\" \"<outputs>\""
    echo ""
    echo "Example:"
    echo "  $0 \"X(p5)\" \"p5\" \"p6\""
    echo "  $0 \"p5 & F(p8)\" \"p5\" \"p8\""
    exit 1
fi

# Default values if not provided
INPUTS="${INPUTS:-}"
OUTPUTS="${OUTPUTS:-}"

# Create temp files
TMP_DIR=$(mktemp -d)
LTLF_FILE="$TMP_DIR/formula.ltlf"
PART_FILE="$TMP_DIR/formula.part"

# Write formula
echo "$FORMULA" > "$LTLF_FILE"

# Write partition (IMPORTANT: Cosy requires .inputs BEFORE .outputs)
# Also, Cosy requires .inputs: line even if empty
if [ -n "$INPUTS" ] || [ -n "$OUTPUTS" ]; then
    echo ".inputs: $INPUTS" >> "$PART_FILE"
    echo ".outputs: $OUTPUTS" >> "$PART_FILE"
fi

# Run CosyZeroRewrite
echo "=========================================="
echo "CosyZeroRewrite Result:"
echo "=========================================="
COSYZERO_OUTPUT=$($BUILD_DIR/Cosy2 -f "$LTLF_FILE" -p "$PART_FILE" 2>&1)
echo "$COSYZERO_OUTPUT" | grep -E "Formula:|REALIZABLE|UNREALIZABLE" || true

# Run Cosy reference if available
if [ -f "$COSY_BIN" ]; then
    echo ""
    echo "=========================================="
    echo "Cosy Reference Result:"
    echo "=========================================="
    COSY_OUTPUT=$($COSY_BIN "$LTLF_FILE" "$PART_FILE" 0 2>&1)
    echo "$COSY_OUTPUT" | grep -iE "Realizable|Unrealizable" || true
else
    echo ""
    echo "(Cosy reference not available)"
fi

# Cleanup
rm -rf "$TMP_DIR"

# Summary
echo ""
echo "=========================================="
echo "Summary:"
echo "=========================================="
echo "Formula: $FORMULA"
echo "Inputs: [$INPUTS]"
echo "Outputs: [$OUTPUTS]"

if [ -f "$COSY_BIN" ]; then
    # Extract results for comparison
    CZ_RESULT=$(echo "$COSYZERO_OUTPUT" | grep -o "REALIZABLE\|UNREALIZABLE" | head -1)
    COSY_RESULT=$(echo "$COSY_OUTPUT" | grep -oE "Realizable|Unrealizable" | head -1 | tr '[:lower:]' '[:upper:]')

    if [ -z "$CZ_RESULT" ]; then CZ_RESULT="ERROR"; fi
    if [ -z "$COSY_RESULT" ]; then COSY_RESULT="ERROR"; fi

    echo "CosyZero: $CZ_RESULT"
    echo "Cosy Ref: $COSY_RESULT"

    if [ "$CZ_RESULT" = "$COSY_RESULT" ]; then
        echo "Status: ✓ MATCH"
    else
        echo "Status: ✗ MISMATCH"
    fi
fi
