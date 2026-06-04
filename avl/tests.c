#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "avl.h"

// Счётчики результатов тестов
static int g_passed = 0, g_failed = 0;

/*
RUN(fn) — запускает тестовую функцию fn и печатает её имя.
CHECK(cond, msg) — проверяет условие; при неудаче выводит сообщение и выходит.
PASS() — фиксирует успешный тест.
*/

#define RUN(fn)  do { printf("  %-45s", #fn); fn(); printf("\n"); } while (0)

#define CHECK(cond, msg)                                      \
    do {                                                      \
        if (!(cond)) {                                        \
            printf("FAIL  [%s  строка %d]", msg, __LINE__);  \
            g_failed++;                                       \
            return;                                           \
        }                                                     \
    } while (0)

#define PASS()  do { printf("ok"); g_passed++; } while (0)


// Проверяем начальное состояние нового дерева
static void test_create_empty(void) {
    AVLTree *t = createAVLTree();
    CHECK(t != NULL,       "createAVLTree вернул NULL");
    CHECK(t->size == 0,    "начальный size != 0");
    CHECK(t->root == NULL, "начальный root != NULL");
    freeAVLTree(t);
    PASS();
}

// Вставляем один ключ и проверяем, что поиск возвращает правильную запись
static void test_insert_search_basic(void) {
    AVLTree *t = createAVLTree();
    avlInsert(t, "hello", 42, "Hello World");

    Vector *v = avlSearch(t, "hello");
    CHECK(v != NULL,                            "поиск вернул NULL");
    CHECK(v->size == 1,                         "posting list должен содержать 1 запись");

    PostingEntry *e = getVectorItem(v, 0);
    CHECK(e != NULL,                            "getVectorItem вернул NULL");
    CHECK(e->doc_id == 42,                      "неверный doc_id");
    CHECK(strcmp(e->title, "Hello World") == 0, "неверный title");

    freeAVLTree(t);
    PASS();
}

// Поиск несуществующего ключа должен возвращать NULL
static void test_search_missing_key(void) {
    AVLTree *t = createAVLTree();
    avlInsert(t, "python", 1, "T");

    CHECK(avlSearch(t, "java")    == NULL, "несуществующий ключ должен давать NULL");
    CHECK(avlSearch(t, "pyython") == NULL, "ключ с опечаткой должен давать NULL");

    freeAVLTree(t);
    PASS();
}

// Поиск в пустом дереве должен возвращать NULL, не падать
static void test_search_empty_tree(void) {
    AVLTree *t = createAVLTree();
    CHECK(avlSearch(t, "anything") == NULL, "поиск в пустом дереве должен давать NULL");
    freeAVLTree(t);
    PASS();
}

// При повторной вставке того же ключа новая запись добавляется в существующий posting list, а не создаётся новый узел.
static void test_duplicate_key_appends(void) {
    AVLTree *t = createAVLTree();
    avlInsert(t, "sort", 1, "Sort list");
    avlInsert(t, "sort", 2, "Sort algorithms");
    avlInsert(t, "sort", 3, "Quick sort");

    Vector *v = avlSearch(t, "sort");
    CHECK(v != NULL,       "поиск вернул NULL");
    CHECK(v->size == 3,    "posting list должен содержать 3 записи");
    CHECK(t->size == 1,    "size дерева должен быть 1 - один уникальный ключ");

    // Проверяем порядок добавления
    PostingEntry *e0 = getVectorItem(v, 0);
    PostingEntry *e1 = getVectorItem(v, 1);
    PostingEntry *e2 = getVectorItem(v, 2);
    CHECK(e0->doc_id == 1, "первая запись doc_id");
    CHECK(e1->doc_id == 2, "вторая запись doc_id");
    CHECK(e2->doc_id == 3, "третья запись doc_id");

    freeAVLTree(t);
    PASS();
}

// size должен считать только уникальные ключи
static void test_size_counts_unique_keys(void) {
    AVLTree *t = createAVLTree();
    avlInsert(t, "a", 1, "T");  // size - 1
    avlInsert(t, "b", 2, "T");  // size - 2
    avlInsert(t, "c", 3, "T");  // size - 3
    avlInsert(t, "a", 4, "T");  // дубликат - size остаётся 3
    avlInsert(t, "b", 5, "T");  // дубликат - size остаётся 3

    CHECK(t->size == 3, "size должен быть 3 (только уникальные ключи)");
    freeAVLTree(t);
    PASS();
}

// вспомогательный контекст для проверки обхода
typedef struct {
    char keys[64][256];  // собранные ключи в порядке посещения 
    int  count;          // сколько узлов посещено              
    int  ordered;        // флаг: 1 — порядок возрастающий     
} TCtx;

