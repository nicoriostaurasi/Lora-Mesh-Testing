# Resultado de ejecución

Fecha (Buenos Aires): 2026-10-07T21:05:11.125588-03:00. Entorno: Linux x86_64 in Docker; no hardware.
Firmware: `c66dd10547b2991a00f0b45c3130e9e2690ff1ba`, rama `testing`.
Cambios locales en firmware: ninguno.
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
| prepare_forward, suite unitaria | 25/25 (100 %) | 12/12 (100 %) |
| network_layer.c completo, suite unitaria | 84/160 (52.5 %) | 44/122 (36.1 %) |
| mesh_frame.c, solo IS-01 | 63/91 (69.2 %) | 21/46 (45.7 %) |
| network_layer.c, solo IS-01 | 102/160 (63.7 %) | 48/122 (39.3 %) |

Las coberturas unitarias y de integración se midieron en carpetas independientes y no se sumaron.
El 100 % del flujo seleccionado no significa 100 % de la capa de red ni prueba de corrección completa.
El análisis GCC no acredita cumplimiento MISRA ni sustituye pruebas dinámicas.

## Casos ejecutados

- `test_CP01_null_context_preserves_frame`: PASS.
- `test_CP02_uninitialized_context_preserves_frame_and_counters`: PASS.
- `test_CP03_null_frame_preserves_counters`: PASS.
- `test_CP04_wrong_next_hop_is_rejected_without_mutation`: PASS.
- `test_CP05_local_delivery_precedes_zero_ttl_and_route_lookup`: PASS.
- `test_CP06_expired_ttl_is_rejected_even_with_route`: PASS.
- `test_CP07_missing_route_is_rejected_without_mutation`: PASS.
- `test_CP08_forward_updates_headers_and_preserves_identity_and_payload`: PASS.
- `test_CP09_ttl_one_allows_last_forward`: PASS.
- `test_IS01_sensor_bridge_concentrator_preserves_message_over_wire_format`: PASS.

## Evidencia y límites

Los archivos `.log`, JSON y HTML de esta carpeta permiten revisar ejecución y métricas.
Los HTML contienen resumen de cobertura, sin publicar código fuente del firmware privado.
Los JSON de cobertura contienen números de línea y contadores, sin el contenido de las líneas.
No se ensayaron placas, radio RF, SPI, FreeRTOS, alcance, consumo ni sensores físicos.
Los contratos RT-04/RT-09 se verifican con la semántica documentada: entrega local precede al chequeo de TTL.
