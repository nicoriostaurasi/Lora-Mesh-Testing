# Master Test Plan — SIMAI Mesh — Alcance del TP de TSSE

## Resultado vigente — integración ampliada (13 de octubre de 2026)

Suite completa verificada: **16 unitarios y 5 integraciones PASS**, sin fallos ni casos ignorados; análisis estático y regresión host PASS. Corrida `results/2026-10-13_integracion_90/`, en el nuevo repositorio LORA_MESH_TESTING con firmware copiado de SIMAI Mesh y los mismos módulos reales. El código de los tests se sincronizó en ambos clones; no se acredita una segunda ejecución en el clon original.

Cobertura de integración: **232/251 líneas (92,4 %), 132/168 ramas (78,6 %) y 21/21 funciones (100 %)**. Cobertura unitaria sin cambios: 90 % de líneas y 82,8 % de ramas. Los cinco casos están descritos en [casos_integracion.md](casos_integracion.md); IS-01 mantiene la derivación STT mostrada y los otros cuatro incorporan límites, errores, cambios de rutas y corrupción de mensajes. Son cinco funciones de test, algunas con variantes parametrizadas. El objetivo del 90 % se refiere a líneas.

La cobertura anterior de integración de 65,7 % correspondía a IS-01 sola. Las referencias a esa corrida en el historial no representan el resultado vigente. No se modificó firmware ni se usaron placas. `results/` continúa ignorada y no se hicieron commits ni push.


## 1. Proyecto y alcance

SIMAI Mesh es un prototipo de red de sensores LoRa sobre Heltec WiFi LoRa 32 V3 (ESP32-S3 y SX1262), con pila propia y reenvío multi-salto. El escenario de referencia es `sensor (1) -> puente (2) -> concentrador (3)`. Los datos de sensor actuales son simulados.

**Objetivo del alcance ejecutable:** verificar que la capa de red decide correctamente entre entrega local, reenvío y rechazo; preserva los datos y actualiza campos y contadores según el contrato propuesto.

**Incluido en la ejecución de TSSE:** pruebas unitarias de `network_layer_prepare_forward()` y sus dependencias internas, usando las APIs públicas para inicializar y cargar rutas; cobertura del archivo `network_layer.c`. También se ejecuta integración de software con `mesh_frame` para el recorrido de tres nodos (IS-01).

**Ampliación C:** además del CFT inicial se prueban inicialización, validaciones, selección/actualización/eliminación de rutas, construcción de mensajes, límites de payload y estadísticas. La fila de resultados de versión B al final conserva el alcance histórico; el cierre vigente es la versión C.

**Fuera de la ejecución comprometida:** radio/SPI reales, tareas FreeRTOS, sensores físicos, OLED/LED, alcance RF, consumo, estabilidad de varias horas y recuperación mediante caminos alternativos reales. La selección de rutas se usa como apoyo del reenvío; una caracterización exhaustiva de sus desempates y capacidad de tabla queda para una ampliación.

## 2. Bases de prueba

| Base | Uso |
|---|---|
| API de red (`firmware/components/network_layer/include/network_layer.h`) | Tipos, errores, configuración y contadores públicos. |
| Implementación de red (`firmware/components/network_layer/network_layer.c`) | Construcción del flujo CFT y contraste con el contrato. |
| Descripción de red (`firmware/docs/protocol/network_layer.md`) | Selección de rutas, TTL y escenario de tres nodos. |
| Formato de trama (`firmware/docs/protocol/frame_format.md`) | Campos y serialización para integración. |
| Tests host existentes (`firmware/test/host/test_network_layer/test_network_layer.c`) | Escenarios reutilizables; no constituyen evidencia Ceedling. |
| Resultados previos (`firmware/docs/tests/results_summary.md`) | Contexto de ensayos físicos anteriores. |
| Limitaciones (`firmware/docs/known_limitations.md`) | Límites de las conclusiones. |

Los resultados esperados se fijan en los requerimientos de prueba antes de ejecutar. Si documentación, intención funcional y código difieren, se registra la diferencia y se decide el contrato; no se cambia el resultado esperado solamente para lograr un PASS.

## 3. Requerimientos del alcance de prueba

