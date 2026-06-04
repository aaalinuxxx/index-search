#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

// Выполняет левый поворот вокруг узла x для сохранения свойств сбалансированности
static void leftRotate(RBTree* tree, RBNode* pivot) {
    RBNode* rightChild = pivot->right;
    pivot->right = rightChild->left;

    if (rightChild->left != tree->nil) {
        rightChild->left->parent = pivot;
    }

    rightChild->parent = pivot->parent;

    if (pivot->parent == tree->nil) {
        tree->root = rightChild;
    } else if (pivot == pivot->parent->left) {
        pivot->parent->left = rightChild;
    } else {
        pivot->parent->right = rightChild;
    }

    rightChild->left = pivot;
    pivot->parent = rightChild;
}

// Выполняет правый поворот вокруг узла x для сохранения свойств сбалансированности
static void rightRotate(RBTree* tree, RBNode* pivot) {
    RBNode* leftChild = pivot->left;
    pivot->left = leftChild->right;

    if (leftChild->right != tree->nil) {
        leftChild->right->parent = pivot;
    }

    leftChild->parent = pivot->parent;

    if (pivot->parent == tree->nil) {
        tree->root = leftChild;
    } else if (pivot == pivot->parent->right) {
        pivot->parent->right = leftChild;
    } else {
        pivot->parent->left = leftChild;
    }

    leftChild->right = pivot;
    pivot->parent = leftChild;
}

// Восстанавливает свойства красно-чёрного дерева после вставки нового узла
static void rbInsertFixup(RBTree* tree, RBNode* insertedNode) {
    while (insertedNode->parent->color == RB_RED) {
        if (insertedNode->parent == insertedNode->parent->parent->left) {
            RBNode* uncle = insertedNode->parent->parent->right;
            if (uncle->color == RB_RED) {
                insertedNode->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                insertedNode->parent->parent->color = RB_RED;
                insertedNode = insertedNode->parent->parent;
            } else {
                if (insertedNode == insertedNode->parent->right) {
                    insertedNode = insertedNode->parent;
                    leftRotate(tree, insertedNode);
                }
                insertedNode->parent->color = RB_BLACK;
                insertedNode->parent->parent->color = RB_RED;
                rightRotate(tree, insertedNode->parent->parent);
            }
        } else {
            RBNode* uncle = insertedNode->parent->parent->left;
            if (uncle->color == RB_RED) {
                insertedNode->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                insertedNode->parent->parent->color = RB_RED;
                insertedNode = insertedNode->parent->parent;
            } else {
                if (insertedNode == insertedNode->parent->left) {
                    insertedNode = insertedNode->parent;
                    rightRotate(tree, insertedNode);
                }
                insertedNode->parent->color = RB_BLACK;
                insertedNode->parent->parent->color = RB_RED;
                leftRotate(tree, insertedNode->parent->parent);
            }
        }
    }

    tree->root->color = RB_BLACK;
}

// Создаёт новый узел с заданным ключом и данными, инициализируя его цветом красным
static RBNode* createNode(const char* key, int doc_id, const char* title, RBNode* nil) {
    RBNode* newNode = (RBNode*)malloc(sizeof(RBNode));
    if (newNode == NULL) {
        printf("Не выделилась память\n");
        return NULL;
    }

    newNode->key = strdup(key);
    if (newNode->key == NULL) {
        free(newNode);
        return NULL;
    }

    newNode->color = RB_RED;
    newNode->left = newNode->right = newNode->parent = nil;
    newNode->postings = createPostingList();

    if (newNode->postings == NULL) {
        free(newNode->key);
        free(newNode);
        return NULL;
    }

    appendPosting(newNode->postings, doc_id, title);
    return newNode;
}

// Освобождает память, выделенную под поддерево
static void freeSubtree(RBTree* tree, RBNode* currentNode) {
    if (currentNode == tree->nil) {
        return;
    }

    freeSubtree(tree, currentNode->left);
    freeSubtree(tree, currentNode->right);

    free(currentNode->key);
    vectorFree(currentNode->postings);
    free(currentNode);
}

// Выполняет симметричный обход дерева и вызывает callback для каждого узла
static void traverseInOrder(
    const RBTree* tree,
    RBNode* currentNode,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
) {
    if (currentNode == tree->nil) {
        return;
    }

    traverseInOrder(tree, currentNode->left, visit, ctx);
    visit(currentNode->key, currentNode->postings, ctx);
    traverseInOrder(tree, currentNode->right, visit, ctx);
}

// Инициализирует пустое красно-чёрное дерево
RBTree* createRBTree(void) {
    RBTree* tree = (RBTree*)malloc(sizeof(RBTree));
    if (tree == NULL) {
        printf("Не выделилась память\n");
        return NULL;
    }

    tree->nil = (RBNode*)malloc(sizeof(RBNode));
    if (tree->nil == NULL) {
        free(tree);
        return NULL;
    }

    tree->nil->key = NULL;
    tree->nil->postings = NULL;
    tree->nil->color = RB_BLACK;
    tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;

    tree->root = tree->nil;
    tree->size = 0;
    return tree;
}

// Освобождает память, выделенную под дерево
void freeRBTree(RBTree* tree) {
    if (tree == NULL) {
        return;
    }

    freeSubtree(tree, tree->root);
    free(tree->nil);
    free(tree);
}

// Вставляет новый ключ или обновляет список документов, если ключ уже существует
void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title) {
    if (tree == NULL) {
        return;
    }

    RBNode* parentNode = tree->nil;
    RBNode* currentNode = tree->root;

    while (currentNode != tree->nil) {
        parentNode = currentNode;
        int compareResult = strcmp(key, currentNode->key);

        if (compareResult == 0) {
            appendPosting(currentNode->postings, doc_id, title);
            return;
        }

        currentNode = (compareResult < 0) ? currentNode->left : currentNode->right;
    }

    RBNode* newNode = createNode(key, doc_id, title, tree->nil);
    if (newNode == NULL) {
        return;
    }

    newNode->parent = parentNode;

    if (parentNode == tree->nil) {
        tree->root = newNode;
    } else if (strcmp(newNode->key, parentNode->key) < 0) {
        parentNode->left = newNode;
    } else {
        parentNode->right = newNode;
    }

    rbInsertFixup(tree, newNode);
    tree->size++;
}

// Ищет узел с заданным ключом и возвращает его список документов
Vector* rbSearch(const RBTree* tree, const char* key) {
    if (tree == NULL) {
        return NULL;
    }

    RBNode* currentNode = tree->root;

    while (currentNode != tree->nil) {
        int compareResult = strcmp(key, currentNode->key);

        if (compareResult == 0) {
            return currentNode->postings;
        }

        currentNode = (compareResult < 0) ? currentNode->left : currentNode->right;
    }

    return NULL;
}

// Выполняет симметричный обход всех узлов дерева
void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
) {
    if (tree == NULL || visit == NULL) {
        return;
    }

    traverseInOrder(tree, tree->root, visit, ctx);
}
