#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <sys/resource.h>
#endif

#include "index/index.h"
#include "index/search.h"

// Вспомогательные функции для парсинга аргументов
TreeType parseType(const char* str) {
    if (strcmp(str, "avl") == 0) return TREE_AVL;
    if (strcmp(str, "rb") == 0) return TREE_RB;
    return TREE_BTREE;
}

const char* typeName(TreeType type) {
    if (type == TREE_AVL) return "avl";
    if (type == TREE_RB) return "rb";
    return "btree";
}

// Функция индексации (читает docs.jsonl и строит дерево)
static void runIndex(TreeType type, const char* data_path, const char* idx_path) {
    FILE* f = fopen(data_path, "r");
    if (!f) {
        fprintf(stderr, "[Error] Failed to open %s\n", data_path);
        return;
    }

    Index* idx = createIndex(type);
    char line[4096];
    int count = 0;

    printf("Building %s index from %s...\n", typeName(type), data_path);

    while (fgets(line, sizeof(line), f)) {
        // парсинг JSONL строки: {"doc_id": "123", "title": "abc", "tokens": ["t1", "t2"]}
        char* doc_id_str = strstr(line, "\"doc_id\": \"");
        char* title_str  = strstr(line, "\"title\": \"");
        char* tokens_str = strstr(line, "\"tokens\": [");

        if (!doc_id_str || !title_str || !tokens_str) continue;

        doc_id_str += 11;
        int doc_id = atoi(doc_id_str);

        title_str += 10;
        char* title_end = strchr(title_str, '"');
        if (title_end) *title_end = '\0';

        tokens_str += 11;
        char* tokens_end = strchr(tokens_str, ']');
        if (tokens_end) *tokens_end = '\0';

        const char* tokens[256];
        int n_tokens = 0;
        char* token = strtok(tokens_str, "\", \t\r\n");
        while (token && n_tokens < 256) {
            tokens[n_tokens++] = token;
            token = strtok(NULL, "\", \t\r\n");
        }

        indexDocument(idx, doc_id, title_str, tokens, n_tokens);
        count++;
    }

    fclose(f);
    saveIndex(idx, idx_path);

    // замеры памяти
    #ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            size_t peak_kb = pmc.PeakWorkingSetSize / 1024;
            printf("Memory: %zu KB (%.2f MB)\n", peak_kb, peak_kb / 1024.0);
        }
    #else
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        long peak_kb = usage.ru_maxrss;
    #ifdef __APPLE__
        peak_kb /= 1024; // На Mac эта функция возвращает байты, а не килобайты
    #endif
        printf("Memory: %ld KB (%.2f MB)\n", peak_kb, peak_kb / 1024.0);
    #endif

    freeIndex(idx);
    printf("Successfully indexed %d documents. Saved to %s\n", count, idx_path);
}


// Запуск поиска по готовому индексу
static void runSearch(TreeType type, const char* idx_path, const char* query, int json_out) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) { 
        fprintf(stderr, "Failed to load index: %s\n", idx_path); 
        exit(1); 
    }

    SearchResults* sr = search(idx, query);
    
    if (json_out) printResultsJSON(sr);
    else          printResultsText(sr);

    freeSearchResults(sr);
    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n",
        prog, prog);
}


static void runBenchmark(TreeType type, const char* idx_path) {
    printf("Loading %s index into memory (please wait)...\n", typeName(type));
    Index* idx = loadIndex(idx_path, type);
    if (!idx) { 
        fprintf(stderr, "Failed to load index: %s\n", idx_path); 
        return; 
    }
    printf("Index loaded! Running 1000 queries for each length...\n\n");

    const char* q1[] = {"python", "sql", "server", "error", "list", "c#", "windows", "database", "string", "array"};
    const char* q2[] = {"python list", "sql server", "c# string", "windows error", "database query", "string format", "memory leak", "web server", "data type", "visual studio"};
    const char* q3[] = {"python list sort", "sql server 2005", "c# string format", "windows memory leak", "visual studio error", "database query optimization", "how to use", "error out of", "get data from", "set environment variable"};

    double total_1 = 0, total_2 = 0, total_3 = 0;

    // Гоняем эти 10 запросов 100 раз (в сумме 1000 запросов каждого типа)
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 10; j++) {
            SearchResults* sr1 = search(idx, q1[j]); 
            total_1 += sr1->time_ms; 
            freeSearchResults(sr1);
            
            SearchResults* sr2 = search(idx, q2[j]); 
            total_2 += sr2->time_ms; 
            freeSearchResults(sr2);
            
            SearchResults* sr3 = search(idx, q3[j]); 
            total_3 += sr3->time_ms; 
            freeSearchResults(sr3);
        }
    }

    printf("--- Benchmark Results for %s ---\n", typeName(type));
    printf("1 слово : %.4f мс / запрос\n", total_1 / 1000.0);
    printf("2 слова : %.4f мс / запрос\n", total_2 / 1000.0);
    printf("3 слова : %.4f мс / запрос\n", total_3 / 1000.0);

    freeIndex(idx);
}


int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    const char* mode = argv[1];
    TreeType    type = TREE_AVL;
    const char* data_path = "data/processed/docs.jsonl";
    char        idx_path[512] = {0};
    int         json_out = 0;
    const char* query    = NULL;

    for (int i = 2; i < argc; i++) {
        if      (strncmp(argv[i], "--type=",  7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=",  7) == 0) data_path = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0)
            strncpy(idx_path, argv[i] + 8, sizeof(idx_path) - 1);
        else if (strcmp(argv[i], "--json")    == 0)    json_out = 1;
        else if (argv[i][0] != '-')                    query = argv[i];
    }

    if (idx_path[0] == '\0')
        snprintf(idx_path, sizeof(idx_path), "data/index_%s.txt", typeName(type));

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    } else if (strcmp(mode, "search") == 0) {
        if (!query) { fprintf(stderr, "No query provided\n"); return 1; }
        runSearch(type, idx_path, query, json_out);
    } else if (strcmp(mode, "benchmark") == 0) {     
        runBenchmark(type, idx_path);               
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        usage(argv[0]);
        return 1;
    }
    return 0;
}