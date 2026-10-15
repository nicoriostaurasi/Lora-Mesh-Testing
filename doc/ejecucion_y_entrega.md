# Ejecución y entrega del TP

## Salida de resultados

Los logs, reportes y metadatos se escriben directamente en `results/`, reemplazando todo su contenido en cada ejecución. No se usa `RunId`. Las referencias antiguas a carpetas fechadas son antecedentes; esas evidencias no se conservan automáticamente con este esquema. La limpieza se limita a `/testing/results` y rechaza que esa carpeta sea un enlace simbólico.


## Resultado vigente — integración ampliada (13 de octubre de 2026)

Suite completa verificada: **16 unitarios y 5 integraciones PASS**, sin fallos ni casos ignorados; análisis estático y regresión host PASS. Corrida `results/`, en el nuevo repositorio LORA_MESH_TESTING con firmware copiado de SIMAI Mesh y los mismos módulos reales. El código de los tests se sincronizó en ambos clones; no se acredita una segunda ejecución en el clon original.

Cobertura de integración: **232/251 líneas (92,4 %), 132/168 ramas (78,6 %) y 21/21 funciones (100 %)**. Cobertura unitaria sin cambios: 90 % de líneas y 82,8 % de ramas. Los cinco casos están descritos en [casos_integracion.md](casos_integracion.md); IS-01 mantiene la derivación STT mostrada y los otros cuatro incorporan límites, errores, cambios de rutas y corrupción de mensajes. Son cinco funciones de test, algunas con variantes parametrizadas. El objetivo del 90 % se refiere a líneas.

La cobertura anterior de integración de 65,7 % correspondía a IS-01 sola. Las referencias a esa corrida en el historial no representan el resultado vigente. No se modificó firmware ni se usaron placas. `results/` continúa ignorada y no se hicieron commits ni push.


## Estado y configuración

Implementados y ejecutados: 16 casos unitarios CP-01 a CP-16, integración IS-01, análisis estático y regresión host de red. Los primeros nueve provienen del CFT de reenvío; los siete complementarios amplían las APIs de red. Se mantiene el firmware original sin modificaciones. La presentación de defensa está en Google Slides; resta la exposición oral.

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

El runner valida la ruta y limpia el contenido anterior de `results/`; cada ejecución escribe directamente en esa carpeta y reemplaza la anterior. Monta el firmware como solo lectura, ejecuta con `set -euo pipefail` y devuelve error si alguna etapa falla. Los builds regenerables quedan ignorados por Git en `build/`.

Dentro del contenedor la secuencia es:

1. Registrar versiones, Git del host y hashes SHA-256.
2. Compilar red y formato con GCC `-fanalyzer -Wall -Wextra -Werror`.
3. Ejecutar `ceedling clobber test:all` para la suite unitaria.
4. Ejecutar `ceedling gcov:all` y generar reportes gcovr.
5. Ejecutar `ceedling --project integration.yml clobber test:all`.
6. Ejecutar `ceedling --project integration.yml gcov:all` y reportar su cobertura aparte.
7. Compilar/ejecutar la suite host de red preexistente.

La consola muestra `Test 1: PASS` hasta `Test 16: PASS` para CP-01 a CP-16, y `Test 17: PASS` para IS-01. También puede mostrar FAIL o IGNORE según el resultado real. Los logs originales se conservan; ante un fallo Unity el runner termina con error aunque Ceedling devuelva cero. Las corridas de cobertura no repiten la lista numerada. No hay un generador adicional de resumen ni un umbral obligatorio de cobertura.

No se combinan los porcentajes unitarios y de integración. La suite instrumentada vuelve a ejecutar los mismos casos; no son casos adicionales.

## Resultado registrado

