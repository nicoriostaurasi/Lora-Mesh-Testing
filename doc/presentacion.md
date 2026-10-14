# Guion de defensa — cuatro puntos del TP

Versión actualizada: 10 de octubre de 2026. La presentación nativa conserva diez diapositivas y el estilo de referencia. Duración: diez minutos de exposición más unos cinco de preguntas. La distribución de tiempos siguiente es una propuesta; falta ensayo cronometrado.

## 1. Proyecto y alcance (1 minuto)

SIMAI Mesh: red de sensores LoRa. Se toma el comportamiento del mensaje sensor–puente–concentrador. Toda la ejecución de este TP ocurre en PC: tres contextos de red, datos determinísticos y serialización en memoria. No se ensayan radio, SPI, placa ni FreeRTOS.

## 2–3. Planificación MTP (2 minutos)

Características: funcionalidad 50 %, confiabilidad 35 %, mantenibilidad 15 %. Tabla A: importancia de características. Tabla B: prioridad de subsistemas incluidos, comunicación de red 80 % y formato 20 %. Tabla C: cruce de calidad con niveles, técnicas y momentos. Los porcentajes son prioridades de planificación, no cobertura ni reparto de horas. Fixtures son recursos, no subsistema. Los demás módulos están fuera de alcance, sin comprometer pruebas futuras.

## 4. Modelo STT del sistema de software (1 minuto)

Mostrar S0 nodos/rutas preparados → S1 mensaje creado → S2 decodificado en puente → S3 reenvío preparado → S4 decodificado en destino → S5 entrega aceptada. Son situaciones observables de un modelo de especificación del recorrido exitoso, no una nueva máquina de estados del firmware.

## 5. Derivación de IS-01 (1 minuto)

Recorrer T1–T5 y concretar las entradas y resultados esperados de [la tabla STT](casos_stt_sistema_software.md). Se obtiene una secuencia: construir, pack/unpack, preparar reenvío, pack/unpack y aceptar entrega. Cada paso tiene comprobaciones, incluyendo conservación de identidad/payload y contadores.

Se cubre el recorrido exitoso del modelo acotado. No se afirma cubrir eventos ilegales, pérdidas o recuperación del sistema completo. Los errores de contexto, salto, TTL y ruta se verifican en unitarios CFT documentados aparte.

## 6. Automatización (1 minuto)

Comando `./scripts/run.ps1`. Docker con imagen fijada, Ceedling/Unity, análisis GCC -fanalyzer y cobertura gcov/gcovr. Se compila el código productivo sin modificarlo. No se necesita conectar una placa.

## 7. Ejemplo unitario CP-06 (1 minuto)

Trama con TTL cero y ruta disponible: esperar TTL_EXPIRED, conservar trama e incrementar únicamente dropped_ttl. Mostrar preparación, llamada y aserciones. El ejemplo unitario demuestra el punto 4; el modelo STT demuestra el punto 3.

## 8–9. Ejecución y cobertura (1,5 minutos)

Mostrar extracto de logs reales y reporte. Suite: 16 unitarios y una integración. Consultar la corrida vigente y sus resultados en [ejecución y entrega](ejecucion_y_entrega.md). Los unitarios cubren 144/160 líneas (90 %), 101/122 ramas (82,8 %) y 13/15 funciones (86,7 %) de network_layer.c. prepare_forward: 25/25 líneas y 12/12 ramas (100 %). La cobertura de integración se informa por separado; no es MC/DC.

## 10. Conclusión (0,5 minutos)

Contexto acotado, MTP resumido, diseño STT aplicado al recorrido de software e implementación unitaria con evidencia. No se extrapolan los resultados a radio, consumo o hardware. Quedan límites de la suite y del modelo documentados. No se promete 100 % del sistema.

El CFT original se conserva en [casos_cft.md](casos_cft.md) como respaldo de los nueve casos iniciales, sin ocupar una segunda derivación completa en la exposición.
