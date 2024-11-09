SRC=main.c elements.c utils.c chatclient.c messages.c
EXE=main

.PHONY: build clean server

build: $(SRC) 
	gcc $(SRC) -lncurses -pthread -o $(EXE)

run: build
	./main

server:
	gcc server.c -o server
	./server

clean:
	rm $(SRC) $(EXE)