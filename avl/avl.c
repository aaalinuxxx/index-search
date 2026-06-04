#include <stdlib.h>
#include <string.h>
#include "avl.h"


// Возвращает высоту узла; для NULL возвращает 0
static int nodeHeight(const AVLNode *n) {
    return n ? n->height : 0;
}

static int max2(int a, int b) {
    return a > b ? a : b;
}

// Пересчитывает высоту узла на основе его потомков
static void updateHeight(AVLNode *n) {
    n->height = 1 + max2(nodeHeight(n->left), nodeHeight(n->right));
}

// Фактор баланса: > 0 — левое поддерево выше, < 0 — правое                  
static int balanceFactor(const AVLNode *n) {
    return nodeHeight(n->left) - nodeHeight(n->right);
}

// Правое вращение вокруг y:
static AVLNode *rotateRight(AVLNode *y) {
    AVLNode *x  = y->left;
    AVLNode *t2 = x->right;   // сохраняем правое поддерево x
    x->right = y;
    y->left  = t2;
    updateHeight(y);  // сначала y, потому что теперь оно ниже
    updateHeight(x);
    return x;         // x становится новым корнем поддерева
}

// Левое вращение вокруг x:
static AVLNode *rotateLeft(AVLNode *x) {
    AVLNode *y  = x->right;
    AVLNode *t2 = y->left;    // сохраняем левое поддерево y
    y->left  = x;
    x->right = t2;
    updateHeight(x);
    updateHeight(y);
    return y;
}

/*
   Восстанавливает свойство AVL после вставки.
   Возможные случаи:
 Left-Left   - одно правое вращение
 Left-Right  - левое вращение левого потомка + правое вращение
 Right-Right - одно левое вращение
 Right-Left  - правое вращение правого потомка + левое вращение
*/
static AVLNode *rebalance(AVLNode *n) {
    updateHeight(n);
    int bf = balanceFactor(n);

    if (bf > 1) {                        // левое поддерево перевешивает
        if (balanceFactor(n->left) < 0)  // Left-Right: сначала левое вращение
            n->left = rotateLeft(n->left);
        return rotateRight(n);
    }
    if (bf < -1) {                        // правое поддерево перевешивает
        if (balanceFactor(n->right) > 0)  // Right-Left: сначала правое вращение
            n->right = rotateRight(n->right);
        return rotateLeft(n);
    }
    return n;  // дерево сбалансировано, ничего не делаем
}

// создание узла

// Выделяет новый узел с ключом и первой записью в posting list
static AVLNode *newNode(const char *key, int doc_id, const char *title) {
    AVLNode *n = malloc(sizeof(AVLNode));
    if (!n) return NULL;

    n->key = strdup(key);
    if (!n->key) { free(n); return NULL; }

    n->height   = 1;
    n->left     = n->right = NULL;
    n->postings = createPostingList();
    if (!n->postings) { free(n->key); free(n); return NULL; }

    appendPosting(n->postings, doc_id, title);
    return n;
}


/*
Вставляет ключ в поддерево с корнем node.
Если ключ уже существует — добавляет запись в его posting list.
Флаг created устанавливается в 1, только если создан новый узел.
Возвращает (возможно новый) корень поддерева после балансировки.
*/
static AVLNode *insertNode(AVLNode *node, const char *key,
                            int doc_id, const char *title, int *created) {
    if (!node) {
        *created = 1;
        return newNode(key, doc_id, title);
    }

    int cmp = strcmp(key, node->key);
    if (cmp < 0)
        node->left  = insertNode(node->left,  key, doc_id, title, created);
    else if (cmp > 0)
        node->right = insertNode(node->right, key, doc_id, title, created);
    else
        appendPosting(node->postings, doc_id, title);  // ключ уже есть

    // Балансируем только при реальной вставке нового узла
    return cmp != 0 ? rebalance(node) : node;
}

// Рекурсивно освобождает всё поддерево 
static void freeNode(AVLNode *n) {
    if (!n) return;
    freeNode(n->left);
    freeNode(n->right);
    free(n->key);
    vectorFree(n->postings);
    free(n);
}

// Итеративный поиск: идёт влево/вправо по ключу
static Vector *searchNode(const AVLNode *n, const char *key) {
    while (n) {
        int cmp = strcmp(key, n->key);
        if      (cmp < 0) n = n->left;
        else if (cmp > 0) n = n->right;
        else              return n->postings;
    }
    return NULL;  // ключ не найден
}

// Симметричный обход: ключи посещаются в алфавитном порядке.
static void traverseNode(const AVLNode *n,
                          void (*visit)(const char *, Vector *, void *),
                          void *ctx) {
    if (!n) return;
    traverseNode(n->left,  visit, ctx);
    visit(n->key, n->postings, ctx);
    traverseNode(n->right, visit, ctx);
}

// Создаёт пустое AVL-дерево
AVLTree *createAVLTree(void) {
    AVLTree *t = malloc(sizeof(AVLTree));
    if (!t) return NULL;
    t->root = NULL;
    t->size = 0;  // количество уникальных ключей
    return t;
}

// Освобождает дерево и все его узлы вместе с posting list'ами
void freeAVLTree(AVLTree *tree) {
    if (!tree) return;
    freeNode(tree->root);
    free(tree);
}

/*
Вставляет пару (key, doc_id, title) в дерево.
Если ключ уже есть — дописывает в его posting list.
Если новый — создаёт узел и сохраняет баланс дерева.
*/
void avlInsert(AVLTree *tree, const char *key, int doc_id, const char *title) {
    if (!tree || !key) return;
    int created = 0;
    tree->root = insertNode(tree->root, key, doc_id, title, &created);
    if (created) tree->size++;
}

// Ищет ключ в дереве. Возвращает posting list или NULL, если ключ не найден
Vector *avlSearch(const AVLTree *tree, const char *key) {
    if (!tree || !key) return NULL;
    return searchNode(tree->root, key);
}

/*
Обходит дерево in-order и для каждого узла вызывает visit(key, postings, ctx).
Ключи передаются в visit в алфавитном порядке.
*/
void avlTraverse(const AVLTree *tree,
                 void (*visit)(const char *key, Vector *postings, void *ctx),
                 void *ctx) {
    if (!tree || !visit) return;
    traverseNode(tree->root, visit, ctx);
}