| Actividad | Resultado |
|---|---|
| CP-01 a CP-16 | 16 PASS, 0 FAIL, 0 IGNORE |
| IS-01 | 1 PASS, 0 FAIL, 0 IGNORE |
| Regresión host anterior de red (10 funciones de test) | PASS |
| Revisión estática de red y formato | Sin diagnósticos con GCC -fanalyzer y advertencias como errores |
| prepare_forward, solo suite unitaria | 25/25 líneas; 12/12 ramas tomadas (100 %) |
| network_layer.c, solo suite unitaria ampliada | 144/160 líneas (90 %); 101/122 ramas (82,8 %); 13/15 funciones (86,7 %) |
| network_layer.c, solo integración | 102/160 líneas (63,7 %); 48/122 ramas (39,3 %) |
| mesh_frame.c, solo integración | 63/91 líneas (69,2 %); 21/46 ramas (45,7 %) |

Entorno: Ceedling 1.0.0, Unity 2.6.1, Ruby 3.1.2, GCC/gcov 12.2.0, gcovr 5.2, Python 3.11.2. La evidencia identifica la versión exacta del firmware y el estado de la copia de trabajo ensayada.

Corrida vigente: `2026-10-10_stt_verificado`. La corrida anterior ampliada registró un fallo del helper al comparar arrays de longitud cero; se corrigió el helper sin cambiar firmware ni expectativas. Se conserva como evidencia intermedia, no como resultado final. La cobertura anterior de 52,5 %/36,1 % pertenece a los nueve tests iniciales. La cobertura actual mide ejecución, no corrección exhaustiva ni MC/DC.

Al repetir la ejecución, consultar `unit.log`, `integration.log`, `unit_coverage.html` e `integration_coverage.html` directamente en `results/`. Estos archivos se generan al ejecutar el runner y se abren localmente; no se publican en Git.

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

## Reportes nativos y automatización en PR

Las configuraciones incluyen `report_tests_log_factory`, siguiendo el ejemplo de la rama develop del profesor. Se generan JSON, JUnit y HTML en `build/unit/artifacts/test/` y `build/integration/artifacts/test/`. La salida numerada de consola se conserva.

La configuración local de GitHub Actions se encuentra en el repositorio principal y aún requiere publicación autorizada para ejecutarse allí. Ver [automatización y referencia](automatizacion.md).

La suite conserva los nueve casos CFT y siete complementarios agrupados por comportamiento. El objetivo orientativo es cerca del 90 % de líneas, sin exigir 100 % ni alterar el código productivo. No es un requisito numérico del profesor. Quedan sin ensayo dedicado la expulsión de rutas con tabla llena, el retorno del ID al superar 65535, los getters de rutas y el predicado de entrega local; también algunas validaciones de argumentos. Esas limitaciones se declaran, no se excluyen del cálculo de cobertura.

## Verificación del ajuste STT — 10 de octubre de 2026

Se ejecutó la suite completa con `./scripts/run.ps1`. Resultado: 16 unitarios y una integración PASS, sin fallos ni casos ignorados; análisis estático y regresión host PASS. IS-01 incluye las nuevas comprobaciones intermedias S0 y T1–T5 y la igualdad de campos en ambos ciclos pack/unpack.

La evidencia vigente está en `results/2026-10-10_stt_verificado/`. Cobertura unitaria de network_layer.c: 144/160 líneas (90 %), 101/122 ramas (82,8 %) y 13/15 funciones (86,7 %). La integración mantiene su reporte independiente: 165/251 líneas (65,7 %), 69/168 ramas (41,1 %) y 17/21 funciones (81 %), sobre network_layer.c y mesh_frame.c.

El intento previo no llegó a ejecutar tests por un fallo de Docker Desktop; se conserva como incidencia histórica en `results/2026-10-10_stt_sistema_software/ejecucion_bloqueada.md`. Docker respondió al reintento y la verificación pendiente quedó resuelta. No se modificó firmware productivo ni se utilizaron placas, radio o SPI.

La presentación muestra el modelo STT, la secuencia y los resultados de la corrida verificada. Sigue pendiente ensayar diez minutos, confirmar fecha/modalidad y realizar la defensa oral. No se hicieron commits ni push.
