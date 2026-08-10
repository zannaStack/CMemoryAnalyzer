#include <stdio.h>
#include <string.h>
#include "leak_detector.h"

/* Struttura dati tipica per liste concatenate */
typedef struct Nodo {
    int valore;
    struct Nodo *prossimo;
} Nodo;

/* Test 1: Allocazione e deallocazione corretta di un buffer */
void test_memoria_corretta(void) {
    printf("[Test 1] Allocazione di un vettore dinamico e rilascio corretto...\n");
    
    int *numeri = (int *)malloc(10 * sizeof(int));
    if (numeri) {
        for (int i = 0; i < 10; i++) {
            numeri[i] = (i + 1) * 10;
        }
        printf("         Array popolato con successo.\n");
    }

    /* Deallocazione corretta */
    free(numeri);
    printf("         Memoria liberata correttamente tramite free().\n\n");
}

/* Test 2: Simulazione di una lista concatenata con un nodo dimenticato (memory leak) */
void test_simulazione_leak(void) {
    printf("[Test 2] Creazione di una lista dinamica e simulazione di un leak...\n");

    Nodo *testa = (Nodo *)malloc(sizeof(Nodo));
    Nodo *secondo = (Nodo *)malloc(sizeof(Nodo));
    char *nome_buffer = (char *)malloc(64 * sizeof(char));

    if (testa && secondo && nome_buffer) {
        testa->valore = 100;
        testa->prossimo = secondo;
        secondo->valore = 200;
        secondo->prossimo = NULL;
        strcpy(nome_buffer, "Buffer di test per leak detection");
    }

    /* Liberiamo solo il primo nodo e il buffer, "dimenticando" il secondo nodo */
    free(testa);
    free(nome_buffer);
    /* NOTA: 'secondo' NON viene deallocato intenzionalmente per testare il detector */

    printf("         Liberati solo 2 elementi su 3 (1 nodo lasciato intenzionalmente orfano).\n\n");
}

int main(void) {
    printf("==================================================\n");
    printf("       DEMO: C Memory Leak Detector\n");
    printf("==================================================\n\n");

    test_memoria_corretta();
    test_simulazione_leak();

    /* Genera il report finale dei leak */
    leak_report();

    /* Pulizia finale per concludere il programma in modo pulito */
    leak_reset();

    return 0;
}