| ID | Comportamiento esperado propuesto |
|---|---|
| RT-01 | Contexto nulo o no inicializado: `ESP_ERR_INVALID_STATE`. |
| RT-02 | Con contexto válido, trama nula: `ESP_ERR_INVALID_ARG`. |
| RT-03 | Una trama cuyo `next_hop` no coincide con la dirección local se rechaza con `NETWORK_LAYER_ERR_NOT_FOR_LOCAL_HOP`, sin modificar la trama; incrementa solo `dropped_not_for_hop`. |
| RT-04 | Si siguiente salto y destino son locales, se acepta con `ESP_OK`, conserva la trama e incrementa solo `delivered_local`, incluso con TTL cero. |
| RT-05 | Para destino remoto y salto local, TTL cero produce `NETWORK_LAYER_ERR_TTL_EXPIRED`, sin modificar la trama; incrementa solo `dropped_ttl`. |
| RT-06 | Para destino remoto, salto local y TTL positivo, ausencia de ruta produce `NETWORK_LAYER_ERR_NO_ROUTE`, sin modificar la trama; incrementa solo `dropped_no_route`. |
| RT-07 | Con ruta disponible, el reenvío devuelve `ESP_OK`, resta uno al TTL, suma uno a `hop_count`, fija `prev_hop` al nodo local y `next_hop` a la ruta elegida; incrementa solo `forwarded`. |
| RT-08 | El reenvío conserva tipo, origen, destino, identificador, longitud y bytes del payload. |
| RT-09 | TTL uno permite el reenvío y produce TTL cero. La entrega local en destino se evalúa antes del TTL. |

RT-04 y RT-09 hacen explícita la semántica observada en el código; conviene confirmar que esa semántica coincide con la intención del protocolo. No se afirma una regla universal de TTL. El desbordamiento de `hop_count` y la validación completa de tramas malformadas quedan fuera de este alcance inicial.

## 4. Riesgos y prioridades

| Riesgo | Impacto | Respuesta de prueba |
|---|---|---|
| Reenvío a salto incorrecto | Mensaje perdido o dirigido incorrectamente | Validación de `next_hop` y campos de salida. |
| Reenvío con TTL agotado | Propagación indebida | TTL 0, 1 y un valor mayor. |
| Ausencia de ruta tratada como éxito | Pérdida silenciosa | Código de error y contador. |
| Corrupción de payload o identidad | Muestra incorrecta en destino | Comparación de campos y bytes. |
| Contadores inconsistentes | Diagnóstico engañoso | Verificación del contador esperado y de los restantes. |
| Prueba PC confundida con validación RF | Conclusión sin evidencia | Separar software, banco y campo en los resultados. |

### Tabla A — Características de calidad

Los porcentajes representan importancia relativa de las características, no cobertura medida ni reparto de horas.

| Característica | Importancia | Justificación |
|---|---:|---|
| Funcionalidad | 50 % | La decisión y transformación de la trama deben cumplir el contrato. |
| Confiabilidad | 35 % | Las condiciones que impiden el reenvío deben producir errores controlados y diagnósticos consistentes. |
| Mantenibilidad | 15 % | Tests repetibles y componentes aislados permiten detectar regresiones tras cambios. |
| Total | 100 % | |

Se usa el vocabulario de calidad trabajado en el MTP del curso. Rendimiento RF, autonomía y portabilidad entre placas se difieren. Usabilidad de consola/pantalla se excluye del alcance de red. Seguridad y seguridad funcional no se validan en este TP: su exclusión no significa que carezcan de importancia para una futura versión operativa.

### Tabla B — Importancia relativa de subsistemas

Ponderación propuesta para los módulos incluidos en este TP, no porcentajes de cobertura ni horas medidas. Se concentra el 80 % en comunicación de red porque allí están las decisiones de rutas, entrega/reenvío/rechazo y errores. El 20 % restante corresponde al formato y conservación de los mensajes. Los fixtures son recursos para ejecutar las pruebas, no un tercer subsistema del producto.

| Subsistema | Importancia relativa | Foco |
|---|---:|---|
| Comunicación de red (`network_layer`) | 80 % | Decisiones, rutas, errores y contadores. |
| Formato de mensajes (`mesh_frame`) | 20 % | Codificación y conservación. |
| Total del alcance seleccionado | 100 % | |

