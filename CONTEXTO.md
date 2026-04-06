# Contexto del Proyecto: ChompChamps

Este documento detalla el estado actual de la implementación realizada por el equipo para el Trabajo Práctico de Sistemas Operativos.

## 1. Arquitectura de Comunicación e IPCs
El proyecto se ha estructurado siguiendo el modelo **Proceso Maestro (Servidor)** y **Procesos Hijos (Clientes)**. La comunicación se basa en:
- **Memoria Compartida (SHM)**: Para el estado global del juego (`/game_state`) y la sincronización (`/game_sync`).
- **Semáforos Anónimos**: Ubicados dentro de la memoria compartida de sincronización para coordinar el acceso concurrente.
- **Pipes Anónimos**: El Master recibirá los movimientos de los jugadores a través de pipes (uno por jugador).

## 2. Estructuras de Datos (`include/structures.h`)
Es el corazón de la comunicación entre procesos. Se han definido tres estructuras críticas:

### `Player`
Representa a un jugador individual. Almacena su nombre, puntaje, estadísticas de movimientos (válidos/inválidos), coordenadas actuales (`x`, `y`), su `PID` y si está bloqueado. Es fundamental para que el Master y la Vista puedan rastrear el progreso de cada participante.

### `GameState`
Es el estado global almacenado en `/game_state`. Contiene las dimensiones del tablero, la cantidad de jugadores, un arreglo con la información de todos los jugadores (`Player players[CANT_PLAYERS]`), un flag de fin de juego y un puntero al tablero dinámico.
*   **Implementación del Tablero**: Se utiliza un arreglo flexible (`int board[]`) al final de la estructura para manejar tableros de distintos tamaños.

### `semaphoresStatus`
Almacena los semáforos anónimos para la sincronización:
- **Master-Vista**: `showNeeded` y `showDone` permiten una señalización bidireccional simple. El Master avisa que hay cambios y espera a que la Vista termine de imprimir.
- **Sincronización de Jugadores**: Se ha planteado el problema de **Lectores-Escritores con prioridad para el escritor (Master)** para evitar la inanición del Master al actualizar el estado. Se usan `masterMutex`, `gameStateMutex`, `nextVariableMutex` y un contador `playersReadingStatus`.
- **Movimientos**: Un arreglo de semáforos `playersAllowedToMove[CANT_PLAYERS]` para que el Master le dé permiso a cada jugador de enviar exactamente un movimiento.

## 3. Gestión de Parámetros (`src/libs/paramsHandler.c`)
Se ha implementado un sistema robusto y escalable para el Master que utiliza una tabla de parámetros (`Parameter parameters[]`).
- **Flexibilidad**: Soporta orden arbitrario de flags.
- **Manejo de Jugadores**: La flag `-p` es especial ya que puede recibir múltiples rutas de binarios de jugadores de forma consecutiva.
- **Defaults**: Existe una función `default_parameters()` que asegura que el juego pueda iniciar incluso si no se pasan todos los argumentos.

## 4. Abstracción de Memoria Compartida (`src/libs/shmCommon.c`)
Se crearon wrappers sobre las llamadas de sistema de POSIX SHM (`shm_open`, `mmap`, `ftruncate`). 
- **Objetivo**: Facilitar la creación y el mapeo de memoria en una sola operación (`createAndMap`) y el acceso por parte de los hijos (`openAndMap`). 
- *Nota: Actualmente hay una inconsistencia en los nombres de las funciones (ej. `createSHM` en los archivos de estado vs `createAndMap` en la librería) que debe corregirse.*

## 5. Visualización (`src/vista.c`)
La Vista ya cuenta con una lógica de renderizado avanzada:
- **Colores ANSI**: Se definió un vector de códigos de color para distinguir a cada jugador (fondo de color con texto contrastante).
- **Estadísticas**: Funciones para imprimir de forma tabular los puntos y coordenadas de cada jugador.
- **Limpieza de Pantalla**: Uso de códigos ANSI (`\033[2J\033[H`) para evitar parpadeos y mantener una interfaz limpia en la terminal.

## 6. Estado del Master y Jugadores
- **Master**: Actualmente solo maneja el parseo de entrada. El esqueleto está listo para comenzar con la creación de procesos y el bucle de juego con `select`.
- **Jugador**: Es un esqueleto que recibe las dimensiones del tablero. Está pendiente la implementación de la "IA" (envío automático de movimientos) y la conexión a la SHM.
