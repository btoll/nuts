CC      = gcc
WARN    = -W -Wall

.PHONY: all hideaway

all: hideaway

hideaway: hideaway.o
	$(CC) $(WARN) -o hideaway hideaway.c

clean:
	rm hideaway *.o

