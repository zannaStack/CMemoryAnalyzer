#ifndef LEAK_DETECTOR_H
#define LEAK_DETECTOR_H

#include <stddef.h>

/*
 * Funzioni di tracciamento base della memoria dinamica.
 * Catturano il file e la riga della chiamata.
 */
void* leak_malloc(size_t size, const char *file, int line);
void  leak_free(void *ptr, const char *file, int line);

/* Stampa il report finale dei leak a terminale */
void  leak_report(void);

#ifndef LEAK_DETECTOR_IMPL
    #define malloc(size) leak_malloc(size, __FILE__, __LINE__)
    #define free(ptr)    leak_free(ptr, __FILE__, __LINE__)
#endif

#endif /* LEAK_DETECTOR_H */
