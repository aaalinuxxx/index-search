#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../rbtree/rbtree.h"

typedef struct {
    const char* keys[256];
    int count;
} TraverseContext;

static void collectKeys(const char* key, Vector* postings, void* ctx) {
    TraverseContext* data = (TraverseContext*)ctx;
    (void)postings;
    data->keys[data->count++] = key;
}

int main(void) {
    RBTree* tree = createRBTree();
    assert(tree != NULL);
    assert(tree->root == tree->nil);
    assert(tree->size == 0);
    assert(tree->nil->color == RB_BLACK);
    freeRBTree(tree);

    tree = createRBTree();
    assert(tree != NULL);

    rbInsert(tree, "apple", 1, "Doc 1");
    assert(tree->size == 1);
    assert(tree->root != tree->nil);
    assert(tree->root->color == RB_BLACK);
    assert(rbSearch(tree, "apple") != NULL);
    assert(rbSearch(tree, "banana") == NULL);

    rbInsert(tree, "apple", 2, "Doc 2");
    rbInsert(tree, "apple", 3, "Doc 3");
    assert(tree->size == 1);
    assert(rbSearch(tree, "apple") != NULL);

    freeRBTree(tree);

    tree = createRBTree();
    assert(tree != NULL);

    rbInsert(tree, "delta", 1, "Doc 1");
    rbInsert(tree, "alpha", 2, "Doc 2");
    rbInsert(tree, "charlie", 3, "Doc 3");
    rbInsert(tree, "bravo", 4, "Doc 4");

    assert(tree->size == 4);
    assert(tree->root->color == RB_BLACK);
    assert(rbSearch(tree, "alpha") != NULL);
    assert(rbSearch(tree, "bravo") != NULL);
    assert(rbSearch(tree, "charlie") != NULL);
    assert(rbSearch(tree, "delta") != NULL);
    assert(rbSearch(tree, "echo") == NULL);

    TraverseContext ctx1 = { .count = 0 };
    rbTraverse(tree, collectKeys, &ctx1);

    assert(ctx1.count == 4);
    assert(strcmp(ctx1.keys[0], "alpha") == 0);
    assert(strcmp(ctx1.keys[1], "bravo") == 0);
    assert(strcmp(ctx1.keys[2], "charlie") == 0);
    assert(strcmp(ctx1.keys[3], "delta") == 0);

    freeRBTree(tree);

    tree = createRBTree();
    assert(tree != NULL);

    const char* mixedKeys[] = {
        "m", "b", "q", "z", "a", "c", "n", "o", "p", "y", "x", "d"
    };
    int mixedCount = (int)(sizeof(mixedKeys) / sizeof(mixedKeys[0]));

    for (int i = 0; i < mixedCount; i++) {
        rbInsert(tree, mixedKeys[i], i + 1, "Doc");
    }

    assert(tree->size == mixedCount);
    assert(tree->root->color == RB_BLACK);

    for (int i = 0; i < mixedCount; i++) {
        assert(rbSearch(tree, mixedKeys[i]) != NULL);
    }
    assert(rbSearch(tree, "not-present") == NULL);

    TraverseContext ctx2 = { .count = 0 };
    rbTraverse(tree, collectKeys, &ctx2);

    assert(ctx2.count == mixedCount);
    for (int i = 1; i < ctx2.count; i++) {
        assert(strcmp(ctx2.keys[i - 1], ctx2.keys[i]) < 0);
    }

    freeRBTree(tree);

    tree = createRBTree();
    assert(tree != NULL);

    const char* sequentialKeys[] = {
        "01", "02", "03", "04", "05", "06", "07", "08", "09", "10"
    };
    int sequentialCount = (int)(sizeof(sequentialKeys) / sizeof(sequentialKeys[0]));

    for (int i = 0; i < sequentialCount; i++) {
        rbInsert(tree, sequentialKeys[i], i + 1, "Doc");
    }

    assert(tree->size == sequentialCount);
    assert(tree->root->color == RB_BLACK);

    for (int i = 0; i < sequentialCount; i++) {
        assert(rbSearch(tree, sequentialKeys[i]) != NULL);
    }
    assert(rbSearch(tree, "11") == NULL);

    TraverseContext ctx3 = { .count = 0 };
    rbTraverse(tree, collectKeys, &ctx3);

    assert(ctx3.count == sequentialCount);
    for (int i = 1; i < ctx3.count; i++) {
        assert(strcmp(ctx3.keys[i - 1], ctx3.keys[i]) < 0);
    }

    freeRBTree(tree);

    printf("Все тесты прошли успешно.\n");
    return 0;
}
