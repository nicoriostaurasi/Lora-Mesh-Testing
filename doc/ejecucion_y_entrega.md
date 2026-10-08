# Ejecución y entrega del TP

## Estado y configuración

Implementados y ejecutados: nueve casos unitarios CP-01 a CP-09, integración IS-01, análisis estático y regresión host de red. Se mantiene el firmware original sin modificaciones. La presentación se prepara en Markdown; resta al autor realizar la defensa oral y ajustar su duración.

- [project.yml](../project.yml): suite unitaria; fuente productiva `network_layer.c`.
- [integration.yml](../integration.yml): suite separada de red más formato `mesh_frame.c`.
- [Test unitario](../test/unit/test_network_layer.c) y [test de integración](../test/integration/test_multihop.c).
- [Runner Windows](../scripts/run.ps1), [runner Linux](../scripts/run.sh) y [secuencia del contenedor](../scripts/run_all.sh).

El componente de red es puro y utiliza tipos de `mesh_frame.h` y la cabecera host `esp_err.h`; no necesita la radio, ESP-IDF ni mocks externos para el flujo elegido. Unity comprueba resultados y Ceedling genera runners/build. No se desarrolló el componente nuevamente con TDD.

## Reproducir

Abrir Docker Desktop con motor Linux activo. Desde la raíz de este repositorio, con Git en PATH:

```powershell
./scripts/run.ps1
# Para un clon independiente:
./scripts/run.ps1 -SimaiMeshRoot 'C:/ruta/al/simai-mesh'
```

En Linux:

```bash
SIMAI_MESH_ROOT=/ruta/al/simai-mesh bash scripts/run.sh
```

Docker descarga la imagen si no está disponible. Se fija el digest `sha256:90f393775dbe7e77b089292903a0e046e183caa8ccee9a83a759fe7cb6ea146a` de la imagen oficial `throwtheswitch/madsciencelab-plugins:1.0.0`. Evita depender de una etiqueta móvil.

El runner valida la ruta y crea `results/<fecha_hora>/`; rechaza identificadores ya existentes para conservar evidencias. Monta el firmware como solo lectura, ejecuta con `set -euo pipefail` y devuelve error si alguna etapa falla. Los builds regenerables quedan ignorados por Git en `build/`.

Dentro del contenedor la secuencia es:

1. Registrar versiones, Git del host y hashes SHA-256.
2. Compilar red y formato con GCC `-fanalyzer -Wall -Wextra -Werror`.
3. Ejecutar `ceedling clobber test:all` para la suite unitaria.
4. Ejecutar `ceedling gcov:all` y generar reportes gcovr.
5. Ejecutar `ceedling --project integration.yml clobber test:all`.
6. Ejecutar `ceedling --project integration.yml gcov:all` y reportar su cobertura aparte.
7. Compilar/ejecutar la suite host de red preexistente.
8. Validar 9 PASS unitarios, 1 PASS de integración y cobertura completa de líneas/ramas del flujo seleccionado; generar `resumen.md`.

No se combinan los porcentajes unitarios y de integración. La suite instrumentada vuelve a ejecutar los mismos casos; no son casos adicionales.

## Resultado registrado

| Actividad | Resultado |
|---|---|
| CP-01 a CP-09 | 9 PASS, 0 FAIL, 0 IGNORE |
| IS-01 | 1 PASS, 0 FAIL, 0 IGNORE |
| Regresión host anterior de red (10 funciones de test) | PASS |
| Revisión estática de red y formato | Sin diagnósticos con GCC -fanalyzer y advertencias como errores |
| prepare_forward, solo suite unitaria | 25/25 líneas; 12/12 ramas tomadas (100 %) |
| network_layer.c, solo suite unitaria | 84/160 líneas (52,5 %); 44/122 ramas (36,1 %) |
| network_layer.c, solo integración | 102/160 líneas (63,7 %); 48/122 ramas (39,3 %) |
| mesh_frame.c, solo integración | 63/91 líneas (69,2 %); 21/46 ramas (45,7 %) |

Entorno: Ceedling 1.0.0, Unity 2.6.1, Ruby 3.1.2, GCC/gcov 12.2.0, gcovr 5.2, Python 3.11.2. La evidencia identifica la versión exacta del firmware y el estado de la copia de trabajo ensayada.

Al repetir la ejecución, consultar `resumen.md`, `unit_coverage.html` e `integration_coverage.html` dentro de la nueva carpeta `results/<fecha_hora>/`. Estos archivos se generan al ejecutar el runner y se abren localmente; no se publican en Git.

Los HTML son resúmenes sin código fuente; los JSON registran nombres, números de línea y contadores. La suite pública requiere acceso al firmware para compilar. Los resultados no certifican radio real, SPI, FreeRTOS, sensores, distancia, consumo ni estabilidad prolongada.

## Trazabilidad y problemas de entorno resueltos

El primer intento del runner falló antes de ejecutar tests por la ruta `.git` del submódulo dentro del contenedor. Se corrigió leyendo la identidad de Git en el host. Una corrida intermedia pasó los tests, pero Git Linux informaba cambios de fin de línea CRLF como modificaciones del firmware. El registro definitivo toma estado y commit desde Git del host y conserva hashes de los bytes efectivamente compilados. Las evidencias intermedias se conservaron localmente en `work/evidence_attempts`, sin presentarlas como la corrida final.

Las primeras pruebas de configuración también permitieron ajustar la sintaxis de flags gcov de Ceedling 1.0.0. No se modificaron los resultados esperados ni el firmware para obtener PASS.

## Checklist de cierre

- [x] Proyecto, alcance y requerimientos de prueba definidos.
- [x] Master Test Plan con características, prioridades, niveles, esfuerzo y exclusiones.
- [x] Técnica CFT, siete caminos y nueve casos físicos documentados.
- [x] Semántica de TTL/entrega local explícita en contrato y verificada; no constituye aprobación externa del protocolo.
- [x] Revisión estática registrada.
- [x] Casos implementados y ejecutados con Ceedling.
- [x] Integración de software y regresión de red ejecutadas.
- [x] Cobertura medida y límites explicados.
- [x] Logs, hashes y reportes preparados como evidencia.
- [x] [Presentación en Markdown](presentacion.md) preparada con resultados reales.
- [ ] Confirmar duración y fecha con la cátedra y realizar la defensa oral.

No se requiere repetir banco o campo para este alcance de la materia.
