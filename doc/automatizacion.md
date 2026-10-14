# Automatización — referencia del profesor

## Referencia revisada

Se revisó [evolentini/26co-ejemplo, rama develop](https://github.com/evolentini/26co-ejemplo/tree/develop), commit `94effed9ca6372cc586a574c431e96a20ff6bf2b`.

La rama main consultada tiene validaciones de formato. La suite Ceedling, los tests de LEDs y su automatización están en develop. La referencia utiliza tests `test_*`, Unity, fixture `setUp`, `project.yml`, reportes nativos y GitHub Actions para PR hacia main.

## Qué se adapta a SIMAI Mesh

| Ejemplo del profesor | Aplicación en este proyecto |
|---|---|
| Unity y setUp | Fixtures independientes y aserciones en CP-01 a CP-09 e IS-01. |
| `ceedling clobber test:all` | Ejecución limpia de la suite de red. |
| `report_tests_log_factory` | JSON, JUnit y HTML nativos de Ceedling. |
| GitHub Actions sobre PR | Workflow en el repositorio principal para PR a main/develop. |
| Artefactos incluso ante errores | Reportes y logs recuperables en Actions durante siete días. |

Se conserva la imagen Docker fijada por digest y Ceedling 1.0.0 ya verificados. El ejemplo declara Ceedling 1.0.1; su patrón de automatización se aplica sin cambiar la herramienta de nuestra corrida. No se agregan mocks porque el flujo de red elegido no depende de hardware externo.

La consola muestra `Test 1: PASS` a `Test 17: PASS` con la suite ampliada, o FAIL/IGNORE según el resultado real. El número de integración se ajusta a los unitarios existentes. El runner propaga un fallo Unity incluso si Ceedling devuelve cero. No se reintroduce `summarize.py` ni un porcentaje obligatorio de cobertura.

## Reportes locales

Ceedling genera sus reportes en:

- `build/unit/artifacts/test/`: suite unitaria.
- `build/integration/artifacts/test/`: integración.

`build/` y `results/` están ignoradas por Git. Los logs y la cobertura de gcovr permanecen en `results/<fecha_hora>/`. La generación se realiza mediante plugins nativos, sin un script propio de resumen.

## GitHub Actions y acceso al firmware

El workflow se prepara en `.github/workflows/testing.yml` del repositorio **simai-mesh**, no en el repositorio público de testing. Actions obtiene el firmware mediante checkout del propio repositorio privado y descarga los submódulos; no necesita agregar un token para acceder al firmware desde el repositorio público de tests.

El job ejecuta el mismo comando reproducible que se usa localmente:

```bash
bash testing/scripts/run.sh
```

Se activa al publicar cambios en testing, main o develop, y en PR hacia main/develop que cambien firmware, testing o su configuración. También contempla ejecución manual. Usa solo permiso de lectura del contenido y no conecta placas. Los resultados se guardan como artefactos temporales de Actions; no se incorporan a Git ni se hace push desde el workflow. La carga de artefactos puede omitirse si se prefiere conservar únicamente la salida de consola.

**Estado:** configuración preparada localmente. La suite se verificó con Docker, pero no se afirma haber ejecutado el job en GitHub. Para habilitarlo hay que publicar, con autorización del usuario, los cambios del submódulo, registrar esa nueva referencia en simai-mesh y publicar el workflow. Las acciones usan el commit del submódulo registrado por el repositorio principal, no una actualización automática al último main.

Los hooks de formato/pre-commit del ejemplo no se incorporan en este cambio: el objetivo es automatizar la suite existente y sus reportes, no reformatear el firmware.

## Integración vinculada con STT

IS-01 implementa el script [S0 → S5 del modelo STT](casos_stt_sistema_software.md). Los comentarios T1–T5 y las aserciones intermedias verifican cada transición; ambos `pack/unpack` comparan todos los campos y el payload. El runner sigue ejecutando 16 unitarios y una integración, sin placa ni servicios de radio.
