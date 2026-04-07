# TODO - ChompChamps

## 1. Infraestructura y Estructuras de Datos
- [x] Definir estructura `Player` con nombre, puntaje, movimientos y estado de bloqueo.
- [x] Definir estructura `GameState` para el tablero y estado global.
- [x] Definir estructura `semaphoresStatus` con todos los semáforos anónimos requeridos.
- [x] Implementar sistema de parseo de parámetros para el Master (`-w`, `-h`, `-d`, `-t`, `-s`, `-v`, `-p`).
- [x] Corregir e implementar completamente la biblioteca de memoria compartida (`shmCommon.c`).
    - [x] Implementar `openAndMap`.
    - [x] Unificar nombres de funciones (actualmente `shmState.c` busca `createSHM` pero existe `createAndMap`).

## 2. Proceso Master (Servidor)
- [x] Crear e inicializar memoria compartida `/game_state`.
- [x] Crear e inicializar memoria compartida `/game_sync` (semáforos anónimos).
- [x] Crear canales de comunicación (Pipes anónimos) para recibir movimientos.
- [x] Implementar el "spawning" de procesos (Vista y Jugadores) usando `fork` y `exec`.
- [x] Implementar distribución inicial determinística de jugadores en el tablero.
- [x] Implementar el bucle principal de juego:
    - [x] Recibir movimientos mediante `select` sobre los pipes de los jugadores.
    - [x] Validar movimientos (evitar colisiones, límites del tablero, celdas ocupadas).
    - [ ] Actualizar el estado del juego y otorgar recompensas (1-9). (Eliminar funciones repetidas y agregarle la seed)
    - [ ] Notificar al jugador que su movimiento fue procesado.
    - [ ] Política de atención Round-Robin para solicitudes.
- [ ] Sincronización con la Vista:
    - [ ] Notificar cambios y esperar confirmación de impresión.
    - [ ] Respetar el `-d delay` tras cada actualización.
- [ ] Gestión de fin de juego:
    - [ ] Control de timeout `-t` (tiempo sin movimientos válidos).
    - [ ] Detectar cuando ningún jugador puede moverse.
    - [ ] Esperar a que los hijos terminen e imprimir sus estados de salida y puntajes.

## 3. Proceso Jugador (Cliente)
- [x] Recibir ancho y alto del tablero por parámetro.
- [ ] Conectarse a las memorias compartidas existentes.
- [ ] Implementar lógica de IA para envío automático de movimientos.
- [ ] Implementar sincronización Lectores-Escritores para consultar el estado del juego.
- [ ] Enviar movimientos (0-7) a través del pipe (STDOUT redirigido).
- [ ] Esperar permiso del Master para enviar el siguiente movimiento.

## 4. Proceso Vista (Interfaz)
- [x] Recibir ancho y alto del tablero por parámetro.
- [x] Implementar limpieza de pantalla mediante códigos ANSI.
- [x] Implementar lógica de impresión del tablero con colores para cada jugador.
- [x] Implementar impresión de estadísticas (puntajes, movimientos válidos/inválidos).
- [ ] Conectarse a las memorias compartidas existentes.
- [ ] Implementar sincronización con el Master para esperar actualizaciones.

## 5. General y Calidad
- [ ] Completar el `Makefile` para compilar los tres binarios (`master`, `vista`, `jugador`).
- [ ] Asegurar que el sistema esté libre de deadlocks y condiciones de carrera.
- [ ] Verificar ausencia de memory leaks con Valgrind.
- [ ] Completar el `README.md` con las secciones de diseño y ejecución requeridas.
- [ ] Limpieza de recursos al finalizar (shm_unlink, sem_destroy, close).
