#!/usr/bin/env bash
set -euo pipefail
cd /testing
export SIMAI_MESH_ROOT="${SIMAI_MESH_ROOT:-/simai}"
export TZ=America/Argentina/Buenos_Aires
export RESULT_DIR="results"
if [ -L "$RESULT_DIR" ]; then echo 'results must not be a symlink' >&2; exit 2; fi
mkdir -p "$RESULT_DIR" build/static build/regression
if [ "$(realpath -- "$RESULT_DIR")" != /testing/results ]; then
    echo 'Unexpected results directory; refusing cleanup' >&2; exit 2
fi
# Replace only the contents of the fixed results directory before each run.
find "$RESULT_DIR" -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
run_logged() {
    local name="$1"; shift
    local status=0
    printf 'Command:' > "$RESULT_DIR/$name.log"
    printf ' %q' "$@" >> "$RESULT_DIR/$name.log"
    printf '\n' >> "$RESULT_DIR/$name.log"
    if "$@" >> "$RESULT_DIR/$name.log" 2>&1; then
        status=0
    else
        status=$?
    fi
    # Some Ceedling tasks return zero even when Unity reports a failed test.
    # Propagate the actual test failure so CI cannot accept that run.
    if [ "$status" -eq 0 ] && grep -Eq ':test_[^:]+:FAIL(:|$)' "$RESULT_DIR/$name.log"; then
        status=1
    fi
    case "$name" in
        unit|integration)
            awk -F: '
                { for (i = 1; i < NF; i++) {
                    if ($i ~ /^test_(CP|IS)[0-9]+_/ && $(i+1) ~ /^(PASS|FAIL|IGNORE)$/) {
                        number = substr($i, 8, 2) + 0;
                        if ($i ~ /^test_IS/) number += unit_count;
                        printf "TEST %02d: %s - %s\n", number, $(i+1), $i;
                    }
                } }
            ' unit_count="$(grep -hc '^void test_CP[0-9]*_' test/unit/*.c | awk '{s+=$1} END {print s+0}')" "$RESULT_DIR/$name.log"
            ;;
        coverage_report|integration_report)
            if [ "$name" = coverage_report ]; then
                printf '\nCobertura unitaria:\n'
            else
                printf '\nCobertura de integracion:\n'
            fi
            grep -E '^(lines|functions|branches):' "$RESULT_DIR/$name.log" || true
            ;;
        static_network|static_frame|legacy_regression)
            if [ "$status" -eq 0 ]; then printf '%s: PASS\n' "$name"; fi
            ;;
    esac
    if [ "$status" -ne 0 ]; then
        printf '\nFallo en %s. Detalle:\n' "$name" >&2
        cat "$RESULT_DIR/$name.log" >&2
        return "$status"
    fi
}
run_logged versions bash -lc 'ceedling version; ruby --version; gcc --version; gcov --version; gcovr --version; python3 --version'
python3 scripts/record_environment.py
INCLUDES=("-I$SIMAI_MESH_ROOT/firmware/test/host/include"
          "-I$SIMAI_MESH_ROOT/firmware/components/network_layer/include"
          "-I$SIMAI_MESH_ROOT/firmware/components/mesh_frame/include")
run_logged static_network gcc -std=c17 -Wall -Wextra -Werror -fanalyzer "${INCLUDES[@]}" \
    -c "$SIMAI_MESH_ROOT/firmware/components/network_layer/network_layer.c" -o build/static/network_layer.o
run_logged static_frame gcc -std=c17 -Wall -Wextra -Werror -fanalyzer "${INCLUDES[@]}" \
    -c "$SIMAI_MESH_ROOT/firmware/components/mesh_frame/mesh_frame.c" -o build/static/mesh_frame.o
run_logged unit ceedling clobber test:all
run_logged unit_coverage ceedling gcov:all
run_logged coverage_report gcovr -r "$SIMAI_MESH_ROOT" \
    --filter "$SIMAI_MESH_ROOT/firmware/components/network_layer/network_layer.c" \
    --json "$RESULT_DIR/unit_coverage.json" --json-summary "$RESULT_DIR/unit_summary.json" \
    --html "$RESULT_DIR/unit_coverage.html" --print-summary build/unit/gcov
run_logged integration ceedling --project integration.yml clobber test:all
run_logged integration_coverage ceedling --project integration.yml gcov:all
run_logged integration_report gcovr -r "$SIMAI_MESH_ROOT" \
    --filter "$SIMAI_MESH_ROOT/firmware/components/(network_layer|mesh_frame)/.*\.c" \
    --json "$RESULT_DIR/integration_coverage.json" --json-summary "$RESULT_DIR/integration_summary.json" \
    --html "$RESULT_DIR/integration_coverage.html" --print-summary build/integration/gcov
run_logged legacy_build gcc -std=c17 -Wall -Wextra -Werror "${INCLUDES[@]}" \
    "$SIMAI_MESH_ROOT/firmware/components/network_layer/network_layer.c" \
    "$SIMAI_MESH_ROOT/firmware/components/mesh_frame/mesh_frame.c" \
    "$SIMAI_MESH_ROOT/firmware/test/host/test_network_layer/test_network_layer.c" \
    -o build/regression/test_network_layer
run_logged legacy_regression build/regression/test_network_layer
printf '\nEvidence saved to %s\n' "$RESULT_DIR"
