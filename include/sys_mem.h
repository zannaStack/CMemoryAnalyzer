#ifndef SYS_MEM_H
#define SYS_MEM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t total_ram_bytes;
    uint64_t avail_ram_bytes;
    uint64_t total_pagefile_bytes;
    uint64_t avail_pagefile_bytes;
    uint64_t total_virtual_bytes;
    uint64_t avail_virtual_bytes;
    uint32_t memory_load_percent;
} SystemMemoryInfo;

typedef struct {
    uint32_t pid;
    char process_name[256];
    uint64_t working_set_bytes;
    uint64_t peak_working_set_bytes;
    uint64_t private_bytes;
    uint32_t page_fault_count;
} ProcessMemoryInfo;

/**
 * Rileva le statistiche correnti della memoria fisica e virtuale del sistema operativo.
 * @return 0 in caso di successo, -1 in caso di errore
 */
int sys_mem_get_system_info(SystemMemoryInfo *info);

/**
 * Rileva il consumo di memoria dettagliato per un dato Process ID (PID).
 * @return 0 in caso di successo, -1 in caso di errore
 */
int sys_mem_get_process_info(uint32_t pid, ProcessMemoryInfo *info);

/**
 * Stampa formattata delle informazioni di sistema a console.
 */
void sys_mem_print_system_info(const SystemMemoryInfo *info);

/**
 * Stampa formattata delle statistiche di processo a console.
 */
void sys_mem_print_process_info(const ProcessMemoryInfo *info);

#ifdef __cplusplus
}
#endif

#endif /* SYS_MEM_H */
