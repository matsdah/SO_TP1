CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -g

all: master vista jugador

master: src/master.c
	$(CC) $(CFLAGS) -o master src/master.c -lrt -lpthread

vista: src/vista.c
	$(CC) $(CFLAGS) -o vista src/vista.c -lrt -lpthread

jugador: src/jugador.c
	$(CC) $(CFLAGS) -o jugador src/jugador.c -lrt -lpthread -lm

clean:
	rm -f master vista jugador *.o