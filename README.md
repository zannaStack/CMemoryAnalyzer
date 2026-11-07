# 🔍 C Memory Leak Detector

> Un tracciatore leggero delle allocazioni dinamiche e rilevatore di memory leak scritto in **C standard (C99)**.

---

## 💡 Perché ho creato questo progetto?

Durante lo studio per l'esame di **Informatica 1** (superato con 30 e Lode presso l'Università di Bologna), uno degli scogli principali è sempre stata la corretta gestione dello heap: quando si implementano liste concatenate, code, pile o alberi binari, dimenticare anche una sola chiamata a `free()` porta a un **memory leak**.

Su Linux si usa spesso **Valgrind**, ma lavorando in ambiente Windows con MinGW/GCC non è disponibile nativamente senza WSL. 

Ho quindi sviluppato questo piccolo tool autonomo:
- **Zero dipendenze esterne**: solo C standard puro, compilabile ovunque (Windows, Linux, macOS).
- **Trasparente**: basta includere l'header e compilare il file `.c` insieme al proprio progetto. Non serve modificare le chiamate esistenti nel codice (`malloc` e `free` continuano a funzionare come al solito).
- **Report chiaro e utile**: alla fine dell'esecuzione mostra subito **in quale file e a quale riga** è avvenuta l'allocazione dimenticata, insieme al numero esatto di byte persi.

---

## ⚙️ Come Funziona

Il funzionamento si basa su due elementi chiave del linguaggio C:

1. **Macro del Preprocessore (`__FILE__` e `__LINE__`)**:
   Nell'header `leak_detector.h`, le funzioni `malloc()`, `calloc()`, `realloc()` e `free()` vengono ridefinite:
   ```c
   #define malloc(size)  leak_malloc(size, __FILE__, __LINE__)
   #define free(ptr)     leak_free(ptr, __FILE__, __LINE__)
   ```
   In questo modo il compilatore inietta le coordinate esatte del file sorgente a tempo di compilazione.

2. **Lista Concatenata dei Blocchi Attivi**:
   Ogni volta che viene allocata memoria, il tracker salva l'indirizzo restituito, la dimensione e la posizione sorgente all'interno di una lista concatenata interna.
   Quando viene chiamata `free()`, il nodo corrispondente viene rimosso. Se al termine del programma la lista contiene ancora elementi, significa che ci sono deallocazioni mancanti.

---

## 📋 Esempio di Output a Terminale

Eseguendo il file dimostrativo (`main.c`), viene simulata sia un'allocazione corretta sia una lista in cui un nodo viene lasciato orfano:

```text
==================================================
       DEMO: C Memory Leak Detector
==================================================

[Test 1] Allocazione di un vettore dinamico e rilascio corretto...
         Array popolato con successo.
         Memoria liberata correttamente tramite free().

[Test 2] Creazione di una lista dinamica e simulazione di un leak...
         Liberati solo 2 elementi su 3 (1 nodo lasciato intenzionalmente orfano).

==================================================
             REPORT STATISTICHE MEMORIA           
==================================================
 Allocazioni effettuate : 5 (184 byte totali)
 Deallocazioni eseguite : 4 (168 byte liberati)
 Picco di memoria usato : 184 byte
--------------------------------------------------
 [!] ATTENZIONE: Rilevati blocchi non liberati:

  #1 -> 16 byte dimenticati
         Allocato in : src/main.c, riga 32
         Indirizzo   : 000002166e5714e0

 Totale memoria persa : 16 byte in 1 blocchi.
==================================================
```

---

## 🚀 Come Compilare ed Eseguire

### Con GCC su Windows o Linux:
```bash
gcc -Wall -Wextra -std=c99 -Iinclude src/main.c src/leak_detector.c -o leak_detector
./leak_detector
```

### Tramite script batch (Windows):
```cmd
build.bat
```

### Tramite Makefile:
```bash
make run
```

---

## 🛠️ Come integrarlo in un altro progetto C

1. Copia `include/leak_detector.h` e `src/leak_detector.c` nel tuo progetto.
2. Includi `#include "leak_detector.h"` nei tuoi file sorgente.
3. Chiama `leak_report()` prima del `return 0;` nel tuo `main()`.
4. Compila includendo `leak_detector.c`.

---

## 🎓 Competenze Consolidate
- Allocazione dinamica dello heap (`malloc`, `calloc`, `realloc`, `free`).
- Gestione di puntatori opachi (`void*`) e strutture dati a lista concatenata.
- Utilizzo avanzato delle macro del preprocessore C (`__FILE__`, `__LINE__`).
- Diagnostica e tracciamento a basso livello senza strumenti esterni pesanti.
