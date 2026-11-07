#include "db_storage.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *g_db = NULL;

int db_init(const char *db_path) {
    if (g_db) {
        return 0; // Già inizializzato
    }

    int rc = sqlite3_open(db_path ? db_path : "memory_metrics.db", &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[SQLITE ERROR] Impossibile aprire database: %s\n", sqlite3_errmsg(g_db));
        g_db = NULL;
        return -1;
    }

    const char *ddl =
        "CREATE TABLE IF NOT EXISTS system_snapshots ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  total_ram_mb REAL NOT NULL,"
        "  avail_ram_mb REAL NOT NULL,"
        "  used_ram_mb REAL NOT NULL,"
        "  mem_load_pct INTEGER NOT NULL,"
        "  total_page_mb REAL NOT NULL,"
        "  avail_page_mb REAL NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS process_metrics ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  snapshot_id INTEGER,"
        "  pid INTEGER NOT NULL,"
        "  proc_name TEXT NOT NULL,"
        "  working_set_mb REAL NOT NULL,"
        "  peak_ws_mb REAL NOT NULL,"
        "  private_bytes_mb REAL NOT NULL,"
        "  page_faults INTEGER NOT NULL,"
        "  FOREIGN KEY (snapshot_id) REFERENCES system_snapshots(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS heap_profiles ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  active_bytes INTEGER NOT NULL,"
        "  peak_bytes INTEGER NOT NULL,"
        "  total_allocations INTEGER NOT NULL,"
        "  leaked_bytes INTEGER NOT NULL"
        ");";

    char *err_msg = NULL;
    rc = sqlite3_exec(g_db, ddl, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[SQLITE DDL ERROR] %s\n", err_msg ? err_msg : "Errore DDL sconosciuto");
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}

int db_save_system_snapshot(const SystemMemoryInfo *info, int64_t *out_snapshot_id) {
    if (!g_db || !info) return -1;

    double total_ram_mb = (double)info->total_ram_bytes / (1024.0 * 1024.0);
    double avail_ram_mb = (double)info->avail_ram_bytes / (1024.0 * 1024.0);
    double used_ram_mb = total_ram_mb - avail_ram_mb;
    double total_page_mb = (double)info->total_pagefile_bytes / (1024.0 * 1024.0);
    double avail_page_mb = (double)info->avail_pagefile_bytes / (1024.0 * 1024.0);

    const char *sql =
        "INSERT INTO system_snapshots (total_ram_mb, avail_ram_mb, used_ram_mb, mem_load_pct, total_page_mb, avail_page_mb) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[SQLITE PREPARE ERROR] %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    sqlite3_bind_double(stmt, 1, total_ram_mb);
    sqlite3_bind_double(stmt, 2, avail_ram_mb);
    sqlite3_bind_double(stmt, 3, used_ram_mb);
    sqlite3_bind_int(stmt, 4, info->memory_load_percent);
    sqlite3_bind_double(stmt, 5, total_page_mb);
    sqlite3_bind_double(stmt, 6, avail_page_mb);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "[SQLITE EXEC ERROR] %s\n", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    if (out_snapshot_id) {
        *out_snapshot_id = sqlite3_last_insert_rowid(g_db);
    }

    return 0;
}

int db_save_process_metric(int64_t snapshot_id, const ProcessMemoryInfo *info) {
    if (!g_db || !info) return -1;

    double ws_mb = (double)info->working_set_bytes / (1024.0 * 1024.0);
    double peak_ws_mb = (double)info->peak_working_set_bytes / (1024.0 * 1024.0);
    double priv_mb = (double)info->private_bytes / (1024.0 * 1024.0);

    const char *sql =
        "INSERT INTO process_metrics (snapshot_id, pid, proc_name, working_set_mb, peak_ws_mb, private_bytes_mb, page_faults) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int64(stmt, 1, snapshot_id);
    sqlite3_bind_int(stmt, 2, info->pid);
    sqlite3_bind_text(stmt, 3, info->process_name, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, ws_mb);
    sqlite3_bind_double(stmt, 5, peak_ws_mb);
    sqlite3_bind_double(stmt, 6, priv_mb);
    sqlite3_bind_int(stmt, 7, info->page_fault_count);

    int step_rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (step_rc == SQLITE_DONE) ? 0 : -1;
}

int db_save_heap_profile(size_t active_bytes, size_t peak_bytes, size_t total_allocs, size_t leaked_bytes) {
    if (!g_db) return -1;

    const char *sql =
        "INSERT INTO heap_profiles (active_bytes, peak_bytes, total_allocations, leaked_bytes) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)active_bytes);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)peak_bytes);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)total_allocs);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)leaked_bytes);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_print_recent_snapshots(int limit) {
    if (!g_db) return -1;

    const char *sql =
        "SELECT id, timestamp, total_ram_mb, used_ram_mb, avail_ram_mb, mem_load_pct "
        "FROM system_snapshots ORDER BY id DESC LIMIT ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, limit);

    printf("\n========================================================================================\n");
    printf("                  STORICO SNAPSHOT MEMORIA (DATABASE SQLITE NATIVO)                     \n");
    printf("========================================================================================\n");
    printf("%-5s | %-19s | %-12s | %-12s | %-12s | %s\n",
           "ID", "Data / Ora", "RAM Totale", "RAM In Uso", "RAM Libera", "Carico");
    printf("----------------------------------------------------------------------------------------\n");

    int rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rows++;
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *ts = sqlite3_column_text(stmt, 1);
        double total = sqlite3_column_double(stmt, 2);
        double used = sqlite3_column_double(stmt, 3);
        double avail = sqlite3_column_double(stmt, 4);
        int load = sqlite3_column_int(stmt, 5);

        printf("#%-4d | %-19s | %9.1f MB | %9.1f MB | %9.1f MB | %3d %%\n",
               id, ts, total, used, avail, load);
    }

    if (rows == 0) {
        printf("  (Nessuno snapshot registrato al momento)\n");
    }

    printf("========================================================================================\n\n");
    sqlite3_finalize(stmt);
    return 0;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}
