CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -O2
SRC = src/main.c src/leak_detector.c
OUT = bin/leak_detector.exe

all: $(OUT)

$(OUT): $(SRC)
	@if not exist bin mkdir bin
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	@if exist bin rmdir /s /q bin

run: $(OUT)
	@$(OUT)

.PHONY: all clean run
