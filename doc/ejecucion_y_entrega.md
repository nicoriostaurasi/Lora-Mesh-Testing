# Ejecución y entrega del TP

## Estado actual

La documentación de planificación y casos está preparada. Los tests de red existentes usan `assert` y un script de compilación C; no se presenta esa suite como si ya fuera Ceedling. No se ejecutaron tests ni se midió cobertura durante la creación de estos documentos.

## Automatización prevista

1. Crear una configuración Ceedling aislada para TSSE, conservando los scripts host existentes.
2. Compilar el archivo productivo `firmware/components/network_layer/network_layer.c` directamente; evitar una copia que pueda divergir.
3. Incluir las cabeceras de `network_layer`, `mesh_frame` y la cabecera host `esp_err.h`. La suite unitaria de preparación del reenvío no necesita compilar la radio ni ESP-IDF; las operaciones de red utilizadas en el fixture pertenecen al mismo componente.
4. Crear funciones `test_*` con Unity para CP-01 a CP-09, fixture limpio en cada test y verificaciones de errores, campos y estadísticas.
5. Si se incluye IS-01, agregar `mesh_frame.c` a una ejecución de integración diferenciada. No introducir mocks sin una dependencia externa que deba aislarse.
6. Ejecutar la suite mediante `ceedling test:all` desde su carpeta de configuración, una vez creada y validada. Este comando es el flujo previsto, no evidencia de una configuración disponible actualmente.
7. Habilitar instrumentación y reporte de cobertura según la versión instalada de Ceedling/gcov/gcovr. Registrar el comando real validado y sus versiones; no fijar una tarea de cobertura antes de disponer de esa configuración.

El entorno previsto sigue el flujo Docker del proyecto. La instalación/configuración concreta de Ceedling queda pendiente. No se requiere desarrollar el componente nuevamente con TDD.

## Qué medir y guardar

- Resultado PASS/FAIL de cada caso, con su identificador CP.
- Advertencias o errores de compilación y discrepancias funcionales.
- Cobertura de líneas y ramas de `network_layer.c`, con numerador/denominador o porcentaje y herramienta utilizada.
- Líneas/ramas relevantes sin recorrer y explicación de los límites del alcance.
- Commit ensayado, cambios locales incluidos, fecha, entorno, versiones y comandos.
- Capturas o video de la corrida y reporte legible para la presentación.

No confundir los siete caminos CFT del flujo seleccionado con el porcentaje de cobertura de todo el archivo, ni cobertura de código con tasa de entrega de paquetes.

## Registro de ejecución para completar

| Dato | Valor |
|---|---|
| Fecha de ejecución | Pendiente |
| Commit y modificaciones locales | Pendiente |
| Entorno y versiones | Pendiente |
| Comando de ejecución | Pendiente de configuración |
| CP-01 a CP-09 | No ejecutados con Ceedling |
| IS-01 | Opcional; ejecución nueva pendiente |
| Cobertura de líneas/ramas | No medida |
| Defectos y excepciones | Pendiente de revisión y ejecución |
| Evidencia (capturas/video/reporte) | Pendiente |

Crear un registro nuevo por corrida relevante; conservar resultados fallidos y su resolución. No reemplazar «pendiente» por PASS a partir de un resultado de otra versión o entorno.

## Guion sugerido de presentación

1. Problema real: red LoRa y camino sensor-puente-concentrador; datos simulados y límites actuales.
2. Alcance de TSSE y requerimientos RT.
3. Planificación: tablas A, B y C, prioridades y exclusiones justificadas.
4. CFT: diagrama y profundidad elegida.
5. Caminos y casos físicos: mostrar entradas y resultados, incluidos TTL 0/1.
6. Código y estructura de los tests en Ceedling.
7. Corrida y cobertura; qué se comprobó y qué falta cubrir.
8. Conclusiones y evidencia previa de banco como complemento opcional, con fecha y versión.

La cantidad de diapositivas y duración se ajustan al tiempo que indique la cátedra. El entregable es la presentación; estos documentos son su base de trabajo.

## Checklist de cierre

- [x] Proyecto real y alcance definidos.
- [x] Características de calidad ponderadas.
- [x] Tipos, niveles, responsables y momentos definidos.
- [x] Distribución de esfuerzo y exclusiones justificadas.
- [x] Técnica CFT y casos concretos documentados.
- [ ] Confirmar el contrato de TTL y entrega local con el autor.
- [ ] Registrar revisión estática y advertencias.
- [ ] Configurar Ceedling y automatizar CP-01 a CP-09.
- [ ] Ejecutar y documentar resultados.
- [ ] Medir y explicar cobertura.
- [ ] Preparar evidencia y presentación final.

## Referencias adicionales

- Script de pruebas host actual (`firmware/scripts/test_network_layer_host.sh`).
- Plan de sistema multi-salto (`firmware/docs/tests/multihop_test_plan.md`).
- Plan de cambio de conectividad (`firmware/docs/tests/connectivity_change_test_plan.md`).
- Resumen histórico de resultados (`firmware/docs/tests/results_summary.md`).
