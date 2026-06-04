#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"

Index* createIndex(TreeType type) {
    Index* idx = (Index*)malloc(sizeof(Index));
    if (!idx) return NULL;
    
    idx->type = type;
    switch (type) {
        case TREE_AVL:   idx->tree = createAVLTree(); break;
        case TREE_RB:    idx->tree = createRBTree(); break;
        case TREE_BTREE: idx->tree = createBTree(); break;
    }
    return idx;
}

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {
    if (!idx || !idx->tree) return;
    
    switch (idx->type) {
        case TREE_AVL:   avlInsert((AVLTree*)idx->tree, term, doc_id, title); break;
        case TREE_RB:    rbInsert((RBTree*)idx->tree, term, doc_id, title); break;
        case TREE_BTREE: btreeInsert((BTree*)idx->tree, term, doc_id, title); break;
    }
}

Vector* lookupTerm(const Index* idx, const char* term) {
    if (!idx || !idx->tree) return NULL;
    
    switch (idx->type) {
        case TREE_AVL:   return avlSearch((const AVLTree*)idx->tree, term);
        case TREE_RB:    return rbSearch((const RBTree*)idx->tree, term);
        case TREE_BTREE: return btreeSearch((const BTree*)idx->tree, term);
    }
    return NULL;
}

void indexDocument(Index* idx, int doc_id, const char* title, const char** tokens, int n_tokens) {
    for (int i = 0; i < n_tokens; i++) {
        insertTerm(idx, tokens[i], doc_id, title);
    }
}

void traverseIndex(const Index* idx, void (*visit)(const char* key, Vector* postings, void* ctx), void* ctx) {
    if (!idx || !idx->tree) return;
    
    switch (idx->type) {
        case TREE_AVL:   avlTraverse((const AVLTree*)idx->tree, visit, ctx); break;
        case TREE_RB:    rbTraverse((const RBTree*)idx->tree, visit, ctx); break;
        case TREE_BTREE: btreeTraverse((const BTree*)idx->tree, visit, ctx); break;
    }
}


static void save_visit(const char* key, Vector* postings, void* ctx) {
    FILE* f = (FILE*)ctx;
    fprintf(f, "TERM %s %zu\n", key, postings->size);
    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* pe = (PostingEntry*)getVectorItem(postings, i);
        fprintf(f, "%d %s\n", pe->doc_id, pe->title);
    }
}

void saveIndex(const Index* idx, const char* path) {
    FILE* f = fopen(path, "wb"); 
    if (!f) return;
    traverseIndex(idx, save_visit, f);
    fclose(f);
}


Index* loadIndex(const char* path, TreeType type) {
    FILE* f = fopen(path, "rb"); 
    if (!f) return NULL;
    
    Index* idx = createIndex(type);
    
    char line[65536]; 
    char current_term[256] = "";
    int is_reading_docs = 0;
    int docs_left = 0;
    
    while (fgets(line, sizeof(line), f)) {
        // Если видим строгий маркер TERM — значит началось новое слово
        if (strncmp(line, "TERM ", 5) == 0) {
            if (sscanf(line, "TERM %255s %d", current_term, &docs_left) == 2) {
                is_reading_docs = 1;
            }
            continue;
        }
        
        // Если мы сейчас читаем документы для этого слова
        if (is_reading_docs && docs_left > 0) {
            int doc_id;
            char title[65536];
            title[0] = '\0';
            
            int parsed = sscanf(line, "%d %65535[^\n]", &doc_id, title);
            if (parsed >= 1) {
                // Очищаем \r на конце
                size_t len = strlen(title);
                if (len > 0 && title[len-1] == '\r') title[len-1] = '\0';
                
                insertTerm(idx, current_term, doc_id, title);
                docs_left--;
            }
        }
    }
    
    fclose(f);
    return idx;
}

void freeIndex(Index* idx) {
    if (!idx) return;
    switch (idx->type) {
        case TREE_AVL:   freeAVLTree((AVLTree*)idx->tree); break;
        case TREE_RB:    freeRBTree((RBTree*)idx->tree); break;
        case TREE_BTREE: freeBTree((BTree*)idx->tree); break;
    }
    free(idx);
}