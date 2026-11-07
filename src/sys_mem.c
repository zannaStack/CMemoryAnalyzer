#include "sys_mem.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <psapi.h>
#else
    #include <sys/types.h>
    #include <unistd.h>
#endif

int sys_mem_get_system_info(SystemMemoryInfo *info) {
    if (!info) return -1;
    memset(info, 0, sizeof(SystemMemoryInfo));

#if defined(_WIN32) || defined(_WIN64)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);

    if (!GlobalMemoryStatusEx(&statex)) {
        return -1;
    }

    info->total_ram_bytes = statex.ullTotalPhys;
    info->avail_ram_bytes = statex.ullAvailPhys;
    info->total_pagefile_bytes = statex.ullTotalPageFile;
    info->avail_pagefile_bytes = statex.ullAvailPageFile;
    info->total_virtual_bytes = statex.ullTotalVirtual;
    info->avail_virtual_bytes = statex.ullAvailVirtual;
    info->memory_load_percent = statex.dwMemoryLoad;
    return 0;
#else
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;

    char line[256];
    uint64_t mem_total_kb = 0, mem_free_kb = 0, mem_avail_kb = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total_kb) == 1) continue;
        if (sscanf(line, "MemFree: %lu kB", &mem_free_kb) == 1) continue;
        if (sscanf(line, "MemAvailable: %lu kB", &mem_avail_kb) == 1) continue;
    }
    fclose(f);

    info->total_ram_bytes = mem_total_kb * 1024;
    info->avail_ram_bytes = (mem_avail_kb ? mem_avail_kb : mem_free_kb) * 1024;
    if (info->total_ram_bytes > 0) {
        uint64_t used = info->total_ram_bytes - info->avail_ram_bytes;
        info->memory_load_percent = (uint32_t)((used * 100) / info->total_ram_bytes);
    }
    return 0;
#endif
}

int sys_mem_get_process_info(uint32_t pid, ProcessMemoryInfo *info) {
    if (!info) return -1;
    memset(info, 0, sizeof(ProcessMemoryInfo));
    info->pid = pid;
    snprintf(info->process_name, sizeof(info->process_name), "PID-%u", pid);

#if defined(_WIN32) || defined(_WIN64)
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        return -1;
    }

    // Recupera il nome dell'eseguibile del processo
    char processName[MAX_PATH] = "<sconosciuto>";
    if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
        snprintf(info->process_name, sizeof(info->process_name), "%s", processName);
    }

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc))) {
        info->working_set_bytes = pmc.WorkingSetSize;
        info->peak_working_set_bytes = pmc.PeakWorkingSetSize;
        info->private_bytes = pmc.PrivateUsage;
        info->page_fault_count = pmc.PageFaultCount;
        CloseHandle(hProcess);
        return 0;
    }

    CloseHandle(hProcess);
    return -1;
#else
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    uint64_t vm_rss_kb = 0, vm_peak_kb = 0, vm_data_kb = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name: %255s", info->process_name);
        } else if (sscanf(line, "VmRSS: %lu kB", &vm_rss_kb) == 1) {
            info->working_set_bytes = vm_rss_kb * 1024;
        } else if (sscanf(line, "VmHWM: %lu kB", &vm_peak_kb) == 1) {
            info->peak_working_set_bytes = vm_peak_kb * 1024;
        } else if (sscanf(line, "VmData: %lu kB", &vm_data_kb) == 1) {
            info->private_bytes = vm_data_kb * 1024;
        }
    }
    fclose(f);
    return 0;
#endif
}

void sys_mem_print_system_info(const SystemMemoryInfo *info) {
    if (!info) return;

    double total_ram_mb = (double)info->total_ram_bytes / (1024.0 * 1024.0);
    double avail_ram_mb = (double)info->avail_ram_bytes / (1024.0 * 1024.0);
    double used_ram_mb = total_ram_mb - avail_ram_mb;

    double total_page_mb = (double)info->total_pagefile_bytes / (1024.0 * 1024.0);
    double avail_page_mb = (double)info->avail_pagefile_bytes / (1024.0 * 1024.0);

    printf("============================================================\n");
    printf("         ANALISI MEMORIA DI SISTEMA (OS HARDWARE LEVEL)     \n");
    printf("============================================================\n");
    printf("RAM Fisica Totale      : %10.2f MB (%6.2f GB)\n", total_ram_mb, total_ram_mb / 1024.0);
    printf("RAM Fisica In Uso      : %10.2f MB (%6.2f GB)\n", used_ram_mb, used_ram_mb / 1024.0);
    printf("RAM Fisica Disponibile : %10.2f MB (%6.2f GB)\n", avail_ram_mb, avail_ram_mb / 1024.0);
    printf("Carico Memoria Stimato : %10u %%\n", info->memory_load_percent);
    printf("------------------------------------------------------------\n");
    printf("File di Paging Totale  : %10.2f MB (%6.2f GB)\n", total_page_mb, total_page_mb / 1024.0);
    printf("File di Paging Libero  : %10.2f MB (%6.2f GB)\n", avail_page_mb, avail_page_mb / 1024.0);
    printf("============================================================\n\n");
}

void sys_mem_print_process_info(const ProcessMemoryInfo *info) {
    if (!info) return;

    double ws_mb = (double)info->working_set_bytes / (1024.0 * 1024.0);
    double peak_ws_mb = (double)info->peak_working_set_bytes / (1024.0 * 1024.0);
    double priv_mb = (double)info->private_bytes / (1024.0 * 1024.0);

    printf("------------------------------------------------------------\n");
    printf("Processo               : %s (PID: %u)\n", info->process_name, info->pid);
    printf("Working Set (RAM reale): %10.2f MB (%10llu bytes)\n", ws_mb, (unsigned long long)info->working_set_bytes);
    printf("Peak Working Set (Max) : %10.2f MB (%10llu bytes)\n", peak_ws_mb, (unsigned long long)info->peak_working_set_bytes);
    printf("Private Bytes (Heap)   : %10.2f MB (%10llu bytes)\n", priv_mb, (unsigned long long)info->private_bytes);
    printf("Page Faults Generati   : %10u\n", info->page_fault_count);
    printf("------------------------------------------------------------\n");
}
