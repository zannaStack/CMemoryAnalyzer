@echo off
setlocal
chcp 65001 >nul

echo ========================================================
echo        COMPILAZIONE C MEMORY ANALYZER E PROFILER
echo ========================================================

if not exist "bin" mkdir bin

if not exist "bin\sqlite3.o" (
    echo [1/2] Compilazione statica motore SQLite...
    gcc -O2 -w -Iinclude -c src/sqlite3.c -o bin/sqlite3.o
)

echo [1/2] Compilazione moduli C11 e linking...
gcc -std=c11 -O2 -Wall -Wextra -Iinclude src/sys_mem.c src/mem_tracker.c src/db_storage.c src/main.c bin/sqlite3.o -o bin/memory_analyzer.exe -lpsapi

if errorlevel 1 (
    echo [ERRORE] Compilazione fallita!
    exit /b 1
)

echo [2/2] Avvio programma...
echo.
bin\memory_analyzer.exe %*
