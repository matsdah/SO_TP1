CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g -Iinc
LDFLAGS = -lm -pthread -lrt

SRCDIR = src
OBJDIR = obj
BINDIR = bin

LIB_SOURCES = \
	$(SRCDIR)/libs/paramsHandler.c \
	$(SRCDIR)/libs/shmCommon.c \
	$(SRCDIR)/libs/shmState.c \
	$(SRCDIR)/libs/shmSync.c \
	$(SRCDIR)/libs/gameLogic.c \
	$(SRCDIR)/libs/playerAI.c \
	$(SRCDIR)/libs/viewRender.c

MASTER_SOURCES = \
	$(SRCDIR)/master.c \
	$(SRCDIR)/libs/paramsHandler.c \
	$(SRCDIR)/libs/shmCommon.c \
	$(SRCDIR)/libs/shmState.c \
	$(SRCDIR)/libs/shmSync.c \
	$(SRCDIR)/libs/gameLogic.c

VISTA_SOURCES = \
	$(SRCDIR)/vista.c \
	$(SRCDIR)/libs/shmCommon.c \
	$(SRCDIR)/libs/shmState.c \
	$(SRCDIR)/libs/shmSync.c \
	$(SRCDIR)/libs/viewRender.c

JUGADOR_SOURCES = \
	$(SRCDIR)/jugador.c \
	$(SRCDIR)/libs/shmCommon.c \
	$(SRCDIR)/libs/shmState.c \
	$(SRCDIR)/libs/shmSync.c \
	$(SRCDIR)/libs/playerAI.c

MASTER_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MASTER_SOURCES))
VISTA_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(VISTA_SOURCES))
JUGADOR_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(JUGADOR_SOURCES))

MASTER_BIN = $(BINDIR)/master
VISTA_BIN = $(BINDIR)/vista
JUGADOR_BIN = $(BINDIR)/jugador

.PHONY: all master vista jugador clean
all: master vista jugador

master: $(MASTER_OBJS)
	@mkdir -p $(BINDIR)
	$(CC) -o $(MASTER_BIN) $(MASTER_OBJS) $(LDFLAGS)

vista: $(VISTA_OBJS)
	@mkdir -p $(BINDIR)
	$(CC) -o $(VISTA_BIN) $(VISTA_OBJS) $(LDFLAGS)

jugador: $(JUGADOR_OBJS)
	@mkdir -p $(BINDIR)
	$(CC) -o $(JUGADOR_BIN) $(JUGADOR_OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR) $(BINDIR)
