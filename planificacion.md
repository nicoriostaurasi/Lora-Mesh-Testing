# Planificación de pruebas

## 1. Proyecto y alcance

SIMAI Mesh es un prototipo de red de sensores LoRa sobre Heltec WiFi LoRa 32 V3 (ESP32-S3 y SX1262), con pila propia y reenvío multi-salto. El escenario de referencia es `sensor (1) -> puente (2) -> concentrador (3)`. Los datos de sensor actuales son simulados.

**Objetivo del alcance ejecutable:** verificar que la capa de red decide correctamente entre entrega local, reenvío y rechazo; preserva los datos y actualiza campos y contadores según el contrato propuesto.

**Incluido en la ejecución de TSSE:** pruebas unitarias de `network_layer_prepare_forward()` y sus dependencias internas, usando las APIs públicas para inicializar y cargar rutas; cobertura del archivo `network_layer.c`. Como extensión, integración de software con `mesh_frame` para el recorrido de tres nodos.

**Fuera de la ejecución comprometida:** radio/SPI reales, tareas FreeRTOS, sensores físicos, OLED/LED, alcance RF, consumo, estabilidad de varias horas y recuperación mediante caminos alternativos reales. La selección de rutas se usa como apoyo del reenvío; una caracterización exhaustiva de sus desempates y capacidad de tabla queda para una ampliación.

## 2. Bases de prueba

| Base | Uso |
|---|---|
| [API de red](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/components/network_layer/include/network_layer.h) | Tipos, errores, configuración y contadores públicos. |
| [Implementación de red](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/components/network_layer/network_layer.c) | Construcción del flujo CFT y contraste con el contrato. |
| [Descripción de red](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/docs/protocol/network_layer.md) | Selección de rutas, TTL y escenario de tres nodos. |
| [Formato de trama](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/docs/protocol/frame_format.md) | Campos y serialización para integración. |
| [Tests host existentes](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/test/host/test_network_layer/test_network_layer.c) | Escenarios reutilizables; no constituyen evidencia Ceedling. |
| [Resultados previos](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/docs/tests/results_summary.md) | Contexto de ensayos físicos anteriores. |
| [Limitaciones](https://github.com/SIMAI-AGRO/simai-mesh/blob/develop/firmware/docs/known_limitations.md) | Límites de las conclusiones. |

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

### Tabla B — Características, niveles, tipos y momento

`++`: énfasis principal del nivel; `+`: aporte parcial; `—`: fuera del alcance de ese nivel. Estas marcas no prometen cobertura completa de toda la característica.

| Nivel | Funcionalidad | Confiabilidad | Mantenibilidad | Tipo/objetivo | Momento y responsable | Estado para TSSE |
|---|---|---|---|---|---|---|
| Unitario en PC | ++ | ++ | + | Pruebas funcionales positivas/negativas y regresión de red | Autor del TP, tras diseñar los casos y ante cambios | Ejecución comprometida con Ceedling. |
| Integración de software en PC | ++ | + | + | Red + serialización de tramas; preservación de datos | Autor del TP, tras los unitarios | Extensión propuesta; existe un escenario host previo. |
| Integración HW/SW en banco | + | + | — | SPI, radio y firmware reales | Autor del proyecto, después del build del target | Fuera de ejecución TSSE; evidencia histórica complementaria. |
| Sistema en banco | ++ | + | — | Entrega sensor-puente-concentrador y pérdida de puente | Autor del proyecto, con perfiles y versión registrados | Fuera de ejecución TSSE; hay resultados registrados. |
| Campo | + | ++ | — | Distancia, obstáculos, interferencia y estabilidad | Autor del proyecto, en una campaña posterior | Diferido; no se demuestra con simulación. |

La regresión reutiliza los tests luego de cada modificación relevante. No se exige implementar CI para este alcance.

### Tabla C — Subsistemas y distribución de esfuerzo

Estimación del esfuerzo de diseño y ejecución del alcance de software: no son tiempos observados. El porcentaje asignado a módulos de apoyo incluye preparar fixtures y verificar su interacción; no implica una suite propia completa para cada módulo.

| Subsistema | Esfuerzo | Funcionalidad | Confiabilidad | Mantenibilidad | Técnica/actividad |
|---|---:|---|---|---|---|
| `network_layer` | 70 % | ++ | ++ | + | CFT nivel 1 sobre preparación del reenvío y casos de borde. |
| `mesh_frame` | 20 % | ++ | + | + | Integración opcional pack/unpack del recorrido de tres nodos. |
| Fixtures de trama y muestra (`sensor_app` como contexto) | 10 % | + | + | + | Preparar payload determinístico y comprobar conservación. |
| Total | 100 % | | | | |

`link_layer` (ACK/reintentos), `neighbor_discovery` (expiración), driver SX1262 y runtimes son importantes para el proyecto completo, pero no reciben ejecución nueva en esta entrega. Se reutilizan sus documentos para contextualizar el sistema. Los datos simulados no requieren probar adquisición de un sensor real.

## 5. Recursos y preparación

- Responsable: autor del TP/proyecto.
- PC y entorno Docker del repositorio como entorno previsto; sin placas para la suite comprometida.
- Compilador C del host, Ceedling/Unity y herramientas gcov/gcovr compatibles entre sí; registrar las versiones efectivamente utilizadas.
- Código productivo existente y cabecera host `firmware/test/host/include/esp_err.h`, sin arrastrar ESP-IDF ni FreeRTOS para el componente puro.
- Análisis estático del componente como revisión de entrada recomendada por el material MTP: registrar hallazgos y correcciones o excepciones justificadas. No confundirlo con ejecución dinámica.

## 6. Secuencia de trabajo y criterios

1. Confirmar RT-01 a RT-09 y fijar la versión de código bajo prueba, incluyendo cambios locales si existen.
2. Revisar advertencias de compilación y análisis estático; registrar problemas que bloqueen la ejecución.
3. Completar el diagrama y la tabla CFT.
4. Implementar los casos en Ceedling, reutilizando escenarios existentes cuando corresponda.
5. Ejecutar suite y cobertura; corregir defectos o informar discrepancias.
6. Ejecutar integración opcional si se incorpora al alcance y registrar evidencia.
7. Preparar presentación, capturas/video y conclusiones limitadas a lo ensayado.

**Entrada:** contrato acordado, código identificable, entorno reproducible y fixtures independientes entre tests.

**Salida de la suite:** todos los casos comprometidos ejecutados, resultados comparados con el contrato, errores/discrepancias documentados y reporte de cobertura disponible. Se busca recorrer todas las salidas del flujo elegido, pero no se promete 100 % del archivo completo. Toda rama relevante no cubierta debe explicarse. No existe un umbral mínimo oficial de cobertura en la consigna oral.

**Cierre del TP:** además de la suite, presentación que muestre las tres etapas. Este documento por sí solo no acredita ese cierre.

## 7. Evidencia previa y límites

El resumen del repositorio registra para 2026-08-26 y commit `db5fbb0` una prueba multi-salto de 49/50 mensajes, 98 % de entrega y latencia máxima de 795 ms. Registra también pérdida del puente sin alternativa de ruta. Son resultados históricos documentados, no pruebas ejecutadas al crear este módulo ni validación automática de la copia de trabajo actual.

La aceptación física del proyecto usa criterios propios (por ejemplo entrega y latencia). Esos criterios no se trasladan a tests unitarios PC ni a un porcentaje de cobertura. Las rutas de la demostración son estáticas o derivadas de vecinos; no se acredita descubrimiento distribuido completo ni reselección física entre caminos alternativos.
