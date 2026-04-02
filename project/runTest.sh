#!/usr/bin/env bash
#
# runTest.sh - Automatically run benchmarks for ICFG analysis
# Usage: ./runTest.sh [cfg|cg|icfg|all]
# If no argument is provided, it defaults to "all".

# Path to the analysis tool
ANALYSIS_TOOL="./build/myAnalysis"

# Root directory for benchmarks
BENCH_ROOT="benchmark"

# If user provided an arg (cfg|cg|icfg|pta|all), use it; otherwise default to "all"
FUNCTION=${1:-"all"}

####################################################################
# Remove dot files generated during analysis
####################################################################
function clean() {
    rm -f *.dot
}

####################################################################
# Compare generated .dot files with reference .dot files
####################################################################
function genPNG()
{
    dotFile="$1"
    pngFile="${dotFile}.png"
    dot -Tpng "$dotFile" -o "$pngFile"
}

function compareResults() 
{
    benchDir="$1"
    results=$(ls *.dot 2>/dev/null || true)

    for res in $results; do

        cp "$res" "$benchDir/$res"
        genPNG "$benchDir/$res"

    done
}

####################################################################
# Run analysis
####################################################################
function runAnalysis() 
{
    for benchDir in "$BENCH_ROOT"/bench*; do
        if [ ! -f "$benchDir/demo.bc" ]; then
            echo "Skipping $benchDir: no demo.bc."
            continue
        fi

        clean

        function="$1"
        bcFile="$benchDir/demo.bc"

        echo ">>>> Running <$function> analysis on $benchDir"
        "$ANALYSIS_TOOL" -b "$bcFile" -f $function

        compareResults $benchDir
    done
    clean
}

####################################################################
# Main process
####################################################################
# Choose which analyses to run
if [ "$FUNCTION" = "all" ] || [ "$FUNCTION" = "cfg" ]; then
    runAnalysis cfg
fi

if [ "$FUNCTION" = "all" ] || [ "$FUNCTION" = "cg" ]; then
    runAnalysis cg
fi

if [ "$FUNCTION" = "all" ] || [ "$FUNCTION" = "icfg" ]; then
    runAnalysis icfg
fi

if [ "$FUNCTION" = "all" ] || [ "$FUNCTION" = "pta" ]; then
    runAnalysis pta
fi

if [ "$FUNCTION" = "all" ] || [ "$FUNCTION" = "dfg" ]; then
    runAnalysis dfg
fi

echo "All benchmark analyses finished."
