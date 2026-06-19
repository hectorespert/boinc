# BOINC Client en Docker: necesidad de `--pid=host`

## Resumen
Cuando el cliente BOINC corre dentro de un contenedor Docker sin `--pid=host`, no puede ver los procesos del host en `/proc`. Como consecuencia, su lógica de detección de carga “no-BOINC” y de aplicaciones exclusivas se basa solo en procesos del propio contenedor, y deja de representar la carga real de la máquina.

## 1) Por qué no funciona correctamente sin `--pid=host` dentro de Docker

En Linux, BOINC obtiene información de procesos y CPU leyendo `procfs`:

- Enumeración de procesos: `opendir("/proc")` en `lib/procinfo_unix.cpp` (`procinfo_setup()`)
- Lectura de cada proceso: `/proc/<pid>/stat` en `lib/procinfo_unix.cpp`
- CPU total del sistema: `/proc/stat` en `lib/procinfo_unix.cpp` (`total_cpu_time()`)

La detección de carga en cliente se ejecuta en `ACTIVE_TASK_SET::get_memory_usage()` (`client/app.cpp`), llamada desde el loop principal en `client/client_state.cpp`.

Sin `--pid=host`, el contenedor tiene su propio namespace PID. Entonces:

- `/proc` contiene solo procesos del contenedor (normalmente BOINC y procesos hijos).
- BOINC no “ve” procesos de usuario del host (navegador, compilaciones, juegos, etc.).
- La métrica `non_boinc_cpu_usage` (`client/app.cpp`) se calcula con visibilidad parcial.
- Las reglas de suspensión por carga (`suspend_cpu_usage` / `niu_suspend_cpu_usage`) en `client/cs_prefs.cpp` pueden no activarse cuando deberían.

En términos prácticos: BOINC puede seguir computando aunque el host esté cargado, porque dentro del namespace del contenedor esa carga no existe.

## 2) Cómo BOINC client detecta la carga de otros programas

El flujo relevante es:

1. Cada ~10 s (`MEMORY_USAGE_PERIOD` en `client/client_state.h`), BOINC llama `active_tasks.get_memory_usage()`.
2. `procinfo_setup(pm)` arma un mapa de procesos (`PROC_MAP`) leyendo `/proc` (`lib/procinfo_unix.cpp`).
3. En `client/app.cpp` se marcan procesos BOINC/relacionados y se calcula CPU no-BOINC:
   - CPU total: `total_cpu_time()`
   - CPU BOINC-relacionada: `boinc_related_cpu_time(...)` (`lib/procinfo.cpp`)
   - CPU no-BOINC: diferencia entre ambas (campo global `non_boinc_cpu_usage`).
4. En `client/cs_prefs.cpp`, `check_suspend_processing()` compara `non_boinc_cpu_usage*100` contra preferencias de suspensión y decide si suspender.
5. También en `client/app.cpp`, BOINC revisa “exclusive apps” (`cc_config.exclusive_apps`) comparando nombres de comando del mapa de procesos (`app_running(pm, ...)`).

Conclusión: toda esta lógica depende de una vista global y correcta de procesos del sistema. Si `/proc` está aislado por namespace PID, la detección se degrada.

## 3) Alternativas dentro de un contenedor

### A. Usar `--pid=host` (opción recomendada para comportamiento equivalente al host)

Ventajas:
- Mantiene intacta la lógica actual de BOINC.
- Permite ver procesos reales del host en `/proc`.

Desventajas:
- Reduce aislamiento del contenedor (exposición de metadata de procesos del host).

### B. Mantener namespace aislado y desactivar dependencia de “carga externa”

Opciones operativas:
- No usar `suspend_cpu_usage` / `niu_suspend_cpu_usage`.
- No usar reglas `exclusive_app`/`exclusive_gpu_app`.
- Controlar BOINC por política externa (CPU quota/cpuset de Docker, horarios, orquestador).

Ventajas:
- Conserva aislamiento del namespace PID.

Desventajas:
- BOINC ya no adapta su ejecución a programas externos del host; la coordinación pasa a infraestructura externa.

### C. Exponer señales externas al contenedor (arquitectura personalizada)

Diseñar un sidecar/agente en host que mida carga real y comunique estado al cliente (p. ej. vía RPC o configuración dinámica).

Ventajas:
- Aislamiento de PID + señal más representativa del host.

Desventajas:
- Mayor complejidad operativa y de integración.
- Requiere desarrollo/mantenimiento adicional fuera del flujo estándar de BOINC.

## Conclusión
Para el comportamiento clásico del cliente BOINC (detectar carga de otros programas del host y suspender/reanudar en consecuencia), en Docker se necesita `--pid=host`. Sin ello, BOINC opera con una visión parcial del sistema y las decisiones basadas en “non-BOINC CPU load” y “exclusive apps” pueden ser incorrectas.
