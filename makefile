CC=gcc
OPS=-Wall -Wextra -o$@
OBJOPS=-c $(OPS) -Wno-unused-function -Wno-missing-braces
OBJ_COMP=$(CC) $(OBJOPS) $<

.PHONY: run

run: osh
	./osh

osh: build/osh.o build/parse.o build/alias.o build/command.o build/environment.o build/history.o build/path.o build/status.o build/string_t.o
	$(CC) $(OPS) build/osh.o build/parse.o build/alias.o build/command.o build/environment.o build/history.o build/path.o build/status.o build/string_t.o

build/osh.o: src/osh.c
	$(OBJ_COMP)

build/parse.o: src/misc/source/parse.c src/misc/include/parse.h
	$(OBJ_COMP)

build/alias.o: src/types/source/alias.c src/types/include/alias.h
	$(OBJ_COMP)

build/command.o: src/types/source/command.c src/types/include/command.h
	$(OBJ_COMP)
	
build/environment.o: src/types/source/environment.c src/types/include/environment.h
	$(OBJ_COMP)

build/history.o: src/types/source/history.c src/types/include/history.h
	$(OBJ_COMP)

build/path.o: src/types/source/path.c src/types/include/path.h
	$(OBJ_COMP)

build/status.o: src/types/source/status.c src/types/include/status.h
	$(OBJ_COMP)

build/string_t.o: src/types/source/string_t.c src/types/include/string_t.h src/types/include/vector_t.h
	$(OBJ_COMP)

clean:
	rm -rf build/*
	rm osh

search:
	grep '$(P)' src/osh.c src/*/*/*
