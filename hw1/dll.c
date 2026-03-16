#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DllNode {
    struct DllNode *prev;
    struct DllNode *next;
    char *word;
} DllNode;

typedef struct {
    DllNode *head;
    DllNode *tail;
} Dll;

Dll *newDll() {
    Dll *dll = malloc(sizeof(Dll));
    dll->head = NULL;
    dll->tail = NULL;
    return dll;
}

static DllNode *newDllNode(char *word) {
    DllNode *node = (DllNode *)malloc(sizeof(DllNode));
    node->word = malloc(strlen(word) + 1);
    strcpy(node->word, word);
    node->prev = NULL;
    node->next = NULL;
    return node;
}

static void dllSwap(Dll *list, DllNode *a, DllNode *b) {
    if (a == b) {
        return;
    }

    DllNode *a_prev = a->prev;
    DllNode *a_next = a->next;
    DllNode *b_prev = b->prev;
    DllNode *b_next = b->next;

    // Update head/tail
    if (a == list->head) {
        list->head = b;
    } else if (b == list->head) {
        list->head = a;
    }

    if (a == list->tail) {
        list->tail = b;
    } else if (b == list->tail) {
        list->tail = a;
    }

    // Handle the three cases: a before b, b before a, non-adjacent
    if (a->next == b) {
        a->prev = b;
        a->next = b_next;
        b->prev = a_prev;
        b->next = a;
        if (a_prev)
            a_prev->next = b;
        if (b_next)
            b_next->prev = a;
    } else if (b->next == a) {
        a->prev = b_prev;
        a->next = b;
        b->prev = a;
        b->next = a_next;
        if (b_prev)
            b_prev->next = a;
        if (a_next)
            a_next->prev = b;
    } else {
        a->prev = b_prev;
        a->next = b_next;
        b->prev = a_prev;
        b->next = a_next;
        if (a_prev)
            a_prev->next = b;
        if (a_next)
            a_next->prev = b;
        if (b_prev)
            b_prev->next = a;
        if (b_next)
            b_next->prev = a;
    }
}

void dllAppend(Dll *list, char *word) {
    DllNode *node = newDllNode(word);
    if (list->head == NULL) {
        list->head = node;
    }
    if (list->tail == NULL) {
        list->tail = node;
    } else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }
}

void dllSort(Dll *list) {
    if (list == NULL)
        return;
    if (list->head == NULL)
        return;
    if (list->head->next == NULL)
        // Length 1
        return;
    while (true) {
        bool sorted = true;
        DllNode *cur = list->head;
        while (cur->next != NULL) {
            DllNode *next = cur->next;
            if (strcmp(cur->word, next->word) > 0) {
                dllSwap(list, cur, next);
                sorted = false;
            }
            cur = next;
        }
        if (sorted) {
            break;
        }
    }
}

void dllDeleteNode(Dll *list, DllNode *node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        // Node is head
        list->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        // Node is tail
        list->tail = node->prev;
    }

    free(node->word);
    free(node);
}

size_t dllDeleteDuplicatesSorted(Dll *list) {
    // Think of adding an assert to this to check that it is sorted
    size_t deleted = 0;
    DllNode *cur = list->head;
    while (cur != NULL && cur->next != NULL) {
        DllNode *next = cur->next;
        if (strcmp(cur->word, next->word) == 0) {
            dllDeleteNode(list, next);
            deleted++;
            // We need to check if cur and the new next are also a duplicate
            // So we don't advance cur
        } else {
            cur = cur->next;
        }
    }
    return deleted;
}

void dllFree(Dll *list) {
    DllNode *cur = list->head;
    while (cur != NULL) {
        DllNode *next = cur->next;
        free(cur->word);
        free(cur);
        cur = next;
    }
    free(list);
}

void dllPrint(Dll *list) {
    DllNode *cur = list->head;
    while (cur != NULL) {
        printf("%s -> ", cur->word);
        cur = cur->next;
    }
    printf("\n");
}

void dllPrintReversed(Dll *list) {
    DllNode *cur = list->tail;
    while (cur != NULL) {
        printf("%s -> ", cur->word);
        cur = cur->prev;
    }
    printf("\n");
}
