# Testing de SIMAI Mesh

Planificación y pruebas de software de los módulos de comunicación de red y formato de mensajes de SIMAI Mesh.

## Alcance

- **Pruebas unitarias:** decisiones de entrega, rechazo y reenvío, gestión de rutas y contadores de comunicación de red.
- **Pruebas de integración:** recorrido sensor-puente-concentrador, límites del contenido, rechazos, cambios de rutas y mensajes inválidos, combinando comunicación de red y formato de mensajes.

Quedan fuera los drivers y periféricos, el control de enlace, el descubrimiento de nodos y la lógica de aplicación y tareas.

## Integración con el repositorio principal

Este repositorio se incorpora como submódulo `testing/`. La suite se ejecuta desde el repositorio principal, que contiene el firmware y la configuración de GitHub Actions.

La automatización utiliza Ceedling y Unity, realiza análisis estático y genera reportes separados de cobertura para unitarios e integración. Los resultados se muestran en los logs del pipeline y los reportes se guardan como artefactos descargables. La carpeta local `results/` está ignorada por Git.

## Resultados

| Suite | Tests PASS | Líneas | Ramas | Funciones |
|---|---:|---:|---:|---:|
| Unitarios | 16 | 90 % | 82,8 % | 86,7 % |
| Integración | 5 | 92,4 % | 78,6 % | 100 % |

La suite unitaria mide comunicación de red; la de integración mide comunicación de red y formato de mensajes. También se ejecutan análisis estático y regresión de red.

## Documentación

- [Master Test Plan](doc/planificacion.md): alcance, prioridades y estrategia de pruebas.
- [Casos unitarios y trazabilidad](doc/casos_cft.md).
- [Casos de integración](doc/casos_integracion.md).
- [Automatización](doc/automatizacion.md).
- [Ejecución y resultados](doc/ejecucion_y_entrega.md).
