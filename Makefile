CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Iinclude
LDFLAGS = -lpsapi

SRC = src/sqlite3.c src/sys_mem.c src/mem_tracker.c src/db_storage.c src/main.c
OUT = bin/memory_analyzer.exe

all: $(OUT)

$(OUT): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

test: $(OUT)
	./$(OUT) --test

clean:
	rm -rf bin memory_analyzer.db
