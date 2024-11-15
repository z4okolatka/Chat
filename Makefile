SRC=main.c elements.c utils.c chatclient.c messages.c debug.c
EXE=main

.PHONY: build clean server

build: $(SRC) 
	gcc $(SRC) -lncurses -pthread -o $(EXE)

run: build
	./main

server:
	gcc server.c -g -o server
	./server

clean:
	rm $(EXE)