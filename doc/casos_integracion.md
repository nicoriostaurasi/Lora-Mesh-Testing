# Casos de integración de comunicación de red y formato de mensajes

Actualización: 13 de octubre de 2026. Máximo acordado: cinco funciones de test de integración. No se modifica el código productivo ni se utilizan placas.

## Alcance y casos

Los tests compilan juntos `network_layer.c` y `mesh_frame.c`. Cada caso conecta decisiones de red con construcción, codificación o decodificación de mensajes. Hay cinco funciones de test; algunos casos recorren variantes con fixtures nuevos o cambios controlados de rutas. Cinco funciones no significa cinco llamadas ni cinco aserciones.

| Caso | Escenario | Resultados esperados |
|---|---|---|
| IS-01 | Sensor → puente → concentrador, derivado del modelo STT. | Cada etapa S0/T1–T5 es correcta; identidad y contenido intactos; TTL 4 → 3, saltos 0 → 1; contadores de creación, reenvío y entrega. |
| IS-02 | Mensajes confirmados de 0 y 64 bytes; último reenvío con TTL 1; ID al superar 65535. | Ambos recorridos llegan íntegros; entrega local con TTL 0; ID vuelve a 1; cabecera y contadores correctos. El fixture se reinicia entre tamaños. |
| IS-03 | Mensajes decodificados en puente: salto incorrecto, TTL agotado, ruta ausente y contexto no inicializado. | Error específico, ningún reenvío ni entrega; al recodificar, bytes idénticos a la entrada; solo aumenta el descarte esperado. Cada variante usa contextos nuevos. |
| IS-04 | Retirar ruta, usar alternativa, llenar tabla y agregar otra ruta; actualizar prioridades por saltos, entrega, RSSI y SNR. | El próximo salto elegido se refleja en mensajes codificados y entregados; se reemplaza la peor ruta al llenar la tabla; al retirar todas las rutas al destino se rechaza construir. Reiniciar estadísticas no altera el ID. |
| IS-05 | Mensaje construido por red: buffer de salida insuficiente, longitud recibida inválida, versión/magic incorrectos, contenido excesivo y CRC alterado. | El formato rechaza cada entrada con error y diagnóstico esperado; salida sin publicar; puente sin procesamiento. Tras corregir el tamaño, codificación/decodificación correcta. |

La preparación/carga de rutas es controlada por el ensayo; IS-04 no demuestra descubrimiento automático de rutas ni recuperación RF. Los errores de recepción se verifican antes de entregar una trama a la red. No se afirma que exista un runtime de recepción ensayado aquí: se prueban los contratos de los dos módulos y su composición.

## Ejecución verificada

Corrida: `results/`.

- 16 unitarios PASS y 5 integraciones PASS; sin fallos ni casos ignorados.
- Integración conjunta: **232/251 líneas (92,4 %)**, **132/168 ramas (78,6 %)** y **21/21 funciones (100 %)**.
- Comunicación de red, dentro de integración: 148/160 líneas (92,5 %), 96/122 ramas (78,7 %).
- Formato de mensajes, dentro de integración: 84/91 líneas (92,3 %), 36/46 ramas (78,3 %).
- Unitarios: mantienen 144/160 líneas (90 %), 101/122 ramas (82,8 %) y 13/15 funciones (86,7 %).
- Análisis estático y regresión host PASS.

Se superó el objetivo del 90 % de **líneas**, no el 90 % de ramas. Los denominadores incluyen los archivos completos; no se fusionan reportes ni se excluyen líneas para mejorar porcentajes.

Quedan sin recorrer algunas validaciones de argumentos nulos/inválidos y rutas de diagnóstico, entre otros caminos. La cobertura no demuestra todas las combinaciones posibles ni ausencia de defectos. STT sigue mostrando solamente la derivación de IS-01; los casos adicionales amplían integración mediante límites, errores y cambios de estado de la tabla de rutas.

## Repetición

```powershell
./scripts/run.ps1
```

El pipeline muestra los unitarios como TEST 01–16 y la integración como TEST 17–21, con los nombres reales. Reportes completos en `integration.log`, `integration_coverage.html`, `integration_coverage.json` e `integration_summary.json`. `results/` permanece ignorada por Git.