// собирает ключи и проверяет порядок
static void collectKeys(const char *key, Vector *postings, void *ctx) {
    (void)postings;
    TCtx *tc = ctx;
    // Если предыдущий ключ >= текущего — порядок нарушен
    if (tc->count > 0 && strcmp(tc->keys[tc->count - 1], key) >= 0)
        tc->ordered = 0;
    strncpy(tc->keys[tc->count], key, 255);
    tc->keys[tc->count][255] = '\0';
    tc->count++;
}

// обход должен посещать ключи в алфавитном порядке.
static void test_traverse_sorted_order(void) {
    AVLTree *t = createAVLTree();
    // Вставляем вразнобой
    const char *words[] = {"mango", "apple", "cherry", "banana", "date", "elderberry"};
    for (int i = 0; i < 6; i++)
        avlInsert(t, words[i], i, "T");

    TCtx tc = { .count = 0, .ordered = 1 };
    avlTraverse(t, collectKeys, &tc);

    CHECK(tc.count == 6, "обход должен посетить ровно 6 узлов");
    CHECK(tc.ordered,    "ключи должны идти в возрастающем порядке");

    freeAVLTree(t);
    PASS();
}

// Обход пустого дерева не должен ни крашиться, ни посещать узлы.
static void test_traverse_empty(void) {
    AVLTree *t = createAVLTree();
    TCtx tc = { .count = 0, .ordered = 1 };
    avlTraverse(t, collectKeys, &tc);
    CHECK(tc.count == 0, "обход пустого дерева должен посетить 0 узлов");
    freeAVLTree(t);
    PASS();
}

// Последовательная вставка отсортированных ключей без балансировки создала бы «палку» высотой 127. AVL должен удержать высоту ≤ 14.
static void test_balance_sequential_insert(void) {
    AVLTree *t = createAVLTree();
    for (int i = 0; i < 127; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%04d", i);
        avlInsert(t, key, i, "Title");
    }
    CHECK(t->size == 127,          "size должен быть 127");
    // Теоретический максимум: 1.44 * log2(128) ≈ 10.1; берём 14 с запасом
    CHECK(t->root->height <= 14,  "дерево слишком высокое - балансировка не работает");

    freeAVLTree(t);
    PASS();
}

// То же, но ключи в обратном порядке
static void test_balance_reverse_insert(void) {
    AVLTree *t = createAVLTree();
    for (int i = 127; i >= 0; i--) {
        char key[32];
        snprintf(key, sizeof(key), "key%04d", i);
        avlInsert(t, key, i, "Title");
    }
    CHECK(t->size == 128,         "size должен быть 128");
    CHECK(t->root->height <= 14,  "дерево слишком высокое — балансировка не работает");

    freeAVLTree(t);
    PASS();
}

// Разные ключи должны иметь независимые posting list'ы нужного размера
static void test_multiple_independent_keys(void) {
    AVLTree *t = createAVLTree();
    avlInsert(t, "python", 1, "Python basics");
    avlInsert(t, "java",   2, "Java intro");
    avlInsert(t, "python", 3, "Python advanced");
    avlInsert(t, "c",      4, "C programming");

    Vector *vp = avlSearch(t, "python");
    Vector *vj = avlSearch(t, "java");
    Vector *vc = avlSearch(t, "c");

    CHECK(vp && vp->size == 2, "python должен иметь 2 записи");
    CHECK(vj && vj->size == 1, "java должен иметь 1 запись");
    CHECK(vc && vc->size == 1, "c должен иметь 1 запись");
    CHECK(t->size == 3,        "дерево должно иметь 3 уникальных ключа");

    freeAVLTree(t);
    PASS();
}

// NULL-аргументы не должны приводить к сбою программы
static void test_null_safety(void) {
    avlInsert(NULL, "key", 1, "T");  
    avlInsert(NULL, NULL,  1, "T");  

    AVLTree *t = createAVLTree();
    avlInsert(t, NULL, 1, "T");                      

    CHECK(avlSearch(NULL, "key") == NULL, "поиск в NULL дереве");
    CHECK(avlSearch(t, NULL)     == NULL, "поиск с NULL ключом");

    avlTraverse(NULL, NULL, NULL);  
    freeAVLTree(NULL);              
    freeAVLTree(t);
    PASS();
}


int main(void) {
    printf("Тесты AVL-дерева\n\n");

    RUN(test_create_empty);
    RUN(test_insert_search_basic);
    RUN(test_search_missing_key);
    RUN(test_search_empty_tree);
    RUN(test_duplicate_key_appends);
    RUN(test_size_counts_unique_keys);
    RUN(test_traverse_sorted_order);
    RUN(test_traverse_empty);
    RUN(test_balance_sequential_insert);
    RUN(test_balance_reverse_insert);
    RUN(test_multiple_independent_keys);
    RUN(test_null_safety);

    printf("\nИтого: %d прошло, %d упало\n", g_passed, g_failed);
    return g_failed ? 1 : 0;
}
