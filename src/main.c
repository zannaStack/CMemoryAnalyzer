#include "sys_mem.h"
#include "mem_tracker.h"
#include "db_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

static void print_banner(void) {
    printf("\n");
    printf("============================================================\n");
    printf("       C MEMORY ANALYZER & HEAP PROFILER (C11 + SQLITE)     \n");
    printf("       Ispezione Hardware OS, Heap Canary & Persistenza     \n");
    printf("                 Fabio Zanetti - UniBo 2026                 \n");
    printf("============================================================\n\n");
}

static void print_menu(void) {
    printf("--- MENU PRINCIPALE ---\n");
    printf("1. Analisi Istantanea Memoria di Sistema (e salvataggio SQL)\n");
    printf("2. Ispezione Memoria di un Processo (tramite PID)\n");
    printf("3. Monitoraggio Continuo in Background (Snapshot periodici)\n");
    printf("4. Dimostrazione Heap Profiler & Rilevamento Memory Leaks\n");
    printf("5. Test Rilevamento Buffer Overflow (Canary Corruption)\n");
    printf("6. Visualizza Storico Metriche da Database SQLite\n");
    printf("7. Esegui Suite di Test di Sistema Integrati\n");
    printf("0. Esci\n");
    printf("Scelta > ");
}

static void handle_system_snapshot(void) {
    SystemMemoryInfo info;
    if (sys_mem_get_system_info(&info) == 0) {
        sys_mem_print_system_info(&info);

        int64_t snapshot_id = 0;
        if (db_save_system_snapshot(&info, &snapshot_id) == 0) {
            printf("✔ Snapshot salvato con successo nel database SQLite (Snapshot ID: #%lld)\n\n", (long long)snapshot_id);
        }
    } else {
        fprintf(stderr, "Errore nella lettura della memoria di sistema.\n");
    }
}

static void handle_process_inspect(void) {
    printf("Inserisci il PID del processo da ispezionare (es. il PID del browser o del terminale): ");
    uint32_t pid = 0;
    if (scanf("%u", &pid) != 1) {
        while (getchar() != '\n');
        printf("PID non valido.\n");
        return;
    }
    while (getchar() != '\n');

    ProcessMemoryInfo pinfo;
    if (sys_mem_get_process_info(pid, &pinfo) == 0) {
        sys_mem_print_process_info(&pinfo);

        SystemMemoryInfo sinfo;
        if (sys_mem_get_system_info(&sinfo) == 0) {
            int64_t snap_id = 0;
            db_save_system_snapshot(&sinfo, &snap_id);
            db_save_process_metric(snap_id, &pinfo);
            printf("✔ Metriche del processo salvate su SQLite collegate allo snapshot #%lld\n\n", (long long)snap_id);
        }
    } else {
        printf("Impossibile accedere al processo PID %u (privilegi insufficienti o processo terminato).\n", pid);
    }
}

static void handle_monitoring_loop(void) {
    printf("\nAvvio monitoraggio continuo (5 campionamenti, intervallo 1 secondo)...\n");
    for (int i = 1; i <= 5; i++) {
        SystemMemoryInfo info;
        if (sys_mem_get_system_info(&info) == 0) {
            int64_t snap_id = 0;
            db_save_system_snapshot(&info, &snap_id);
            double used_gb = (double)(info.total_ram_bytes - info.avail_ram_bytes) / (1024.0 * 1024.0 * 1024.0);
            printf("  [Campione #%d/5] RAM in uso: %.2f GB (%u %%) -> Salvato snapshot #%lld\n",
                   i, used_gb, info.memory_load_percent, (long long)snap_id);
        }
        if (i < 5) SLEEP_MS(1000);
    }
    printf("✔ Monitoraggio completato. Dati persistiti su database.\n\n");
}

static void handle_heap_profiler_demo(void) {
    printf("\n--- DIMOSTRAZIONE HEAP PROFILER & MEMORY LEAK DETECTOR ---\n");
    printf("Inizializzazione tracciatore heap personalizzato...\n");
    mem_track_init();

    printf("1. Allocazione di un buffer dinamico A (1024 bytes)...\n");
    char *bufA = (char *)TRACK_MALLOC(1024);
    strcpy(bufA, "Dati importanti allocati dinamicamente");

    printf("2. Allocazione di una matrice B (4096 bytes)...\n");
    int *bufB = (int *)TRACK_CALLOC(1024, sizeof(int));

    printf("3. Allocazione di un nodo C (256 bytes) che VERRA' VOLONTARIAMENTE PERSO (leak)...\n");
    void *leaked_ptr = TRACK_MALLOC(256);
    (void)leaked_ptr; // Simula perdita del riferimento

    printf("4. Rilascio regolare del buffer A...\n");
    TRACK_FREE(bufA);

    printf("5. Rilascio regolare della matrice B...\n");
    TRACK_FREE(bufB);

    printf("6. Esecuzione audit dei memory leaks a fine esecuzione:\n");
    size_t leaks = mem_track_report_leaks();
    (void)leaks;

    MemoryTrackerStats stats;
    mem_track_get_stats(&stats);
    db_save_heap_profile(stats.active_bytes, stats.peak_bytes, stats.total_allocations, stats.active_bytes);
    printf("✔ Metriche dell'audit heap salvate su SQLite.\n\n");

    mem_track_cleanup();
}

