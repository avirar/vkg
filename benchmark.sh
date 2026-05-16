#!/bin/bash
# vkg performance benchmark suite
# Usage: ./benchmark.sh [particle_counts...]
# Default counts: 1K 10K 100K 500K 1M 2M 5M

BENCH_SECONDS=3
export DISPLAY=:99
cd "$(dirname "$0")/build"

COUNTS=${*:-"1000 10000 100000 500000 1000000 2000000 5000000"}

echo "vkg benchmark — ${BENCH_SECONDS}s per test"
echo "=========================================="
printf "%-12s %-12s %-12s %-12s\n" "Particles" "Frames" "Elapsed" "Avg_FPS"

for N in $COUNTS; do
    output=$(timeout $((BENCH_SECONDS + 5)) ./vkg --benchmark "$BENCH_SECONDS" --particles "$N" 2>&1)
    line=$(echo "$output" | grep "^BENCHMARK")
    if [ -z "$line" ]; then
        printf "%-12s %-12s %-12s %-12s\n" "$N" "FAIL" "-" "-"
    else
        # Parse: BENCHMARK particles=N frames=F elapsed=E avg_fps=A
        frames=$(echo "$line" | grep -oP 'frames=\K[0-9.]+')
        elapsed=$(echo "$line" | grep -oP 'elapsed=\K[0-9.]+')
        avg_fps=$(echo "$line" | grep -oP 'avg_fps=\K[0-9.]+')
        printf "%-12s %-12s %-12s %-12s\n" "$N" "$frames" "$elapsed" "$avg_fps"
    fi
done
