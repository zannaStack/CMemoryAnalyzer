#define LEAK_DETECTOR_IMPL
#include "leak_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Struttura di un nodo della lista dei blocchi allocati.
 * Mantiene l'indirizzo, i byte riservati e la posizione (file e riga) nel codice sorgente.
 */
typedef struct AllocNode {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    struct AllocNode *next;
} AllocNode;

static AllocNode *head = NULL;

/* Metriche globali di esecuzione */
static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t current_allocated = 0;
static size_t peak_allocated = 0;
static int total_alloc_count = 0;
static int total_free_count = 0;

void* leak_malloc(size_t size, const char *file, int line) {
    if (size == 0) return NULL;

    /* Chiamata alla vera malloc di sistema */
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "[ERRORE] Impossibile allocare %zu byte in %s:%d\n", size, file, line);
        return NULL;
    }

    /* Registrazione del blocco nella nostra lista concatenata */
    AllocNode *node = (AllocNode *)malloc(sizeof(AllocNode));
    if (node) {
        node->ptr = ptr;
        node->size = size;
        node->file = file;
        node->line = line;
        node->next = head;
        head = node;
    }

    /* Aggiornamento contatori */
    total_allocated += size;
    current_allocated += size;
    if (current_allocated > peak_allocated) {
        peak_allocated = current_allocated;
    }
    total_alloc_count++;

    return ptr;
}

void* leak_calloc(size_t num, size_t size, const char *file, int line) {
    size_t total = num * size;
    void *ptr = leak_malloc(total, file, line);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* leak_realloc(void *ptr, size_t new_size, const char *file, int line) {
    if (!ptr) {
        return leak_malloc(new_size, file, line);
    }
    if (new_size == 0) {
        leak_free(ptr, file, line);
        return NULL;
    }

    /* Cerchiamo la vecchia dimensione per copiare il contenuto corretto */
    AllocNode *curr = head;
    size_t old_size = 0;
    while (curr) {
        if (curr->ptr == ptr) {
            old_size = curr->size;
            break;
        }
        curr = curr->next;
    }

    void *new_ptr = leak_malloc(new_size, file, line);
    if (!new_ptr) return NULL;

    size_t copy_size = (old_size > 0 && old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    leak_free(ptr, file, line);

    return new_ptr;
}

void leak_free(void *ptr, const char *file, int line) {
    if (!ptr) return;

    AllocNode *curr = head;
    AllocNode *prev = NULL;

    while (curr) {
        if (curr->ptr == ptr) {
            /* Rimuove il blocco dalla lista */
            if (prev) {
                prev->next = curr->next;
            } else {
                head = curr->next;
            }

            total_freed += curr->size;
            if (current_allocated >= curr->size) {
                current_allocated -= curr->size;
            }
            total_free_count++;

            /* Libera il metadato e il puntatore reale */
            free(curr);
            free(ptr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    /* Puntatore non trovato: possibile double-free o puntatore non tracciato */
    fprintf(stderr, "[ATTENZIONE] free() su puntatore non valido o gia' liberato (%p) in %s:%d\n", ptr, file, line);
}

void leak_report(void) {
    printf("\n");
    printf("==================================================\n");
    printf("             REPORT STATISTICHE MEMORIA           \n");
    printf("==================================================\n");
    printf(" Allocazioni effettuate : %d (%zu byte totali)\n", total_alloc_count, total_allocated);
    printf(" Deallocazioni eseguite : %d (%zu byte liberati)\n", total_free_count, total_freed);
    printf(" Picco di memoria usato : %zu byte\n", peak_allocated);
    printf("--------------------------------------------------\n");

    if (head == NULL) {
        printf(" [OK] Nessun memory leak! Tutta la memoria e' stata liberata.\n");
    } else {
        size_t leaked_bytes = 0;
        int leak_count = 0;
        AllocNode *curr = head;

        printf(" [!] ATTENZIONE: Rilevati blocchi non liberati:\n\n");
        while (curr) {
            leak_count++;
            leaked_bytes += curr->size;
            printf("  #%d -> %zu byte dimenticati\n", leak_count, curr->size);
            printf("         Allocato in : %s, riga %d\n", curr->file, curr->line);
            printf("         Indirizzo   : %p\n\n", curr->ptr);
            curr = curr->next;
        }

        printf(" Totale memoria persa : %zu byte in %d blocchi.\n", leaked_bytes, leak_count);
    }
    printf("==================================================\n\n");
}

void leak_reset(void) {
    AllocNode *curr = head;
    while (curr) {
        AllocNode *next = curr->next;
        free(curr->ptr);
        free(curr);
        curr = next;
    }
    head = NULL;
    total_allocated = 0;
    total_freed = 0;
    current_allocated = 0;
    peak_allocated = 0;
    total_alloc_count = 0;
    total_free_count = 0;
}