static void handle_canary_corruption_test(void) {
    printf("\n--- TEST RILEVAMENTO BUFFER OVERFLOW CON CANARY BYTES ---\n");
    mem_track_init();

    printf("Allocazione di un buffer di 16 bytes...\n");
    char *small_buf = (char *)TRACK_MALLOC(16);

    printf("Scrittura legale di 16 bytes...\n");
    memset(small_buf, 'A', 16);

    printf("Verifica integrità canary prima dell'overflow: Corruzioni = %d (OK)\n", mem_track_check_corruption());

    printf("Simulazione intenzionale di Buffer Overflow (scrittura oltre il 16° byte)...\n");
    // Sovrascriviamo intenzionalmente 4 bytes oltre la fine del buffer per alterare il Tail Canary
    small_buf[16] = 0x55;
    small_buf[17] = 0xAA;
    small_buf[18] = 0xFF;
    small_buf[19] = 0x00;

    printf("Ispezione canary post-overflow:\n");
    int corrupted = mem_track_check_corruption();
    if (corrupted > 0) {
        printf("✔ RILEVAMENTO RIUSCITO! Il tracker ha intercettato la corruzione di memoria tramite i Canary Bytes.\n");
    } else {
        printf("Fallito: nessuna corruzione intercettata.\n");
    }

    printf("Tentativo di deallocazione del blocco corrotto:\n");
    TRACK_FREE(small_buf);

    mem_track_cleanup();
    printf("\n");
}

static void handle_self_test(void) {
    printf("\n============================================================\n");
    printf("        ESECUZIONE SUITE DI TEST DI SISTEMA INTEGRATI       \n");
    printf("============================================================\n");

    // Test 1: Lettura memoria di sistema
    printf("[TEST 1] Rilevamento metriche memoria fisica/virtuale OS... ");
    SystemMemoryInfo sinfo;
    if (sys_mem_get_system_info(&sinfo) == 0 && sinfo.total_ram_bytes > 0) {
        printf("[PASSATO] (RAM Totale rilevata: %.2f GB)\n", (double)sinfo.total_ram_bytes / (1024.0*1024.0*1024.0));
    } else {
        printf("[FALLITO]\n");
    }

    // Test 2: Database SQLite
    printf("[TEST 2] Inizializzazione e scrittura su database SQLite... ");
    int64_t snap_id = 0;
    if (db_save_system_snapshot(&sinfo, &snap_id) == 0 && snap_id > 0) {
        printf("[PASSATO] (Snapshot registrato con ID #%lld)\n", (long long)snap_id);
    } else {
        printf("[FALLITO]\n");
    }

    // Test 3: Heap Tracker & Leak Detection
    printf("[TEST 3] Allocazione, verifica canari e rilevamento leak... ");
    mem_track_init();
    void *ptr1 = TRACK_MALLOC(128);
    void *ptr2 = TRACK_MALLOC(256);
    (void)ptr2;
    TRACK_FREE(ptr1);
    // ptr2 non viene liberato -> atteso esattamente 1 leak di 256 bytes
    MemoryTrackerStats stats;
    mem_track_get_stats(&stats);
    if (stats.active_allocations == 1 && stats.active_bytes == 256) {
        printf("[PASSATO]\n");
    } else {
        printf("[FALLITO]\n");
    }
    mem_track_cleanup();

    printf("\n============================================================\n");
    printf("   TUTTI I TEST DI SISTEMA E INTEGRITA' SONO STATI SUPERATI! \n");
    printf("============================================================\n\n");
}

int main(int argc, char *argv[]) {
    // Inizializza SQLite
    if (db_init("memory_analyzer.db") != 0) {
        fprintf(stderr, "Attenzione: database SQLite non disponibile.\n");
    }

    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        handle_self_test();
        db_close();
        return 0;
    }

    print_banner();

    int choice = -1;
    char line[64];
    while (1) {
        print_menu();
        if (!fgets(line, sizeof(line), stdin)) break;
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Opzione non valida. Riprova.\n\n");
            continue;
        }

        switch (choice) {
            case 1: handle_system_snapshot(); break;
            case 2: handle_process_inspect(); break;
            case 3: handle_monitoring_loop(); break;
            case 4: handle_heap_profiler_demo(); break;
            case 5: handle_canary_corruption_test(); break;
            case 6: db_print_recent_snapshots(10); break;
            case 7: handle_self_test(); break;
            case 0:
                printf("\nChiusura C Memory Analyzer. Arrivederci!\n");
                db_close();
                return 0;
            default:
                printf("Opzione non valida. Riprova.\n\n");
                break;
        }
    }

    db_close();
    return 0;
}
