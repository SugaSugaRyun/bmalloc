#include <stdio.h>
#include <stdlib.h>
#include "bmalloc.h"

int main() {
    int numArrays;
    int* arraySizes;
    int** arrays;

    printf("할당할 배열의 개수를 입력하세요: ");
    scanf("%d", &numArrays);

    arraySizes = (int*)bmalloc(numArrays * sizeof(int));  // 배열 크기를 저장할 메모리 할당
    arrays = (int**)bmalloc(numArrays * sizeof(int*));    // 배열 포인터를 저장할 메모리 할당

    printf("각 배열의 크기를 입력하세요:\n");
    for (int i = 0; i < numArrays; i++) {
        scanf("%d", &arraySizes[i]);
        arrays[i] = (int*)bmalloc(arraySizes[i] * sizeof(int));  // 각 배열 크기만큼 메모리 할당
        if (arrays[i] == NULL) {
            printf("메모리 할당 오류\n");
            return 1;
        }
    }

    printf("각 배열에 값을 입력하세요:\n");
    for (int i = 0; i < numArrays; i++) {
        printf("배열 %d:\n", i + 1);
        for (int j = 0; j < arraySizes[i]; j++) {
            scanf("%d", &arrays[i][j]);
        }
    }

    printf("할당된 배열들의 값은 다음과 같습니다:\n");
    for (int i = 0; i < numArrays; i++) {
        printf("배열 %d: ", i + 1);
        for (int j = 0; j < arraySizes[i]; j++) {
            printf("%d ", arrays[i][j]);
        }
        printf("\n");
    }

    for (int i = 0; i < numArrays; i++) {
        bfree(arrays[i]);  // 각 배열의 메모리 해제
    }
    bfree(arraySizes);  // 배열 크기를 저장한 메모리 해제
    bfree(arrays);      // 배열 포인터를 저장한 메모리 해제

    return 0;
}
