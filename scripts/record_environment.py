"""Record source identity without publishing private firmware contents."""
import datetime
import hashlib
import json
import os
from pathlib import Path
from zoneinfo import ZoneInfo

root = Path(os.environ['SIMAI_MESH_ROOT'])
result = Path(os.environ['RESULT_DIR'])

files = [
    'firmware/components/network_layer/network_layer.c',
    'firmware/components/network_layer/include/network_layer.h',
    'firmware/components/mesh_frame/mesh_frame.c',
    'firmware/components/mesh_frame/include/mesh_frame.h',
    'firmware/test/host/include/esp_err.h',
    'firmware/test/host/test_network_layer/test_network_layer.c',
]
metadata = {
    'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'timestamp_local': datetime.datetime.now(ZoneInfo('America/Argentina/Buenos_Aires')).isoformat(),
    'environment': 'Linux x86_64 in Docker; no hardware',
    'image_reference': os.environ.get('TEST_IMAGE', ''),
    'firmware_commit': os.environ['FIRMWARE_COMMIT'],
    'firmware_branch': os.environ['FIRMWARE_BRANCH'],
    'firmware_worktree_status': os.environ.get('FIRMWARE_STATUS', ''),
    'testing_base_commit': os.environ['TESTING_COMMIT'],
    'testing_worktree_status': os.environ.get('TESTING_STATUS', ''),
    'source_sha256': {f: hashlib.sha256((root/f).read_bytes()).hexdigest() for f in files},
    'test_sha256': {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in sorted(Path('test').rglob('*.c'))},
    'config_sha256': {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in [Path('project.yml'), Path('integration.yml')]},
}
(result/'environment.json').write_text(json.dumps(metadata, indent=2)+'\n')
