#include "dll.c"
#include <stdbool.h>
#include <string.h>
#include <time.h>

typedef long long ll;

ll sumChars(Dll *);
bool isSorted(Dll *);

int main() {
    Dll *list = newDll();

    srand(time(NULL));

    for (int i = 0; i < 100; i++) {
        char str[2] = {'a' + rand() % 26, '\0'};
        dllAppend(list, str);
    }

    dllPrint(list);

    ll sumb4 = sumChars(list);

    dllSort(list);

    ll sumaftr = sumChars(list);

    dllPrint(list);

    printf("Before: %lld, After: %lld \n", sumb4, sumaftr);
    if (isSorted(list)) {
        printf("Sorted\n");
    } else {
        printf("Womp womp\n");
    }

    dllDeleteDuplicatesSorted(list);

    dllPrint(list);

    dllFree(list);

    return 0;
}

bool isSorted(Dll *list) {
    DllNode *cur = list->head;
    while (cur != NULL && cur->next != NULL) {
        if (strcmp(cur->word, cur->next->word) > 0) {
            return false;
        }
        cur = cur->next;
    }
    return true;
}

ll sumChars(Dll *list) {
    DllNode *cur = list->head;
    ll sum = 0;
    while (cur != NULL) {
        for (int i = 0; cur->word[i] != '\0'; i++) {
            sum += (ll)(cur->word[i]);
        }
        cur = cur->next;
    }
    return sum;
}
