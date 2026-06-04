#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "search.h"

// Возвращает текущее время в миллисекундах
static double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// Пересечение нескольких posting list. Возвращает новый список, который нужно освободить.
Vector* intersectPostings(Vector** lists, int n) {
    Vector* result = createPostingList();
    if (n == 0) return result;
    
    Vector* base = lists[0];
    size_t* cursors = (size_t*)calloc(n, sizeof(size_t));
    
    for (size_t i = 0; i < base->size; i++) {
        PostingEntry* pe = (PostingEntry*)getVectorItem(base, i);
        int target_doc_id = pe->doc_id;
        
        // Пропускаем дубликаты
        if (i > 0) {
            PostingEntry* prev = (PostingEntry*)getVectorItem(base, i - 1);
            if (prev->doc_id == target_doc_id) continue;
        }
        
        int found_in_all = 1;
        for (int j = 1; j < n; j++) {
            int found_here = 0;
            Vector* other = lists[j];
            
            while (cursors[j] < other->size) {
                PostingEntry* ope = (PostingEntry*)getVectorItem(other, cursors[j]);
                if (ope->doc_id == target_doc_id) {
                    found_here = 1;
                    break;
                } else if (ope->doc_id > target_doc_id) {
                    break;
                }
                cursors[j]++;
            }
            
            if (!found_here) {
                found_in_all = 0;
                break;
            }
        }
        
        if (found_in_all) {
            appendPosting(result, pe->doc_id, pe->title);
        }
    }
    
    free(cursors);
    return result;
}

// Выполняет поиск по запросу. Возвращает результаты, которые нужно освободить.
SearchResults* search(Index* idx, const char* query) {
    double start_time = get_time_ms();
    
    char* q_copy = strdup(query);
    char* token = strtok(q_copy, " \t\r\n");
    Vector* lists[128]; 
    int n_tokens = 0;
    
    while (token && n_tokens < 128) {
        Vector* pl = lookupTerm(idx, token);
        if (!pl) {
            free(q_copy);
            SearchResults* sr = (SearchResults*)malloc(sizeof(SearchResults));
            sr->results = createVector(sizeof(SearchResult)); 
            sr->total = 0;
            sr->time_ms = get_time_ms() - start_time;
            return sr;
        }
        lists[n_tokens++] = pl;
        token = strtok(NULL, " \t\r\n");
    }
    free(q_copy);
    
    Vector* intersection = intersectPostings(lists, n_tokens);
    
    SearchResults* sr = (SearchResults*)malloc(sizeof(SearchResults));
    sr->total = intersection ? intersection->size : 0;
    sr->results = createVector(sizeof(SearchResult)); 
    
    int limit = sr->total > 10 ? 10 : sr->total;
    for (int i = 0; i < limit; i++) {
        PostingEntry* pe = (PostingEntry*)getVectorItem(intersection, i);
        
        SearchResult s_res;
        s_res.doc_id = pe->doc_id;
        strncpy(s_res.title, pe->title, MAX_TITLE_LEN - 1);
        s_res.title[MAX_TITLE_LEN - 1] = '\0';
        s_res.score = n_tokens; 
        
        appendVectorItem(sr->results, &s_res); 
    }
    
    if (intersection) {
        vectorFree(intersection);
    }
    
    sr->time_ms = get_time_ms() - start_time;
    return sr;
}

// Печатает результаты в текстовом виде
void printResultsText(const SearchResults* sr) {
    if (!sr) return;
    printf("Время: %.2f мс | Найдено: %d документов\n\n", sr->time_ms, sr->total);
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = (SearchResult*)getVectorItem(sr->results, i);
        printf(" %2zu. [id=%d] %s\n", i + 1, r->doc_id, r->title);
    }
}

// Печатает результаты в JSON-формате
void printResultsJSON(const SearchResults* sr) {
    if (!sr) return;
    printf("{\n");
    printf("  \"time_ms\": %.2f,\n", sr->time_ms);
    printf("  \"total\": %d,\n", sr->total);
    printf("  \"results\": [\n");
    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = (SearchResult*)getVectorItem(sr->results, i);
        printf("    {\"doc_id\": %d, \"title\": \"%s\", \"score\": %d}%s\n",
               r->doc_id, r->title, r->score,
               (i == sr->results->size - 1) ? "" : ",");
    }
    printf("  ]\n");
    printf("}\n");
}

void freeSearchResults(SearchResults* sr) {
    if (!sr) return;
    if (sr->results) {
        vectorFree(sr->results);
    }
    free(sr);
}