CC=gcc
OPS=-Wall -Wextra -o$@
OBJOPS=-c $(OPS) -Wno-unused-function -Wno-missing-braces
OBJ_COMP=$(CC) $(OBJOPS) $<

.PHONY: run

run: osh
	./osh

osh: build/osh.o build/parse.o build/alias.o build/command.o build/environment.o build/history.o build/path.o build/status.o build/string_t.o
	$(CC) $(OPS) build/osh.o build/parse.o build/alias.o build/command.o build/environment.o build/history.o build/path.o build/status.o build/string_t.o

build/osh.o: osh.c
	$(OBJ_COMP)

build/parse.o: misc/source/parse.c misc/include/parse.h
	$(OBJ_COMP)

build/alias.o: types/source/alias.c types/include/alias.h
	$(OBJ_COMP)

build/command.o: types/source/command.c types/include/command.h
	$(OBJ_COMP)
	
build/environment.o: types/source/environment.c types/include/environment.h
	$(OBJ_COMP)

build/history.o: types/source/history.c types/include/history.h
	$(OBJ_COMP)

build/path.o: types/source/path.c types/include/path.h
	$(OBJ_COMP)

build/status.o: types/source/status.c types/include/status.h
	$(OBJ_COMP)

build/string_t.o: types/source/string_t.c types/include/string_t.h types/include/vector_t.h
	$(OBJ_COMP)

clean:
	rm -rf build/*
	rm osh
