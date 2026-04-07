# ChompChamps - TP1 Sistemas Operativos

## Integrantes del Grupo

| Nombre              | Legajo  |
|---------------------|---------|
| [Nombre Apellido 1] | [12345] |
| [Nombre Apellido 2] | [12346] |
| [Nombre Apellido 3] | [12347] |

## Compilacion

El proyecto se compila utilizando Docker con la imagen `agodio/itba-so-multiarch:3.1`:

```bash
# Iniciar el contenedor Docker
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multiarch:3.1

# Dentro del contenedor, compilar:
cd /root
make clean
make
```

Los binarios generados se encuentran en el directorio `bin/`:
- `bin/master` - Proceso principal del juego
- `bin/vista` - Proceso de visualizacion
- `bin/jugador` - Proceso jugador con IA

## Ejecucion

### Sintaxis

```bash
./bin/master [opciones]
```

### Opciones

| Flag | Descripcion                           | Valor por defecto |
|------|---------------------------------------|-------------------|
| `-w` | Ancho del tablero                     | 10                |
| `-h` | Alto del tablero                      | 10                |
| `-d` | Delay entre turnos (ms)               | 200               |
| `-t` | Timeout de inactividad (segundos)     | 10                |
| `-s` | Semilla para generacion aleatoria     | 67                |
| `-v` | Path al ejecutable de la vista        | (ninguno)         |
| `-p` | Paths a los ejecutables de jugadores  | (ninguno)         |

### Ejemplos

```bash
# Juego con 2 jugadores, tablero 15x10, con vista
./bin/master -w 15 -h 10 -v ./bin/vista -p ./bin/jugador ./bin/jugador

# Juego con 4 jugadores, timeout de 30 segundos
./bin/master -w 20 -h 15 -t 30 -v ./bin/vista -p ./bin/jugador ./bin/jugador ./bin/jugador ./bin/jugador

# Juego sin vista (solo resultados finales)
./bin/master -w 10 -h 10 -p ./bin/jugador ./bin/jugador
```

## Arquitectura

### Procesos

1. **Master**: Proceso servidor que:
   - Inicializa la memoria compartida y semaforos
   - Crea y gestiona procesos jugadores y vista
   - Valida y aplica movimientos
   - Detecta condiciones de fin de juego
   - Limpia recursos al finalizar

2. **Vista**: Proceso cliente que:
   - Se conecta a la memoria compartida
   - Renderiza el estado del juego en terminal
   - Muestra estadisticas de jugadores

3. **Jugador**: Proceso cliente que:
   - Se conecta a la memoria compartida
   - Implementa IA greedy (busca celda adyacente de mayor valor)
   - Comunica movimientos al master via pipe

### Comunicacion IPC

- **Memoria compartida POSIX**: 
  - `/game_state` - Estado del juego (tablero, jugadores, flags)
  - `/game_sync` - Semaforos de sincronizacion

- **Pipes anonimos**: Comunicacion jugador -> master (movimientos)

- **Semaforos POSIX**:
  - `masterMutex` / `gameStateMutex` - Patron readers-writers
  - `readerCount` - Contador de lectores activos
  - `printNeeded` / `renderDone` - Sincronizacion con vista
  - `playersAllowedToMove[]` - ACK por jugador

### Sincronizacion

Se implementa un patron **readers-writers** con prioridad al escritor (master):

- **Lectores** (vista, jugadores): Pueden leer concurrentemente usando `acquireReadLock()` / `releaseReadLock()`
- **Escritor** (master): Tiene acceso exclusivo durante actualizaciones

## Estructura del Proyecto

```
SO_TP1/
├── src/
│   ├── master.c          # Proceso principal
│   ├── vista.c           # Proceso de visualizacion
│   ├── jugador.c         # Proceso jugador
│   └── libs/
│       ├── gameLogic.c   # Logica del juego
│       ├── viewRender.c  # Renderizado de tablero
│       ├── playerAI.c    # Inteligencia artificial
│       ├── shmState.c    # Memoria compartida (estado)
│       ├── shmSync.c     # Memoria compartida (semaforos)
│       ├── shmCommon.c   # Funciones comunes shm
│       └── paramsHandler.c # Parseo de argumentos
├── inc/
│   ├── structures.h      # Estructuras de datos
│   ├── gameLogic.h
│   ├── viewRender.h
│   ├── playerAI.h
│   ├── shmState.h
│   ├── shmSync.h
│   ├── shmCommon.h
│   └── paramsHandler.h
├── Makefile
└── README.md
```

## Limitaciones Conocidas

- Maximo 9 jugadores simultaneos
- El tablero debe tener dimensiones minimas de 1x1
- Requiere terminal con soporte para colores ANSI (256 colores)

## Limpieza

```bash
make clean
```

Esto elimina los directorios `obj/` y `bin/` con todos los archivos generados.
