#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"

// Вспомогательная функция создания нового узла
static BTreeNode* createNode(int is_leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (!node) {
        fprintf(stderr, "[B-Tree] Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    
    node->n = 0;
    node->is_leaf = is_leaf;
    
    for (int i = 0; i < BTREE_MAX_KEYS; i++) {
        node->keys[i] = NULL;
        node->postings[i] = NULL;
    }
    for (int i = 0; i < BTREE_MAX_CH; i++) {
        node->children[i] = NULL;
    }
    
    return node;
}

// Инициализация дерева
BTree* createBTree(void) {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (!tree) return NULL;
    
    tree->root = NULL;
    tree->size = 0;
    return tree;
}


// Поиск по дереву. Возвращает указатель на posting list или NULL, если ключ не найден.
Vector* btreeSearch(const BTree* tree, const char* key) {
    if (!tree || !tree->root) return NULL;
    
    BTreeNode* curr = tree->root;
    while (curr) {
        int i = 0;
        // Ищем первый ключ, который больше или равен искомому
        while (i < curr->n && strcmp(key, curr->keys[i]) > 0) {
            i++;
        }
        
        // Если нашли точное совпадение
        if (i < curr->n && strcmp(key, curr->keys[i]) == 0) {
            return curr->postings[i];
        }
        
        // Если мы в листе и не нашли ключ
        if (curr->is_leaf) {
            return NULL;
        }
        
        // Идем в соответствующего потомка
        curr = curr->children[i];
    }
    
    return NULL;
}


// алгоритмы вставки теперь

// Разделение переполненного дочернего узла y.
// i — индекс y в массиве children родителя (parent)
static void splitChild(BTreeNode* parent, int i, BTreeNode* y) {
    // Создаем новый узел z, который будет братом y
    BTreeNode* z = createNode(y->is_leaf);
    z->n = BTREE_T - 1;

    // Переносим последние T-1 ключей и постингов из y в z
    for (int j = 0; j < BTREE_T - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_T];
        z->postings[j] = y->postings[j + BTREE_T];
        
        // Зануляем старые указатели, чтобы избежать double free
        y->keys[j + BTREE_T] = NULL;
        y->postings[j + BTREE_T] = NULL;
    }

    // Если y не лист, переносим T потомков из y в z
    if (!y->is_leaf) {
        for (int j = 0; j < BTREE_T; j++) {
            z->children[j] = y->children[j + BTREE_T];
            y->children[j + BTREE_T] = NULL;
        }
    }

    // Уменьшаем количество ключей в y
    y->n = BTREE_T - 1;

    // Сдвигаем потомков родителя вправо, чтобы освободить место для z
    for (int j = parent->n; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = z;

    // Сдвигаем ключи родителя вправо, чтобы освободить место для медианы
    for (int j = parent->n - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->postings[j + 1] = parent->postings[j];
    }

    // Поднимаем медианный ключ из y в parent
    parent->keys[i] = y->keys[BTREE_T - 1];
    parent->postings[i] = y->postings[BTREE_T - 1];
    
    y->keys[BTREE_T - 1] = NULL; // Защита
    y->postings[BTREE_T - 1] = NULL;
    
    parent->n++;
}

// Вставка в непереполненный узел
static void insertNonFull(BTreeNode* x, const char* key, int doc_id, const char* title) {
    int i = x->n - 1;

    if (x->is_leaf) {
        // Сдвигаем ключи, чтобы найти место для нового
        while (i >= 0 && strcmp(key, x->keys[i]) < 0) {
            x->keys[i + 1] = x->keys[i];
            x->postings[i + 1] = x->postings[i];
            i--;
        }
        
        // Вставляем новый ключ
        x->keys[i + 1] = strdup(key);
        // создаем новый posting list для этого ключа и добавляем запись
        x->postings[i + 1] = createPostingList(); 
        appendPosting(x->postings[i + 1], doc_id, title);
        
        x->n++;
    } else {
        // Ищем потомка, в которого нужно спуститься
        while (i >= 0 && strcmp(key, x->keys[i]) < 0) {
            i--;
        }
        i++;
        
        // Если потомок переполнен, разделяем его
        if (x->children[i]->n == BTREE_MAX_KEYS) {
            splitChild(x, i, x->children[i]);
            // После разделения средний ключ поднимается
            // Проверяем, в какую из половин нужно идти
            if (strcmp(key, x->keys[i]) > 0) {
                i++;
            }
        }
        insertNonFull(x->children[i], key, doc_id, title);
    }
}

// Главная функция вставки
void btreeInsert(BTree* tree, const char* key, int doc_id, const char* title) {
    if (!tree) return;

    // Поисковый индекс требует добавления в список, если ключ уже есть.
    Vector* existing_postings = btreeSearch(tree, key);
    if (existing_postings != NULL) {
        // Ключ найден. Просто добавляем новый документ в Posting List.
        appendPosting(existing_postings, doc_id, title);
        return;
    }

    // Ключа нет в дереве. Нужно вставить новый узел.
    if (!tree->root) {
        tree->root = createNode(1); // Создаем корень-лист
    }

    BTreeNode* root = tree->root;
    
    // Если корень переполнен, дерево растет в высоту
    if (root->n == BTREE_MAX_KEYS) {
        BTreeNode* new_root = createNode(0); // Новый корень (не лист)
        tree->root = new_root;
        new_root->children[0] = root;
        
        splitChild(new_root, 0, root);
        insertNonFull(new_root, key, doc_id, title);
    } else {
        insertNonFull(root, key, doc_id, title);
    }
    
    tree->size++;
}


// обход и очистка дерева

static void traverseNode(BTreeNode* node, void (*visit)(const char*, Vector*, void*), void* ctx) {
    if (!node) return;
    
    int i;
    for (i = 0; i < node->n; i++) {
        // Сначала идем в левого потомка
        if (!node->is_leaf) {
            traverseNode(node->children[i], visit, ctx);
        }
        // Затем обрабатываем сам ключ
        visit(node->keys[i], node->postings[i], ctx);
    }
    // Обрабатываем самого правого потомка
    if (!node->is_leaf) {
        traverseNode(node->children[i], visit, ctx);
    }
}

void btreeTraverse(const BTree* tree, void (*visit)(const char*, Vector*, void*), void* ctx) {
    if (tree && tree->root) {
        traverseNode(tree->root, visit, ctx);
    }
}

static void freeNode(BTreeNode* node) {
    if (!node) return;
    
    // Освобождаем ключи и списки постингов
    for (int i = 0; i < node->n; i++) {
        free(node->keys[i]);
        if (node->postings[i]) {
            // Освобождаем все записи в posting list, а затем сам список
            vectorFree(node->postings[i]); 
        }
    }
    
    // Рекурсивно освобождаем потомков
    if (!node->is_leaf) {
        for (int i = 0; i <= node->n; i++) {
            freeNode(node->children[i]);
        }
    }
    
    free(node);
}

void freeBTree(BTree* tree) {
    if (!tree) return;
    freeNode(tree->root);
    free(tree);
}