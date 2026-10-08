#!/usr/bin/env bash
set -euo pipefail
TESTING_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SIMAI_MESH_ROOT="${SIMAI_MESH_ROOT:-$(dirname "$TESTING_ROOT")}"
SIMAI_MESH_ROOT="$(cd "$SIMAI_MESH_ROOT" && pwd)"
if [ ! -f "$SIMAI_MESH_ROOT/firmware/components/network_layer/network_layer.c" ]; then
    echo 'Set SIMAI_MESH_ROOT to the root of a firmware checkout' >&2; exit 2
fi
IMAGE='throwtheswitch/madsciencelab-plugins@sha256:90f393775dbe7e77b089292903a0e046e183caa8ccee9a83a759fe7cb6ea146a'
TESTING_COMMIT="$(git -C "$TESTING_ROOT" rev-parse HEAD)"
TESTING_STATUS="$(git -C "$TESTING_ROOT" status --porcelain --untracked-files=all -- . ':!results')"
FIRMWARE_COMMIT="$(git -C "$SIMAI_MESH_ROOT" rev-parse HEAD)"
FIRMWARE_BRANCH="$(git -C "$SIMAI_MESH_ROOT" branch --show-current)"
FIRMWARE_STATUS="$(git -C "$SIMAI_MESH_ROOT" status --porcelain --untracked-files=all -- firmware)"
docker run --rm --mount "type=bind,source=$SIMAI_MESH_ROOT,target=/simai,readonly" \
    --mount "type=bind,source=$TESTING_ROOT,target=/testing" -w /testing \
    -e SIMAI_MESH_ROOT=/simai -e "TEST_IMAGE=$IMAGE" \
    -e "TESTING_COMMIT=$TESTING_COMMIT" -e "TESTING_STATUS=$TESTING_STATUS" \
    -e "FIRMWARE_COMMIT=$FIRMWARE_COMMIT" -e "FIRMWARE_BRANCH=$FIRMWARE_BRANCH" -e "FIRMWARE_STATUS=$FIRMWARE_STATUS" \
    --entrypoint bash "$IMAGE" scripts/run_all.sh
