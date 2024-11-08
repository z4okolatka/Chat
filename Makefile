SRC=main.c
EXE=main

.PHONY: build clean

build: $(SRC) 
	gcc $(SRC) -lncurses -o $(EXE)

clean:
	rm $(SRC) $(EXE)