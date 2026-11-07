#ifndef MEM_TRACKER_H
#define MEM_TRACKER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_CANARY_HEAD 0xDEADBEEFU
#define MEM_CANARY_TAIL 0xC0DEDEADU

typedef struct {
    size_t active_bytes;
    size_t peak_bytes;
    size_t total_allocations;
    size_t active_allocations;
    size_t total_freed_bytes;
    size_t corrupted_blocks_count;
} MemoryTrackerStats;

/**
 * Inizializza il modulo di tracciamento della memoria heap.
 */
void mem_track_init(void);

/**
 * Wrapper strumentati per allocazione e rilascio di memoria heap.
 */
void* mem_track_malloc(size_t size, const char *file, int line);
void* mem_track_calloc(size_t num, size_t size, const char *file, int line);
void* mem_track_realloc(void *ptr, size_t new_size, const char *file, int line);
void  mem_track_free(void *ptr, const char *file, int line);

/**
 * Recupera le metriche globali dell'heap tracker.
 */
void mem_track_get_stats(MemoryTrackerStats *stats);

/**
 * Verifica l'integrità dei canary bytes su tutti i blocchi attualmente allocati.
 * Rileva buffer underflow o buffer overflow.
 * @return Numero di blocchi con memoria corrotta (0 = nessun errore)
 */
int mem_track_check_corruption(void);

/**
 * Ispeziona l'heap e stampa un report dettagliato di tutti i Memory Leak non deallocati.
 * @return Numero di blocchi di memoria persi (memory leaks)
 */
size_t mem_track_report_leaks(void);

/**
 * Resetta e libera le strutture interne dell'allocatore.
 */
void mem_track_cleanup(void);

/* Macro per l'uso immediato e pulito nel codice (prefisso TRACK_ per evitare collisioni con Win32 MEM_FREE) */
#define TRACK_MALLOC(sz)        mem_track_malloc((sz), __FILE__, __LINE__)
#define TRACK_CALLOC(num, sz)   mem_track_calloc((num), (sz), __FILE__, __LINE__)
#define TRACK_REALLOC(ptr, sz)  mem_track_realloc((ptr), (sz), __FILE__, __LINE__)
#define TRACK_FREE(ptr)         mem_track_free((ptr), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* MEM_TRACKER_H */
