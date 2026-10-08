"""Validate recorded tests and report function coverage separately from file coverage."""
import json
import os
from pathlib import Path
import re

result = Path(os.environ['RESULT_DIR'])
unit_log = (result/'unit.log').read_text()
integration_log = (result/'integration.log').read_text()
unit_cases = re.findall(r'(test_CP\d+_[^:\n]+):PASS', unit_log)
integration_cases = re.findall(r'(test_IS\d+_[^:\n]+):PASS', integration_log)
assert len(unit_cases) == 9 and len(set(unit_cases)) == 9, 'Expected 9 distinct unit tests'
assert len(integration_cases) == 1, 'Expected one integration test'
assert '9 Tests 0 Failures 0 Ignored' in unit_log
assert '1 Tests 0 Failures 0 Ignored' in integration_log
assert 'PASS network_layer host tests' in (result/'legacy_regression.log').read_text()
coverage = json.loads((result/'unit_coverage.json').read_text())
f = next(x for x in coverage['files'] if x['file'].endswith('/network_layer.c'))
functions = sorted(f['functions'], key=lambda x: x['lineno'])
index = next(i for i,x in enumerate(functions) if x['name']=='network_layer_prepare_forward')
start = functions[index]['lineno']
end = functions[index+1]['lineno'] if index+1<len(functions) else float('inf')
lines = [x for x in f['lines'] if start<=x['line_number']<end
         and not x.get('gcovr/noncode',False) and not x.get('gcovr/excluded',False)]
branches = [b for line in lines for b in line['branches']]
line_covered = sum(x['count']>0 for x in lines)
branch_covered = sum(x['count']>0 for x in branches)
assert lines and branches, 'Target function coverage missing'
assert line_covered == len(lines), 'A target function line was not covered'
assert branch_covered == len(branches), 'A target function branch was not covered'
unit_summary = json.loads((result/'unit_summary.json').read_text())
integration_summary = json.loads((result/'integration_summary.json').read_text())
meta = json.loads((result/'environment.json').read_text())

def metrics(s):
    return (f"{s['line_covered']}/{s['line_total']} ({s['line_percent']} %) | "
            f"{s['branch_covered']}/{s['branch_total']} ({s['branch_percent']} %)")

text = f'''# Resultado de ejecución

Fecha (Buenos Aires): {meta['timestamp_local']}. Entorno: {meta['environment']}.
Firmware: `{meta['firmware_commit']}`, rama `{meta['firmware_branch']}`.
Cambios locales en firmware: {'ninguno' if not meta['firmware_worktree_status'] else 'ver environment.json'}.
Identidad de tests/configuración: hashes SHA-256 en `environment.json` (el commit base no incluye por sí solo cambios locales).

## Pruebas

| Actividad | Resultado |
|---|---|
| Análisis estático GCC -fanalyzer, red y formato | PASS; sin diagnósticos con -Wall -Wextra -Werror |
| CP-01 a CP-09, Ceedling/Unity | 9 PASS, 0 FAIL, 0 IGNORE |
| IS-01, integración red + formato | 1 PASS, 0 FAIL, 0 IGNORE |
| Suite host de red preexistente | PASS |

## Cobertura (gcovr)

| Alcance | Líneas | Ramas tomadas al menos una vez |
|---|---|---|
| prepare_forward, suite unitaria | {line_covered}/{len(lines)} (100 %) | {branch_covered}/{len(branches)} (100 %) |
| network_layer.c completo, suite unitaria | {metrics(unit_summary)} |
'''
for item in integration_summary['files']:
    text += f"| {Path(item['filename']).name}, solo IS-01 | {metrics(item)} |\n"
text += '''
Las coberturas unitarias y de integración se midieron en carpetas independientes y no se sumaron.
El 100 % del flujo seleccionado no significa 100 % de la capa de red ni prueba de corrección completa.
El análisis GCC no acredita cumplimiento MISRA ni sustituye pruebas dinámicas.

## Casos ejecutados

'''
for case in unit_cases+integration_cases:
    text += f'- `{case}`: PASS.\n'
text += '''
## Evidencia y límites

Los archivos `.log`, JSON y HTML de esta carpeta permiten revisar ejecución y métricas.
Los HTML contienen resumen de cobertura, sin publicar código fuente del firmware privado.
Los JSON de cobertura contienen números de línea y contadores, sin el contenido de las líneas.
No se ensayaron placas, radio RF, SPI, FreeRTOS, alcance, consumo ni sensores físicos.
Los contratos RT-04/RT-09 se verifican con la semántica documentada: entrega local precede al chequeo de TTL.
'''
(result/'resumen.md').write_text(text,encoding='utf-8')
print(text)
