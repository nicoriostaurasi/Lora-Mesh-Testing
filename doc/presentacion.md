# SIMAI Mesh — Trabajo final de Testing en Sistemas Embebidos

Presentación en Markdown. Cada sección corresponde a una diapositiva; los archivos de evidencia sirven como apoyo de la exposición.

## 1. Proyecto y problema

Red de sensores LoRa con pila propia sobre Heltec WiFi LoRa 32 V3, ESP32-S3 y SX1262. Escenario: sensor 1 → puente 2 → concentrador 3. La aplicación genera muestras simuladas.

Objetivo del TP: probar la lógica que decide entrega local, rechazo o reenvío y conserva identidad/payload. La ejecución se realiza en PC; las pruebas físicas del proyecto son contexto adicional.

## 2. Master Test Plan: calidad y prioridades

| Característica | Importancia | Motivo |
|---|---:|---|
| Funcionalidad | 50 % | Decisiones y campos correctos. |
| Confiabilidad | 35 % | Errores y descartes controlados. |
| Mantenibilidad | 15 % | Pruebas repetibles para regresión. |

Esfuerzo estimado: red 70 %, integración con formato 20 %, fixtures y datos 10 %. Son prioridades de planificación, no métricas de cobertura.

## 3. Master Test Plan: niveles y alcance

| Nivel | Actividad | Situación |
|---|---|---|
| Unitario PC | CFT de prepare_forward y errores | Ejecutado. |
| Integración software PC | Red + serialización en tres contextos | Ejecutado. |
| HW/SW y sistema en banco | SPI, radio y tres placas | Evidencia histórica; no repetida para TSSE. |
| Campo | Distancia, interferencia, consumo y estabilidad | Diferido. |

Responsable: autor del TP. Se ejecuta tras diseñar casos y ante cambios relevantes. Criterio de salida: todos los casos PASS, cobertura registrada y huecos explicados.

## 4. Generación de casos: CFT nivel 1

Objeto: network_layer_prepare_forward(). Flujo resumido:

```text
Validar contexto → validar trama → verificar siguiente salto
→ entregar si destino local → rechazar TTL cero
→ buscar ruta → rechazar sin ruta / preparar reenvío
```

Siete acciones finales dan siete caminos lógicos. Se derivan nueve casos físicos: dos variantes de contexto inválido y dos de reenvío (TTL 2 y TTL 1). El diagrama completo y entradas están en [casos_cft.md](casos_cft.md).

## 5. Casos y resultados esperados

| Casos | Comportamiento comprobado |
|---|---|
| CP-01 / CP-02 | Contexto nulo/no inicializado: INVALID_STATE. |
| CP-03 | Trama nula: INVALID_ARG. |
| CP-04 | Otro siguiente salto: rechazo sin alterar trama. |
| CP-05 | Entrega local incluso con TTL cero. |
| CP-06 | Destino remoto con TTL cero: descarte. |
| CP-07 | Sin ruta: error controlado. |
| CP-08 / CP-09 | Reenvío: TTL -1, hop_count +1, saltos actualizados. |

También se comprueban payload, identidad y seis contadores. Los casos rechazados conservan la trama; cada test tiene un fixture independiente. TTL cero en entrega local es semántica explícita de este protocolo.

## 6. Automatización con Ceedling

Unity realiza las aserciones; Ceedling genera runners y build. Se compila el archivo productivo directamente, sin copias ni dependencias físicas. La capa es pura; no se requieren mocks para los casos elegidos.

```c
TEST_ASSERT_EQUAL(NETWORK_LAYER_ERR_TTL_EXPIRED,
                  network_layer_prepare_forward(&bridge, &frame));
```

IS-01 utiliza network_layer y mesh_frame reales para originar, serializar, reenviar y entregar un mensaje entre tres contextos. La secuencia se ejecuta con `./scripts/run.ps1` y registra resultados automáticamente.

## 7. Ejecución y cobertura

9/9 unitarios PASS, 1/1 integración PASS; regresión host de red PASS. Análisis GCC -fanalyzer sin diagnósticos con advertencias como errores.

| Alcance de la medición unitaria | Líneas | Ramas tomadas |
|---|---|---|
| Función prepare_forward | 25/25 (100 %) | 12/12 (100 %) |
| Archivo network_layer.c | 84/160 (52,5 %) | 44/122 (36,1 %) |

Antes de la exposición, ejecutar el runner y mostrar `unit.log` y `unit_coverage.html` desde la carpeta local `results/<fecha_hora>/`. La integración tiene su reporte independiente. Ramas tomadas al menos una vez es la métrica usada; no es MC/DC.

## 8. Conclusiones y límites

Se cubrieron las tres etapas: planificación con prioridades, derivación CFT y automatización verificada. Los nueve casos verifican las decisiones elegidas y la integración conserva el mensaje hasta su entrega local.

Quedan pendientes pruebas exhaustivas de administración/desempates de rutas y APIs adicionales. Una cobertura del 100 % de esta función no acredita corrección de toda la red ni desempeño físico. No se midieron alcance RF, SPI, tiempos FreeRTOS, consumo ni sensores reales.

La evidencia identifica versiones, commits y hashes, y permite repetir la corrida. Próxima ampliación técnica: selección y expiración de rutas; banco y campo corresponden al plan general del proyecto.
