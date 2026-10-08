param(
    [string]$SimaiMeshRoot = '',
    [string]$RunId = '',
    [string]$GitCommand = 'git'
)
$ErrorActionPreference = 'Stop'
$testingRoot = Split-Path -Parent $PSScriptRoot
if (-not $SimaiMeshRoot) { $SimaiMeshRoot = Split-Path -Parent $testingRoot }
$firmwareRoot = (Resolve-Path -LiteralPath $SimaiMeshRoot).Path
if (-not (Test-Path -LiteralPath (Join-Path $firmwareRoot 'firmware/components/network_layer/network_layer.c'))) {
    throw 'Indicar -SimaiMeshRoot con la raiz del clon de simai-mesh.'
}
if (-not $RunId) { $RunId = Get-Date -Format 'yyyy-MM-dd_HH-mm-ss' }
if ($RunId -notmatch '^[a-zA-Z0-9_-]+$') { throw 'RunId invalido' }
$imageRef = 'throwtheswitch/madsciencelab-plugins@sha256:90f393775dbe7e77b089292903a0e046e183caa8ccee9a83a759fe7cb6ea146a'
$testingCommit = & $GitCommand -C $testingRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'No se pudo identificar el commit de testing' }
$testingStatus = (& $GitCommand -C $testingRoot status --porcelain --untracked-files=all -- . ':!results') -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'No se pudo leer el estado de testing' }
$firmwareCommit = & $GitCommand -C $firmwareRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'No se pudo identificar el firmware' }
$firmwareBranch = & $GitCommand -C $firmwareRoot branch --show-current
if ($LASTEXITCODE -ne 0) { throw 'No se pudo leer la rama de firmware' }
$firmwareStatus = (& $GitCommand -C $firmwareRoot status --porcelain --untracked-files=all -- firmware) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'No se pudo leer el estado del firmware' }
docker run --rm `
    --mount "type=bind,source=$firmwareRoot,target=/simai,readonly" `
    --mount "type=bind,source=$testingRoot,target=/testing" `
    -w /testing -e SIMAI_MESH_ROOT=/simai -e "RUN_ID=$RunId" -e "TEST_IMAGE=$imageRef" `
    -e "TESTING_COMMIT=$testingCommit" -e "TESTING_STATUS=$testingStatus" `
    -e "FIRMWARE_COMMIT=$firmwareCommit" -e "FIRMWARE_BRANCH=$firmwareBranch" -e "FIRMWARE_STATUS=$firmwareStatus" `
    --entrypoint bash $imageRef scripts/run_all.sh
if ($LASTEXITCODE -ne 0) { throw "La verificacion fallo (codigo $LASTEXITCODE); revisar results/$RunId." }
