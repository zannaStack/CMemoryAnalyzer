@echo off
setlocal
chcp 65001 >nul

echo ========================================================
echo        COMPILAZIONE C MEMORY ANALYZER E PROFILER
echo ========================================================

if not exist "bin" mkdir bin

REM Compila il motore SQLite solo se l'oggetto bin\sqlite3.o non esiste ancora
if not exist "bin\sqlite3.o" (
    echo [1/3] Compilazione statica motore SQLite C API (una tantum)...
    gcc -O2 -w -Iinclude -c src/sqlite3.c -o bin/sqlite3.o
    if %errorlevel% neq 0 (
        echo [ERRORE] Compilazione SQLite fallita!
        exit /b %errorlevel%
    )
)

echo [2/3] Compilazione moduli C11 e linking...
gcc -std=c11 -O2 -Wall -Wextra -Iinclude src/sys_mem.c src/mem_tracker.c src/db_storage.c src/main.c bin/sqlite3.o -o bin/memory_analyzer.exe -lpsapi

if %errorlevel% neq 0 (
    echo [ERRORE] Compilazione fallita!
    exit /b %errorlevel%
)

echo [3/3] Avvio programma...
echo.
if "%1"=="--test" (
    bin\memory_analyzer.exe --test
) else if not "%1"=="" (
    bin\memory_analyzer.exe %*
) else (
    bin\memory_analyzer.exe
)
