# Testing de SIMAI Mesh — Trabajo final de TSSE

**Versión:** B, 2026-10-07. **Estado:** planificación, casos y automatización implementados; suite de software verificada. La defensa oral sigue a cargo del autor.

## Propósito

Este repositorio aplica las tres etapas del TP de Testing en Sistemas Embebidos a SIMAI Mesh: Master Test Plan, generación de casos con CFT y automatización con Ceedling. Se ensaya el código productivo de la capa de red, sin copiar el firmware ni conectar placas.

## Documentación

- [Master Test Plan](doc/planificacion.md): alcance, riesgos, características de calidad, niveles, esfuerzo y criterios.
- [Diseño CFT y trazabilidad](doc/casos_cft.md): siete caminos y nueve casos unitarios.
- [Ejecución y entrega](doc/ejecucion_y_entrega.md): entorno, instrucciones, resultados y checklist.
- [Presentación en Markdown](doc/presentacion.md): ocho diapositivas con las tres etapas y las conclusiones.
- [Evidencia de la corrida](results/2026-10-07_verificacion_03/resumen.md): resultados, cobertura, logs y hashes.

## Ejecución rápida

Requisitos: Docker Desktop con motor Linux activo (o Docker en Linux), Git y acceso a un clon del firmware SIMAI Mesh. El repositorio de testing es público; no incluye el firmware privado y por eso no se puede ejecutar sin ese clon.

Desde `testing`, en PowerShell:

```powershell
./scripts/run.ps1
```

Con testing clonado por separado:

```powershell
./scripts/run.ps1 -SimaiMeshRoot 'C:/ruta/al/simai-mesh'
```

Si Git no está en PATH, pasar `-GitCommand` con la ruta al ejecutable. Cada corrida crea una carpeta nueva de evidencia y falla si ese identificador ya existe.

En Linux:

```bash
bash scripts/run.sh
# Clon independiente:
SIMAI_MESH_ROOT=/ruta/al/simai-mesh bash scripts/run.sh
```

Se usa una imagen oficial de ThrowTheSwitch fijada por digest, con Ceedling 1.0.0, Unity 2.6.1, GCC/gcov 12.2.0 y gcovr 5.2. El firmware se monta en modo de solo lectura. El runner ejecuta análisis estático, tests unitarios, integración, cobertura separada y la suite host de red anterior. No modifica el firmware.

## Resultados del alcance

- CP-01 a CP-09: 9 PASS, sin fallos ni casos ignorados.
- IS-01: 1 PASS, recorrido sensor-puente-concentrador con serialización real.
- Análisis GCC `-fanalyzer`: sin diagnósticos para red y formato con advertencias tratadas como errores.
- `network_layer_prepare_forward()`: 25/25 líneas y 12/12 ramas tomadas, 100 % en el flujo elegido.
- Todo `network_layer.c`, suite unitaria: 84/160 líneas (52,5 %) y 44/122 ramas (36,1 %).

La cobertura completa del flujo seleccionado no equivale a validar toda la capa de red. El banco con placas, SPI, RF, tareas FreeRTOS, sensores físicos y campo quedan fuera de esta ejecución. Los resultados históricos de hardware son contexto, no resultados nuevos.

## Integración Git

Este repositorio se incorpora como submódulo `testing/` de SIMAI Mesh.

```bash
git submodule update --init --recursive
cd testing
git switch main
```

Después de publicar cambios aquí, desde la raíz de SIMAI Mesh:

```bash
git -C testing pull --ff-only origin main
git add testing
git commit -m "Update testing submodule"
```

La configuración toma `SIMAI_MESH_ROOT` para ubicar el firmware. La evidencia registra commit, estado del firmware, versiones y hashes SHA-256 de las fuentes, pruebas y configuraciones ensayadas. Los reportes públicos contienen métricas y nombres, sin publicar el código productivo.

## Fuentes de la consigna y herramientas

Consigna oral reconstruida de `tp_final_tsse.txt` y material del curso de MTP, generación de casos y Ceedling. El MTP está acotado a TSSE y no afirma certificar todo el proyecto de grado.

Referencias técnicas: [Ceedling, ThrowTheSwitch](https://github.com/ThrowTheSwitch/Ceedling), [imagen oficial y ejecución Docker](https://github.com/ThrowTheSwitch/Ceedling#docker) y [gcovr](https://gcovr.com/en/5.2/).
