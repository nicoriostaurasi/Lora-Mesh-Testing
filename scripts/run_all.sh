#!/usr/bin/env bash
set -euo pipefail
cd /testing
export SIMAI_MESH_ROOT="${SIMAI_MESH_ROOT:-/simai}"
export TZ=America/Argentina/Buenos_Aires
RUN_ID="${RUN_ID:-$(date +%Y-%m-%d_%H-%M-%S)}"
case "$RUN_ID" in *[!a-zA-Z0-9_-]*) echo 'Invalid RUN_ID' >&2; exit 2;; esac
export RESULT_DIR="results/$RUN_ID"
if [ -e "$RESULT_DIR" ]; then echo "Result directory exists: $RESULT_DIR" >&2; exit 2; fi
mkdir -p "$RESULT_DIR" build/static build/regression
run_logged() {
    local name="$1"; shift
    printf 'Command:' | tee "$RESULT_DIR/$name.log"
    printf ' %q' "$@" | tee -a "$RESULT_DIR/$name.log"
    printf '\n' | tee -a "$RESULT_DIR/$name.log"
    "$@" 2>&1 | tee -a "$RESULT_DIR/$name.log"
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
python3 scripts/summarize.py
printf '\nEvidence saved to %s\n' "$RESULT_DIR"
