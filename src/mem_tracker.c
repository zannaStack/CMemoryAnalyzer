#include "mem_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MemoryBlockHeader {
    uint32_t head_canary;
    size_t payload_size;
    const char *file;
    int line;
    struct MemoryBlockHeader *next;
    struct MemoryBlockHeader *prev;
} MemoryBlockHeader;

static MemoryBlockHeader *g_head = NULL;
static MemoryTrackerStats g_stats = {0, 0, 0, 0, 0, 0};

void mem_track_init(void) {
    g_head = NULL;
    memset(&g_stats, 0, sizeof(g_stats));
}

static inline uint32_t* get_tail_canary_ptr(MemoryBlockHeader *header) {
    uint8_t *user_payload = (uint8_t *)(header + 1);
    return (uint32_t *)(user_payload + header->payload_size);
}

void* mem_track_malloc(size_t size, const char *file, int line) {
    if (size == 0) return NULL;

    // Dimensione totale: Header + Payload Utente + Tail Canary
    size_t total_size = sizeof(MemoryBlockHeader) + size + sizeof(uint32_t);
    uint8_t *raw = (uint8_t *)malloc(total_size);
    if (!raw) {
        fprintf(stderr, "[MEM_TRACKER CRITICAL] malloc fallita per %zu bytes (%s:%d)\n", size, file, line);
        return NULL;
    }

    MemoryBlockHeader *header = (MemoryBlockHeader *)raw;
    header->head_canary = MEM_CANARY_HEAD;
    header->payload_size = size;
    header->file = file ? file : "unknown";
    header->line = line;
    header->prev = NULL;
    header->next = g_head;

    if (g_head) {
        g_head->prev = header;
    }
    g_head = header;

    // Scrivi tail canary
    uint32_t *tail = get_tail_canary_ptr(header);
    *tail = MEM_CANARY_TAIL;

    // Aggiorna metriche
    g_stats.total_allocations++;
    g_stats.active_allocations++;
    g_stats.active_bytes += size;
    if (g_stats.active_bytes > g_stats.peak_bytes) {
        g_stats.peak_bytes = g_stats.active_bytes;
    }

    return (void *)(header + 1);
}

void* mem_track_calloc(size_t num, size_t size, const char *file, int line) {
    size_t total = num * size;
    void *ptr = mem_track_malloc(total, file, line);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* mem_track_realloc(void *ptr, size_t new_size, const char *file, int line) {
    if (!ptr) {
        return mem_track_malloc(new_size, file, line);
    }
    if (new_size == 0) {
        mem_track_free(ptr, file, line);
        return NULL;
    }

    MemoryBlockHeader *header = ((MemoryBlockHeader *)ptr) - 1;
    if (header->head_canary != MEM_CANARY_HEAD) {
        fprintf(stderr, "[MEM_TRACKER CORRUPTION] Realloc su puntatore non valido o head canary corrotta! (%s:%d)\n", file, line);
        return NULL;
    }

    void *new_ptr = mem_track_malloc(new_size, file, line);
    if (!new_ptr) return NULL;

    size_t copy_size = (new_size < header->payload_size) ? new_size : header->payload_size;
    memcpy(new_ptr, ptr, copy_size);
    mem_track_free(ptr, file, line);

    return new_ptr;
}

void mem_track_free(void *ptr, const char *file, int line) {
    if (!ptr) return;

    MemoryBlockHeader *header = ((MemoryBlockHeader *)ptr) - 1;

    // Controllo integrità prima di liberare
    if (header->head_canary != MEM_CANARY_HEAD) {
        fprintf(stderr, "\n[MEM_TRACKER ALERT] BUFFER UNDERFLOW o DOUBLE FREE rilevato! Puntatore: %p (%s:%d)\n", ptr, file, line);
        g_stats.corrupted_blocks_count++;
        return;
    }

    uint32_t *tail = get_tail_canary_ptr(header);
    if (*tail != MEM_CANARY_TAIL) {
        fprintf(stderr, "\n[MEM_TRACKER ALERT] BUFFER OVERFLOW rilevato! Blocco %zu bytes (%s:%d), allocato a (%s:%d)\n",
                header->payload_size, file, line, header->file, header->line);
        g_stats.corrupted_blocks_count++;
    }

    // Rimuovi dalla lista doppiamente linkata
    if (header->prev) {
        header->prev->next = header->next;
    } else {
        g_head = header->next;
    }

    if (header->next) {
        header->next->prev = header->prev;
    }

    g_stats.active_allocations--;
    g_stats.active_bytes -= header->payload_size;
    g_stats.total_freed_bytes += header->payload_size;

    // Distruggi i canary per catturare double-free futuri
    header->head_canary = 0xDEAD0000;
    *tail = 0x0000DEAD;

    free(header);
}

void mem_track_get_stats(MemoryTrackerStats *stats) {
    if (stats) {
        *stats = g_stats;
    }
}

int mem_track_check_corruption(void) {
    int corrupted = 0;
    MemoryBlockHeader *curr = g_head;

    while (curr) {
        if (curr->head_canary != MEM_CANARY_HEAD) {
            fprintf(stderr, "[CORRUPTION DETECTED] Head canary alterata nel blocco allocato a %s:%d\n", curr->file, curr->line);
            corrupted++;
        }
        uint32_t *tail = get_tail_canary_ptr(curr);
        if (*tail != MEM_CANARY_TAIL) {
            fprintf(stderr, "[CORRUPTION DETECTED] Tail canary alterata (Buffer Overflow) nel blocco (%zu bytes) a %s:%d\n",
                    curr->payload_size, curr->file, curr->line);
            corrupted++;
        }
        curr = curr->next;
    }

    return corrupted;
}

size_t mem_track_report_leaks(void) {
    printf("\n============================================================\n");
    printf("         REPORT MEMORY LEAKS (HEAP PROFILER AUDIT)          \n");
    printf("============================================================\n");

    if (!g_head) {
        printf("✔ Nessun Memory Leak rilevato! Tutte le allocazioni sono state deallocate correttamente.\n");
        printf("Allocazioni totali: %zu | Picco memoria: %zu bytes\n", g_stats.total_allocations, g_stats.peak_bytes);
        printf("============================================================\n\n");
        return 0;
    }

    printf("⚠ ATTENZIONE: Rilevati %zu blocchi di memoria NON liberati (%zu bytes totali persi):\n\n",
           g_stats.active_allocations, g_stats.active_bytes);

    MemoryBlockHeader *curr = g_head;
    size_t count = 0;
    while (curr) {
        count++;
        printf("  [LEAK #%02zu] Indirizzo: %p | Dimensione: %6zu bytes | Origine: %s:%d\n",
               count, (void *)(curr + 1), curr->payload_size, curr->file, curr->line);
        curr = curr->next;
    }

    printf("------------------------------------------------------------\n");
    printf("Totale Bytes Persi : %zu bytes\n", g_stats.active_bytes);
    printf("Picco Memoria Raggiunto : %zu bytes\n", g_stats.peak_bytes);
    printf("============================================================\n\n");
    return count;
}

void mem_track_cleanup(void) {
    MemoryBlockHeader *curr = g_head;
    while (curr) {
        MemoryBlockHeader *next = curr->next;
        free(curr);
        curr = next;
    }
    g_head = NULL;
    g_stats.active_allocations = 0;
    g_stats.active_bytes = 0;
}
