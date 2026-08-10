@echo off
if not exist bin mkdir bin
gcc -Wall -Wextra -std=c99 -Iinclude src/main.c src/leak_detector.c -o bin/leak_detector.exe
if %ERRORLEVEL% equ 0 (
    echo [OK] Compilazione completata con successo!
    echo.
    bin\leak_detector.exe
) else (
    echo [ERRORE] Compilazione fallita.
)
