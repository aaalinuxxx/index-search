#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "btree.h"
#include "../posting.h"

int traverse_count = 0;


// Создание и удаление пустого дерева
void test_create_and_free() {
    printf("Running test_create_and_free... ");
    BTree* tree = createBTree();
    assert(tree != NULL);
    assert(tree->root == NULL);
    assert(tree->size == 0);
    
    freeBTree(tree);
    printf("OK\n");
}

// Одиночная вставка и поиск
void test_single_insert_and_search() {
    printf("Running test_single_insert_and_search... ");
    BTree* tree = createBTree();
    
    btreeInsert(tree, "hello", 1, "First Title");
    
    Vector* res = btreeSearch(tree, "hello");
    assert(res != NULL); // Ключ должен быть найден
    assert(res->size == 1);
    
    Vector* not_found = btreeSearch(tree, "world");
    assert(not_found == NULL); // Ключа 'world' нет
    
    freeBTree(tree);
    printf("OK\n");
}


// Вставка дубликатов
void test_duplicate_insert() {
    printf("Running test_duplicate_insert... ");
    BTree* tree = createBTree();
    
    // Вставляем одно и то же слово, но с разными ID документов
    btreeInsert(tree, "python", 10, "Learn Python");
    btreeInsert(tree, "python", 11, "Python lists");
    btreeInsert(tree, "python", 12, "Advanced Python");
    
    Vector* res = btreeSearch(tree, "python");
    assert(res != NULL);
    
    // Дерево не должно расти в узлах, ключи объединяются в Posting List
    assert(tree->size == 1); 
    assert(res->size == 3); 
    
    freeBTree(tree);
    printf("OK\n");
}


// Множественная вставка и сплит узлов
void test_split_and_large_insert() {
    printf("Running test_split_and_large_insert... ");
    BTree* tree = createBTree();
    
    // Если BTREE_T = 3, то узел сплитится при достижении 5 ключей.
    // Вставим 15 уникальных ключей в случайном/обратном порядке
    char* keys[] = {
        "zebra", "monkey", "apple", "banana", "orange", 
        "grape", "kiwi", "lemon", "mango", "peach",
        "pear", "plum", "cherry", "date", "fig"
    };
    int num_keys = 15;
    
    for (int i = 0; i < num_keys; i++) {
        btreeInsert(tree, keys[i], i + 100, "Dummy Title");
    }
    
    // Проверяем, что все элементы можно найти
    for (int i = 0; i < num_keys; i++) {
        Vector* res = btreeSearch(tree, keys[i]);
        assert(res != NULL);
    }
    
    // Проверяем элемент, которого точно нет
    assert(btreeSearch(tree, "strawberry") == NULL);
    
    freeBTree(tree);
    printf("OK\n");
}

// Проверка обхода и сортировки
void test_visit(const char* key, Vector* postings, void* ctx) {
    (void)postings;
    char** prev_key_ptr = (char**)ctx;
    
    // Проверяем, что обход идет строго в алфавитном порядке
    if (*prev_key_ptr != NULL) {
        assert(strcmp(*prev_key_ptr, key) < 0);
    }
    
    *prev_key_ptr = (char*)key; // Обновляем предыдущий ключ
    traverse_count++;           // Считаем пройденные узлы
}

void test_traverse() {
    printf("Running test_traverse... ");
    BTree* tree = createBTree();
    
    char* keys[] = {"d", "b", "a", "c", "e", "g", "f"};
    for (int i = 0; i < 7; i++) {
        btreeInsert(tree, keys[i], i, "Title");
    }
    
    char* prev_key = NULL;
    traverse_count = 0;
    
    // Запускаем обход. В ctx передаем указатель на prev_key, 
    // чтобы сравнивать текущий ключ с предыдущим (проверка сортировки)
    btreeTraverse(tree, test_visit, &prev_key);
    
    assert(traverse_count == 7); // Должны обойти ровно 7 уникальных ключей
    
    freeBTree(tree);
    printf("OK\n");
}


int main() {
    printf("  Starting B-Tree Tests   \n");
    
    test_create_and_free();
    test_single_insert_and_search();
    test_duplicate_insert();
    test_split_and_large_insert();
    test_traverse();
    
    printf("  All B-Tree tests passed successfully  \n");
    return 0;
}