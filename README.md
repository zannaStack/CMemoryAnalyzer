# 🔬 C Memory Analyzer & Heap Profiler (C11 + Win32/POSIX + SQLite)

> Strumento di analisi della memoria di sistema a basso livello e profiler personalizzato per heap con rilevamento di memory leak e buffer overflow tramite canary bytes, con persistenza delle metriche su database relazionale SQLite.

---

## 🎯 Obiettivi e Competenze Ingegneristiche Dimostrate

Questo progetto è sviluppato per esplorare la gestione della memoria nei sistemi operativi moderni attraverso il linguaggio C standard (C11):
1. **Analisi Hardware & OS Level**: Interfacciamento diretto con le API di sistema (Win32 `GlobalMemoryStatusEx` e `GetProcessMemoryInfo` su Windows; `/proc/meminfo` su ambienti POSIX/Linux) per estrarre in tempo reale metriche su RAM fisica, Virtual Memory, Working Set e Page Faults.
2. **Custom Heap Allocator & Leak Detector ("Mini-Valgrind")**: Strumentazione delle chiamate `malloc`, `calloc`, `realloc` e `free` tramite header opachi e liste doppiamente concatenate.
3. **Integrazione Canary Guard Bytes**: Protezione contro **Buffer Overflow** e **Underflow** inserendo sequenze di byte magiche (`0xDEADBEEF` all'inizio e `0xC0DEDEAD` alla fine del payload) verificate a runtime e in fase di deallocazione.
4. **Database Relazionale SQLite C API**: Persistenza storica dei campionamenti e delle statistiche heap interrogabili via SQL standard.

---

## 🏗️ Architettura del Sistema

```
                  [C MEMORY ANALYZER]
                           │
       ┌───────────────────┼───────────────────┐
       ▼                                       ▼
[System & Process Inspector]         [Heap Memory Profiler]
 ├── RAM Totale / Disponibile         ├── Header con Metadati e Timestamp
 ├── Working Set per PID              ├── Doubly Linked List allocazioni attive
 ├── Private Bytes & Heap             ├── Canary Guard Bytes (0xDEADBEEF / 0xC0DEDEAD)
 └── Page Fault Tracking              └── Report Leak a fine esecuzione
       │                                       │
       └───────────────────┬───────────────────┘
                           ▼
              [SQLite C API Integration]
              ├── Tabella: system_snapshots
              ├── Tabella: process_metrics
              └── Tabella: heap_profiles
```

---

## 🗄️ Struttura delle Tabelle SQL (SQLite)

```sql
-- Snapshot globali della memoria fisica del PC
CREATE TABLE system_snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    total_ram_mb REAL NOT NULL,
    avail_ram_mb REAL NOT NULL,
    used_ram_mb REAL NOT NULL,
    mem_load_pct INTEGER NOT NULL,
    total_page_mb REAL NOT NULL,
    avail_page_mb REAL NOT NULL
);

-- Metriche granulari per singolo Process ID (PID)
CREATE TABLE process_metrics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    snapshot_id INTEGER,
    pid INTEGER NOT NULL,
    proc_name TEXT NOT NULL,
    working_set_mb REAL NOT NULL,
    peak_ws_mb REAL NOT NULL,
    private_bytes_mb REAL NOT NULL,
    page_faults INTEGER NOT NULL,
    FOREIGN KEY (snapshot_id) REFERENCES system_snapshots(id)
);

-- Storico dei report dell'Heap Profiler
CREATE TABLE heap_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    active_bytes INTEGER NOT NULL,
    peak_bytes INTEGER NOT NULL,
    total_allocations INTEGER NOT NULL,
    leaked_bytes INTEGER NOT NULL
);
```

---

## 🛡️ Rilevamento Buffer Overflow con Canary Bytes

Ogni blocco allocato tramite `TRACK_MALLOC(sz)` presenta questa struttura di memoria in memoria:

```
[ MemoryBlockHeader ] [ CANARY HEAD (0xDEADBEEF) ] [ USER PAYLOAD (N bytes) ] [ CANARY TAIL (0xC0DEDEAD) ]
```

Se il programma scrive anche un solo byte oltre la dimensione allocata (`buffer overflow`), il `CANARY TAIL` viene corrotto. La funzione `mem_track_check_corruption()` o `TRACK_FREE()` intercetta immediatamente l'anomalia prima che possa causare vulnerabilità o crash silenziosi.

---

## 🚀 Compilazione ed Esecuzione

### Prerequisiti
- Compilatore **GCC** (MSYS2, MinGW o Linux GCC) con supporto C11.
- Su Windows non sono necessarie librerie esterne: SQLite amalgamation (`sqlite3.c` e `sqlite3.h`) è integrato direttamente nel repository e linkato a tempo di compilazione.

### Compilazione e Avvio Rapido su Windows (Batch script)
```bash
build.bat
```

### Esecuzione della Suite di Test Integrata
```bash
build.bat --test
```

### Compilazione con Makefile
```bash
make
make test
```

### Compilazione Manuale con GCC
```bash
gcc -std=c11 -O2 -Wall -Wextra -Iinclude src/sqlite3.c src/sys_mem.c src/mem_tracker.c src/db_storage.c src/main.c -o bin/memory_analyzer.exe -lpsapi
```

---

## 🎮 Funzionalità da Menu Interattivo
1. **Analisi Istantanea Memoria di Sistema**: Visualizza RAM in uso/libera, Paging file e salva lo snapshot su SQLite.
2. **Ispezione Memoria di un Processo**: Inserisci un PID qualsiasi e ispeziona Working Set, Peak RAM e Page Faults.
3. **Monitoraggio Continuo**: Campionamento periodico ad intervalli di tempo definiti con logging continuo su database.
4. **Dimostrazione Heap Profiler**: Dimostra l'audit di allocazioni attive, picco di memoria e tracciamento dei memory leaks con riga e file di origine.
5. **Test Buffer Overflow**: Simula una corruzione volontaria e mostra l'intercettazione dei Canary Bytes.
6. **Storico Metriche da SQLite**: Visualizza la cronologia degli snapshot registrati nel DB.

---

## 👤 Autore
**Fabio Zanetti**  
Studente di Ingegneria Informatica @ Università di Bologna  
- Portfolio: [fabiozanetti.it](https://fabiozanetti-it.vercel.app)  
- GitHub: [@zannaStack](https://github.com/zannaStack)
