#define LEAK_DETECTOR_IMPL
#include "leak_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Nodo della lista dei blocchi allocati.
 */
typedef struct AllocNode {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    struct AllocNode *next;
} AllocNode;

static AllocNode *head = NULL;

static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t current_allocated = 0;
static int total_alloc_count = 0;
static int total_free_count = 0;

void* leak_malloc(size_t size, const char *file, int line) {
    if (size == 0) return NULL;

    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "[ERRORE] Impossibile allocare %zu byte in %s:%d\n", size, file, line);
        return NULL;
    }

    AllocNode *node = (AllocNode *)malloc(sizeof(AllocNode));
    if (node) {
        node->ptr = ptr;
        node->size = size;
        node->file = file;
        node->line = line;
        node->next = head;
        head = node;
    }

    total_allocated += size;
    current_allocated += size;
    total_alloc_count++;

    return ptr;
}

void leak_free(void *ptr, const char *file, int line) {
    if (!ptr) return;

    AllocNode *curr = head;
    AllocNode *prev = NULL;

    while (curr) {
        if (curr->ptr == ptr) {
            if (prev) {
                prev->next = curr->next;
            } else {
                head = curr->next;
            }

            total_freed += curr->size;
            current_allocated -= curr->size;
            total_free_count++;

            free(curr);
            free(ptr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    fprintf(stderr, "[ATTENZIONE] Free non valida di puntatore %p in %s:%d\n", ptr, file, line);
    free(ptr);
}
