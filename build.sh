#!/bin/bash

INPUT_DIR="input_files"
OUTPUT_DIR="output_files"

echo "Creating output directory..."
mkdir -p $OUTPUT_DIR

echo ""
echo "====================== COMPILING ======================"

echo "Compiling EP..."
g++ -std=c++17 interrupts_wendingsha_janbeyati_EP.cpp -o interrupts_EP

echo "Compiling RR..."
g++ -std=c++17 interrupts_wendingsha_janbeyati_RR.cpp -o interrupts_RR

echo "Compiling EP_RR..."
g++ -std=c++17 interrupts_wendingsha_janbeyati_EP_RR.cpp -o interrupts_EP_RR

echo "Compilation complete!"
echo "======================================================="
echo ""

run_scheduler () {
    local EXEC="$1"
    local TAG="$2"

    echo ""
    echo "====================== Running $TAG ======================"

    for file in $INPUT_DIR/*.txt; do
        base=$(basename "$file")

        echo "Running $TAG on $base..."

        ./$EXEC "$file"

        cp execution.txt "$OUTPUT_DIR/execution_${TAG}_${base}"
    done
}

run_scheduler "interrupts_EP" "EP"
run_scheduler "interrupts_RR" "RR"
run_scheduler "interrupts_EP_RR" "EP_RR"

echo ""
echo "====================== ALL DONE ======================="
echo "Results saved in: $OUTPUT_DIR/"
echo "======================================================="
