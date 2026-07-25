#ifndef LEAK_DETECTOR_H
#define LEAK_DETECTOR_H

#include <stddef.h>

/*
 * Funzioni di tracciamento della memoria dinamica.
 * Vengono invocate tramite le macro sottostanti per catturare
 * automaticamente il file e la riga della chiamata.
 */
void* leak_malloc(size_t size, const char *file, int line);
void* leak_calloc(size_t num, size_t size, const char *file, int line);
void* leak_realloc(void *ptr, size_t new_size, const char *file, int line);
void  leak_free(void *ptr, const char *file, int line);

/* Stampa il report finale dei leak a terminale */
void  leak_report(void);

/* Pulisce i metadati interni prima della chiusura */
void  leak_reset(void);

/*
 * Ridefinizione trasparente di malloc, calloc, realloc e free.
 * Includendo questo header in qualsiasi file .c, le chiamate standard
 * vengono automaticamente tracciate senza dover modificare la logica del codice.
 */
#ifndef LEAK_DETECTOR_IMPL
    #define malloc(size)        leak_malloc(size, __FILE__, __LINE__)
    #define calloc(num, size)   leak_calloc(num, size, __FILE__, __LINE__)
    #define realloc(ptr, size)  leak_realloc(ptr, size, __FILE__, __LINE__)
    #define free(ptr)           leak_free(ptr, __FILE__, __LINE__)
#endif

#endif /* LEAK_DETECTOR_H */
