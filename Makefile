CC      = gcc
WARN    = -W -Wall

.PHONY: all nuts

all: nuts

nuts: nuts.o
	$(CC) $(WARN) -o nuts nuts.c

clean:
	rm nuts *.o

