CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g -Iinclude
LDFLAGS = -lm

SRCDIR = src
OBJDIR = obj
BINDIR = bin

MASTER_SOURCES = \
	$(SRCDIR)/master.c \
	$(SRCDIR)/libs/paramsHandler.c #(deberia ir un back slash aca)
# 	$(SRCDIR)/libs/shmCommon.c \
# 	$(SRCDIR)/libs/shmState.c \
# 	$(SRCDIR)/libs/shmSync.c

MASTER_OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MASTER_SOURCES))
MASTER_BIN = $(BINDIR)/master

.PHONY: all master clean
all: master

master: $(MASTER_OBJS)
	@mkdir -p $(BINDIR)
	$(CC) -o $(MASTER_BIN) $(MASTER_OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR) $(MASTER_BIN)
