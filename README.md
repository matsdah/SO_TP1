# TP1 Sistemas Operativos (ChompChamps)

## Integrantes del Grupo

| Nombre                | Legajo    |         Mail                   |
|-----------------------|-----------|--------------------------------|
| Mateo Andrioli        | 64042     | mandrioi@itba.edu.ar           |
| Matías Sanchez        | 65741     | msancheznovelli@itba.edu.ar    |
| Francesco Vega        | 65199     | fvegascarabino@itba.edu.ar     |
| Santino Giambelluca   | 64697     | sgiambelluca@itba.edu.ar       |

## Compilación

El proyecto se compila utilizando Docker con la imagen de la cátedra `agodio/itba-so-multiarch:3.1`:

```bash
# Iniciar el contenedor Docker
docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multiarch:3.1

# Dentro del contenedor, compilar:
cd /root
make clean
make
```

Los binarios generados se encuentran en la carpeta `bin/`:
- `bin/master` - Proceso principal del juego.
- `bin/vista` - Proceso de visualizacion.
- `bin/jugador` - Proceso jugador con IA.

## Ejecución

### Sintáxis

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
# Juego con 2 jugadores y tablero de 15x10, con vista.
./bin/master -w 15 -h 10 -v ./bin/vista -p ./bin/jugador ./bin/jugador

# Juego con 4 jugadores, tablero de 20x30 y timeout de 30 segundos, con vista.
./bin/master -w 20 -h 15 -t 30 -v ./bin/vista -p ./bin/jugador ./bin/jugador ./bin/jugador ./bin/jugador

# Juego sin vista con tablerode 10x10 (solo resultados finales).
./bin/master -w 10 -h 10 -p ./bin/jugador ./bin/jugador
```

## Estructura del Proyecto

```
SO_TP1/
├── src/
│   ├── master.c              # Proceso principal
│   ├── vista.c               # Proceso de visualización
│   ├── jugador.c             # Proceso de jugador
│   └── libs/
│       ├── gameLogic.c       # Lógica del juego
│       ├── viewRender.c      # Renderizado de tablero
│       ├── playerAI.c        # Inteligencia artificial
│       ├── shmState.c        # Memoria compartida (estado)
│       ├── shmSync.c         # Memoria compartida (semaforos)
│       ├── shmCommon.c       # Funciones comunes de shm
│       └── paramsHandler.c   # Parseo de argumentos
├── inc/
│   ├── structures.h          # Estructuras de datos
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

## Limpieza

```bash
make clean
```

Esto elimina las carpetas `obj/` y `bin/`, junto con todos los archivos generados previamente.