### Tabla C — Cruce de calidad, nivel, técnica y momento

`++`: énfasis principal; `+`: aporte parcial. Son prioridades de planificación, no cobertura medida ni certificación de una característica completa.

| Subsistema / nivel | Funcionalidad | Confiabilidad | Mantenibilidad | Cómo / técnica | Cuándo |
|---|---|---|---|---|---|
| Comunicación de red / unitarios | ++ | ++ | + | CFT de reenvío, límites y errores. | Tras diseñar casos y ante cambios. |
| Comunicación de red + formato / integración | ++ | + | + | STT del recorrido exitoso; comprobar cada paso. | Tras diseñar STT y pasar unitarios. |

La regresión aporta a mantenibilidad mediante una suite repetible. La integración de éxito comprueba funcionamiento y conservación del mensaje, sin acreditar recuperación ante fallos o estabilidad prolongada. Conservación es un resultado esperado de funcionalidad, no una característica de calidad independiente.

Fuera de esta entrega: drivers/periféricos, control de enlace (ACK/reintentos), descubrimiento y mensajes de presencia, lógica de aplicación y ejecución de tareas. No se asigna ejecución ni se promete trabajo futuro para estos grupos. Las pruebas físicas históricas se conservan como contexto en la sección 7 y no se mezclan con esta tabla de planificación.

Las letras A y B siguen el material de MTP del curso: características ponderadas y subsistemas ponderados. C es el resumen combinado del cruce y de técnicas/momentos para esta defensa, no el nombre de una tercera tabla obligatoria enumerada por el profesor. La última clase pide dos o tres cuadros resumidos que expliquen qué, cómo y cuándo probar.

## 5. Recursos y preparación

- Responsable: autor del TP/proyecto.
- PC y Docker del módulo testing; imagen oficial fijada por digest y sin placas para la suite comprometida.
- Compilador C del host, Ceedling/Unity y herramientas gcov/gcovr compatibles entre sí; registrar las versiones efectivamente utilizadas.
- Código productivo existente y cabecera host `firmware/test/host/include/esp_err.h`, sin arrastrar ESP-IDF ni FreeRTOS para el componente puro.
- Análisis estático del componente como revisión de entrada recomendada por el material MTP: registrar hallazgos y correcciones o excepciones justificadas. No confundirlo con ejecución dinámica.

## 6. Secuencia de trabajo y criterios

1. Confirmar RT-01 a RT-09 y fijar la versión de código bajo prueba, incluyendo cambios locales si existen.
2. Revisar advertencias de compilación y análisis estático; registrar problemas que bloqueen la ejecución.
3. Completar el diagrama y la tabla CFT.
4. Implementar los casos en Ceedling, reutilizando escenarios existentes cuando corresponda.
5. Ejecutar suite y cobertura; corregir defectos o informar discrepancias.
6. Ejecutar integración de software IS-01 y registrar evidencia.
7. Preparar presentación, capturas/video y conclusiones limitadas a lo ensayado.

**Entrada:** contrato acordado, código identificable, entorno reproducible y fixtures independientes entre tests.

**Salida de la suite:** todos los casos comprometidos ejecutados, resultados comparados con el contrato, errores/discrepancias documentados y reporte de cobertura disponible. Se busca recorrer todas las salidas del flujo elegido, pero no se promete 100 % del archivo completo. Toda rama relevante no cubierta debe explicarse. No existe un umbral mínimo oficial de cobertura en la consigna oral.

**Cierre del TP:** además de la suite, presentación que muestre las tres etapas. Este documento por sí solo no acredita ese cierre.

## 7. Evidencia previa y límites

El resumen del repositorio registra para 2026-08-26 y commit `db5fbb0` una prueba multi-salto de 49/50 mensajes, 98 % de entrega y latencia máxima de 795 ms. Registra también pérdida del puente sin alternativa de ruta. Son resultados históricos documentados, no pruebas ejecutadas al crear este módulo ni validación automática de la copia de trabajo actual.

La aceptación física del proyecto usa criterios propios (por ejemplo entrega y latencia). Esos criterios no se trasladan a tests unitarios PC ni a un porcentaje de cobertura. Las rutas de la demostración son estáticas o derivadas de vecinos; no se acredita descubrimiento distribuido completo ni reselección física entre caminos alternativos.

