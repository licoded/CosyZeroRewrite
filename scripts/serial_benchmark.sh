#!/bin/bash
# Serial benchmark runner - runs one formula at a time to avoid concurrency issues

if [ $# -lt 2 ]; then
    echo "Usage: $0 <benchmark_dir> <count> [start_idx]"
    echo "Example: $0 benchmarks/sm1000 10      # Run 10 formulas starting from 0"
    echo "Example: $0 benchmarks/sm1000 10 50   # Run 10 formulas starting from 50"
    exit 1
fi

BENCHMARK_DIR="$1"
COUNT="$2"
START_IDX="${3:-0}"

END_IDX=$((START_IDX + COUNT))

echo "========================================"
echo "Serial Benchmark Runner"
echo "========================================"
echo "Benchmark directory: $BENCHMARK_DIR"
echo "Range: $START_IDX to $END_IDX"
echo "Total formulas: $COUNT"
echo "========================================"

# Find ltlf files (may be in subdirectories)
FILES=($(find "$BENCHMARK_DIR" -name "*.ltlf" 2>/dev/null | sort))
TOTAL_FILES=${#FILES[@]}

if [ $TOTAL_FILES -eq 0 ]; then
    echo "Error: No .ltlf files found in $BENCHMARK_DIR"
    exit 1
fi

echo "Found $TOTAL_FILES benchmark files"
echo ""

# Adjust range if needed
if [ $END_IDX -gt $TOTAL_FILES ]; then
    END_IDX=$TOTAL_FILES
    echo "Adjusted end index to $TOTAL_FILES (total files available)"
fi

PASSED=0
FAILED=0
TIMEOUTS=0
ERRORS=0
TOTAL_TIME=0

# Results file
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
RESULT_DIR="results/benchmark/serial/$(date +"%Y-%m-%d")"
mkdir -p "$RESULT_DIR"
RESULT_FILE="$RESULT_DIR/serial_benchmark_${TIMESTAMP}.csv"

# CSV header
echo "idx,formula,expected,computed,matches,time_ms,status" > "$RESULT_FILE"

# Run each formula serially
for ((i=START_IDX; i<END_IDX; i++)); do
    FILE="${FILES[$i]}"
    BASENAME=$(basename "$FILE" .ltlf)

    # Get expected result from results.csv if available
    EXPECTED_FILE="$BENCHMARK_DIR/results.csv"
    EXPECTED="unknown"
    if [ -f "$EXPECTED_FILE" ]; then
        # results.csv format: Folder,Filename,Result
        # We need to match on Filename column
        EXPECTED_RAW=$(awk -F',' -v f="$BASENAME" '$2==f {print $3; exit}' "$EXPECTED_FILE")
        if [ "$EXPECTED_RAW" = "Realizable" ]; then
            EXPECTED="true"
        elif [ "$EXPECTED_RAW" = "Unrealizable" ]; then
            EXPECTED="false"
        fi
    fi

    echo "[$((i+1))/$END_IDX] Testing: $BASENAME (expected: $EXPECTED)"

    START_TIME=$(date +%s%N)

    # Run synthesis
    PART_FILE="${FILE%.ltlf}.part"
    OUTPUT=$(timeout 60 ./build/Cosy2 -f "$FILE" -p "$PART_FILE" 2>&1)
    EXIT_CODE=$?

    END_TIME=$(date +%s%N)
    ELAPSED_MS=$(( (END_TIME - START_TIME) / 1000000 ))
    TOTAL_TIME=$((TOTAL_TIME + ELAPSED_MS))

    STATUS="PASS"
    MATCHES="true"
    COMPUTED="unknown"

    if [ $EXIT_CODE -eq 124 ]; then
        STATUS="TIMEOUT"
        TIMEOUTS=$((TIMEOUTS + 1))
        COMPUTED="timeout"
        MATCHES="false"
    elif [ $EXIT_CODE -ne 0 ]; then
        STATUS="ERROR"
        ERRORS=$((ERRORS + 1))
        COMPUTED="error"
        MATCHES="false"
    else
        # Parse result
        if echo "$OUTPUT" | grep -q "REALIZABLE"; then
            COMPUTED="true"
        elif echo "$OUTPUT" | grep -q "UNREALIZABLE"; then
            COMPUTED="false"
        fi

        # Check if matches expected
        if [ "$EXPECTED" = "true" ] && [ "$COMPUTED" = "true" ]; then
            MATCHES="true"
            PASSED=$((PASSED + 1))
        elif [ "$EXPECTED" = "false" ] && [ "$COMPUTED" = "false" ]; then
            MATCHES="true"
            PASSED=$((PASSED + 1))
        elif [ "$EXPECTED" = "unknown" ]; then
            MATCHES="N/A"
        else
            MATCHES="false"
            FAILED=$((FAILED + 1))
        fi
    fi

    # Write to CSV
    echo "$i,$BASENAME,$EXPECTED,$COMPUTED,$MATCHES,$ELAPSED_MS,$STATUS" >> "$RESULT_FILE"

    echo "  Result: $COMPUTED (${ELAPSED_MS}ms)"
    echo ""
done

# Summary
echo "========================================"
echo "Summary"
echo "========================================"
echo "Total: $COUNT"
echo "Passed: $PASSED ($(python3 -c "print(f'{100*$PASSED/$COUNT:.2f}%')" 2>/dev/null || echo "N/A"))"
echo "Failed: $FAILED"
echo "Timeouts: $TIMEOUTS"
echo "Errors: $ERRORS"
echo "Total Time: $TOTAL_TIME ms ($(($TOTAL_TIME / 1000)) seconds)"
echo "Results saved to: $RESULT_FILE"
echo "========================================"
