CC      = gcc
FLAGS	= -W -Wall
OUT		= nuts

.PHONY: build

build: $(OUT)

$(OUT): $(OUT).o
	$(CC) $(FLAGS) -o $(OUT) $(OUT).c

clean:
	rm -f $(OUT) *.o