## 8. Ejecución de esta versión del plan

**Versión B — 2026-10-07:** se implementaron CP-01 a CP-09 con Unity/Ceedling y se incorporó IS-01 al alcance ejecutado. Se ensaya la semántica RT-01 a RT-09 documentada; no se redefine el protocolo al ajustar los tests.

El entorno concreto es Docker Linux con una imagen oficial ThrowTheSwitch fijada por digest. Se ejecuta GCC `-fanalyzer` como revisión estática de entrada para `network_layer.c` y `mesh_frame.c`, además de `-Wall -Wextra -Werror`. No es un chequeo MISRA ni una certificación.

Las tres etapas están documentadas y la automatización produjo 9 PASS unitarios y 1 PASS de integración. Se cumple el criterio de salida para el flujo seleccionado: 25/25 líneas y 12/12 ramas tomadas en `prepare_forward`. El criterio de cobertura de esa función es una decisión de este plan, no un umbral impuesto por la cátedra.

La cobertura de todo el archivo con CP-01 a CP-09 es menor: 52,5 % de líneas y 36,1 % de ramas. Quedan fuera del recorrido unitario construcción de mensajes, eliminación de rutas, consulta de rutas, reinicio de contadores y predicado de entrega local. IS-01 recorre algunas de esas APIs, pero se reporta por separado. Tampoco se cubren completamente desempates de rutas, tabla llena ni todas las validaciones de entrada de APIs de apoyo.

El cierre académico incluye la [presentación preparada](presentacion.md) y su exposición dentro del tiempo de la cátedra. Los resultados y sus límites están resumidos en [ejecución y entrega](ejecucion_y_entrega.md). Los reportes detallados se generan localmente en `results/`, ignorada por Git.

## 9. Ampliación y cierre de versión C

Se amplía la suite a 16 pruebas unitarias: CP-01 a CP-09 derivadas por CFT, y CP-10 a CP-16 por particiones, límites y combinaciones de prioridades de ruta. IS-01 sigue como integración separada. La ponderación vigente 80/20 concentra la prioridad en comunicación de red y formato; fixtures no se consideran un subsistema del producto.

La corrida `2026-10-10_stt_verificado` pasó los 16 unitarios y la integración; análisis estático y regresión host PASS. El archivo `network_layer.c` registra 144/160 líneas (90 %), 101/122 ramas (82,8 %) y 13/15 funciones (86,7 %). Así quedan ensayadas las APIs antes pendientes, incluyendo selección, desempates y construcción de mensajes. La integración mantiene su propia cobertura, sin sumar porcentajes.

Se conserva fuera de alcance: RF, SPI, tareas FreeRTOS, sensores físicos y consumo. La cobertura medida no implica MC/DC, ausencia de defectos ni cobertura exhaustiva de todos los estados del sistema. No se agrega un umbral impuesto por la cátedra. La evidencia identifica estado y hashes del código bajo prueba.

La suite conserva los nueve casos CFT y siete complementarios agrupados por comportamiento. El objetivo orientativo es cerca del 90 % de líneas, sin exigir 100 % ni alterar el código productivo. No es un requisito numérico del profesor. Quedan sin ensayo dedicado la expulsión de rutas con tabla llena, el retorno del ID al superar 65535, los getters de rutas y el predicado de entrega local; también algunas validaciones de argumentos. Esas limitaciones se declaran, no se excluyen del cálculo de cobertura.

## 10. Diseño del recorrido de sistema en software (actualización 2026-10-10)

Para el punto 3 de la última clase se desarrolla [STT del recorrido sensor–puente–concentrador](casos_stt_sistema_software.md). El sistema seleccionado es la composición de tres contextos de red y el formato de mensajes en PC. La técnica produce IS-01: un recorrido de cinco transiciones del modelo de éxito, con precondiciones y resultados intermedios. No se presenta como cobertura de todo el sistema ni como FSM implementada en el firmware.

Los 16 unitarios siguen demostrando el punto 4. CFT se conserva como técnica complementaria de esos unitarios; no se requiere mostrar dos derivaciones completas en la defensa. No se amplía el alcance a radio/SPI, placas ni FreeRTOS. La prueba de integración se refuerza con comprobaciones en cada transición, sin aumentar la cantidad de casos.
