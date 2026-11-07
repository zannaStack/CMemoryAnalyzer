#ifndef DB_STORAGE_H
#define DB_STORAGE_H

#include "sys_mem.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inizializza il database SQLite locale e crea le tabelle necessarie.
 * @param db_path Percorso file database (es. "memory_metrics.db")
 * @return 0 se successo, -1 in caso di errore
 */
int db_init(const char *db_path);

/**
 * Salva uno snapshot globale della memoria di sistema.
 * @param info Puntatore alle statistiche correnti
 * @param out_snapshot_id ID generato per lo snapshot
 * @return 0 se successo, -1 in caso di errore
 */
int db_save_system_snapshot(const SystemMemoryInfo *info, int64_t *out_snapshot_id);

/**
 * Salva le metriche di un processo collegandole a uno snapshot di sistema.
 */
int db_save_process_metric(int64_t snapshot_id, const ProcessMemoryInfo *info);

/**
 * Salva le metriche di un'analisi dell'heap (allocazioni, picco, leak).
 */
int db_save_heap_profile(size_t active_bytes, size_t peak_bytes, size_t total_allocs, size_t leaked_bytes);

/**
 * Visualizza e stampa a console gli ultimi N snapshot salvati nel database.
 */
int db_print_recent_snapshots(int limit);

/**
 * Chiude la connessione al database SQLite.
 */
void db_close(void);

#ifdef __cplusplus
}
#endif

#endif /* DB_STORAGE_H */
