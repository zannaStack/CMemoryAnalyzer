@echo off
setlocal
chcp 65001 >nul

echo ========================================================
echo        COMPILAZIONE C MEMORY ANALYZER E PROFILER
echo ========================================================

if not exist "bin" mkdir bin

echo [1/2] Compilazione con GCC (C11, Ottimizzazione O2, Win32 PSAPI, SQLite)...
gcc -std=c11 -O2 -Wall -Wextra -Iinclude src/sqlite3.c src/sys_mem.c src/mem_tracker.c src/db_storage.c src/main.c -o bin/memory_analyzer.exe -lpsapi

if %errorlevel% neq 0 (
    echo [ERRORE] Compilazione fallita!
    exit /b %errorlevel%
)

echo [2/2] Compilazione completata con successo: bin\memory_analyzer.exe
echo.
if "%1"=="--test" (
    bin\memory_analyzer.exe --test
) else if not "%1"=="" (
    bin\memory_analyzer.exe %*
) else (
    bin\memory_analyzer.exe
)
