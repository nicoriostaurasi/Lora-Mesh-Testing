# Testing de SIMAI Mesh — Trabajo final de TSSE

**Versión:** A. **Estado:** planificación y diseño de casos; ejecución con Ceedling y medición de cobertura pendientes.

## Propósito y relación con la consigna

Este módulo documental adapta el trabajo final de Testing en Sistemas Embebidos al proyecto SIMAI Mesh. Cubre la planificación y deja definido el diseño de casos y el trabajo de automatización necesario. No declara terminado el TP: falta implementar/ejecutar los tests con Ceedling, medir cobertura y preparar la presentación.

La consigna oral exige tres etapas sobre el mismo proyecto: planificación general, generación de casos mediante una técnica del curso y automatización de algunas pruebas con Ceedling. El entregable es una presentación con resultados, cobertura y evidencia de ejecución. TDD y la ejecución en vivo son opcionales; se aceptan video o capturas. No se especifica un porcentaje mínimo de cobertura ni una duración exacta.

## Índice

- [Planificación de pruebas](planificacion.md): alcance, base de prueba, riesgos, prioridades, niveles, esfuerzo y criterios.
- [Diseño mediante CFT](casos_cft.md): flujo, caminos, entradas y resultados esperados.
- [Ejecución y presentación](ejecucion_y_entrega.md): adaptación a Ceedling, registro de resultados y checklist de entrega.

## Decisión de alcance

La ejecución de este TP se concentra en software en PC, sobre `network_layer`, con un escenario de sensor 1, puente 2 y concentrador 3. No requiere conectar placas. Los ensayos físicos existentes son evidencia complementaria de versiones anteriores y no sustituyen la ejecución con Ceedling.

Las pruebas en banco con placas y en campo aparecen como contexto del plan del proyecto. No se compromete su repetición para esta entrega. La simulación verifica lógica; no demuestra funcionamiento de SPI, radio real, propagación RF ni tiempos físicos.

## Fuentes

Consigna reconstruida del diálogo `tp_final_tsse.txt`; presentaciones de MTP, generación de casos y Ceedling proporcionadas para la materia. Como base del proyecto se usan código, contratos y documentos referenciados en los archivos de este módulo. Los requerimientos de prueba definidos aquí son una especificación propuesta para el alcance de TSSE, a confirmar con el autor del proyecto, no una transcripción literal del anteproyecto.

## Uso como repositorio independiente o submódulo

Este repositorio se integra como `testing/` dentro de SIMAI Mesh. La documentación también se puede consultar desde un clon independiente.

```bash
git clone https://github.com/nicoriostaurasi/Lora-Mesh-Testing.git
```

Para obtener los submódulos desde SIMAI Mesh:

```bash
git submodule update --init --recursive
```

Para trabajar en una rama dentro del submódulo:

```bash
cd testing
git switch main
```

Después de publicar cambios en este repositorio, actualizar y registrar su referencia desde la raíz de SIMAI Mesh:

```bash
git -C testing pull --ff-only origin main
git add testing
git commit -m "Update testing submodule"
```

La futura suite Ceedling deberá recibir una ruta explícita al clon del firmware (por ejemplo `SIMAI_MESH_ROOT`) y registrar el commit y los cambios locales ensayados. Actualmente esa configuración no está implementada y no se incluye una copia del firmware en este repositorio.
