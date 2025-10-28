#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define WSIZE 4             // 워드 사이즈
#define DSIZE 8             // 더블 워드 사이즈
#define GET_SIZE(p) ((GET(p) >> 1) << 1)//헤더로 크기 읽기(하위 1바이트 삭제)


//rbtree 관련 매크로
// #define SET_RED(p) PUT(p, (GET(p) | 0x2))//2번째 비트를 1로 설정
// #define SET_BLACK(p) PUT(p, (GET(p) & ~0x2))//2번째 비트를 0으로 설정
#define GET_COLOR(p) (GET(p) & 0x2)//2번째 비트가 1인지 확인 1RED 0BLACK
#define SET_COLOR(p,n) PUT(p, (GET(p) | n << 1))//2번째 비트 설정
// #define IS_RED(p) (GET(p) & 0x2)//2번째 비트가 1인지 확인
// #define IS_BLACK(p) !(GET(p) & 0x2)//2번째 비트가 0인지 확인

#define GET_PARENT_PTR(ptr) (*(char **)(ptr + 2 * WSIZE))//부모 노드 포인터
#define GET_LEFT_PTR(ptr) (*(char **)(ptr + 3 * WSIZE))//왼쪽 자식 노드 포인터
#define GET_RIGHT_PTR(ptr) (*(char **)(ptr + 4 * WSIZE))//오른쪽 자식 노드 포인터

#define SET_PARENT_PTR(ptr, parent_ptr) (*(char **)(ptr + 2 * WSIZE) = (parent_ptr))//부모 노드 포인터 설정
#define SET_LEFT_PTR(ptr, left_ptr) (*(char **)(ptr + 3 * WSIZE) = (left_ptr))//왼쪽 자식 노드 포인터 설정
#define SET_RIGHT_PTR(ptr, right_ptr) (*(char **)(ptr + 4 * WSIZE) = (right_ptr))//오른쪽 자식 노드 포인터 설정


// NIL 노드를 사용하지 않는 레드-블랙 트리 (C 언어 버전)
// NULL 포인터로 자식이 없는 경우를 표현함.

typedef enum { RED, BLACK } Color;

void *root = NULL;

typedef struct {
    void *root;
} RBTree;

// 함수 프로토타입
void insert(int key);
void insertFixup(void *z);
void leftRotate(void *x);
void rightRotate(void *y);
// void inorder(void *node);
void transplant(void *u, void *v);
void *findNode(void *root, int key);
// void *minimum(void *x);
void deleteNode(int key);
void deleteFixup(void *x, void *parentOfX);

// --- 구현부 ---

// 좌회전 (Left Rotate)
void leftRotate(void *x) {
    void *y = GET_RIGHT_PTR(x);
    SET_RIGHT_PTR(x, GET_LEFT_PTR(y));
    if (GET_LEFT_PTR(y)) SET_PARENT_PTR(GET_LEFT_PTR(y), x);
    SET_PARENT_PTR(y, GET_PARENT_PTR(x));
    if (!GET_PARENT_PTR(x)) root = y;
    else if (x == GET_LEFT_PTR(GET_PARENT_PTR(x))) SET_LEFT_PTR(GET_PARENT_PTR(x), y);
    else SET_RIGHT_PTR(GET_PARENT_PTR(x), y);
    SET_LEFT_PTR(y, x);
    SET_PARENT_PTR(x, y);
}

// 우회전 (Right Rotate)
void rightRotate(void *y) {
    void *x = GET_LEFT_PTR(y);
    SET_LEFT_PTR(y, GET_RIGHT_PTR(x));
    if (GET_RIGHT_PTR(x)) SET_PARENT_PTR(GET_RIGHT_PTR(x), y);
    SET_PARENT_PTR(x, GET_PARENT_PTR(y));
    
    if (!GET_PARENT_PTR(y)) root = x;
    else if (y == GET_LEFT_PTR(GET_PARENT_PTR(y))) SET_LEFT_PTR(GET_PARENT_PTR(y), x);
    else SET_RIGHT_PTR(GET_PARENT_PTR(y), x);
    SET_RIGHT_PTR(x, y);
    SET_PARENT_PTR(y, x);
}

// 노드 삽입
void insert(int key) {
    void *z = createNode(key, RED, NULL);
    void *y = NULL;
    void *x = root;

    // 트리 내 적절한 위치 탐색
    while (x) {
        y = x;
        if (key < GET_SIZE(x)) x = GET_LEFT_PTR(x);
        else x = GET_RIGHT_PTR(x);
    }

    SET_PARENT_PTR(z, y);
    if (!y) root = z;
    else if (key <GET_SIZE(y)) SET_LEFT_PTR(y, z);
    else SET_RIGHT_PTR(y, z);

    insertFixup(z);
}

// 삽입 후 색 균형 조정
void insertFixup(void *z) {
    while (GET_PARENT_PTR(z) && GET_COLOR(GET_PARENT_PTR(z))) {
        if (GET_PARENT_PTR(z) == GET_LEFT_PTR(GET_PARENT_PTR(GET_PARENT_PTR(z)))) {
            void *y = GET_RIGHT_PTR(GET_PARENT_PTR(GET_PARENT_PTR(z))); // 삼촌 노드
            if (y && GET_COLOR(y)) {
                // case 1: 삼촌이 RED인 경우
                SET_COLOR(GET_PARENT_PTR(z), 0);
                SET_COLOR(y, 0);
                SET_COLOR(GET_PARENT_PTR(GET_PARENT_PTR(z)), 1);
                z = GET_PARENT_PTR(GET_PARENT_PTR(z));
            } else {
                if (z == GET_RIGHT_PTR(GET_PARENT_PTR(z))) {
                    // case 2: z가 오른쪽 자식인 경우
                    z = GET_PARENT_PTR(z);
                    leftRotate(z);
                }
                // case 3: z가 왼쪽 자식인 경우
                SET_COLOR(GET_PARENT_PTR(z), 0);
                SET_COLOR(GET_PARENT_PTR(GET_PARENT_PTR(z)), 1);
                rightRotate(GET_PARENT_PTR(GET_PARENT_PTR(z)));
            }
        } else {
            void *y = GET_LEFT_PTR(GET_PARENT_PTR(GET_PARENT_PTR(z))); // 삼촌 노드
            if (y && GET_COLOR(y)) {
                // case 1: 삼촌이 RED인 경우
                SET_COLOR(GET_PARENT_PTR(z), 0);
                SET_COLOR(y, 0);
                SET_COLOR(GET_PARENT_PTR(GET_PARENT_PTR(z)), 1);
                z = GET_PARENT_PTR(GET_PARENT_PTR(z));
            } else {
                if (z == GET_LEFT_PTR(GET_PARENT_PTR(z))) {
                    // case 2: z가 왼쪽 자식인 경우
                    z = GET_PARENT_PTR(z);
                    rightRotate(z);
                }
                // case 3: z가 오른쪽 자식인 경우
                SET_COLOR(GET_PARENT_PTR(z), 0);
                SET_COLOR(GET_PARENT_PTR(GET_PARENT_PTR(z)), 1);
                leftRotate(GET_PARENT_PTR(GET_PARENT_PTR(z)));
            }
        }
    }
    SET_COLOR(root, 0); // 루트는 항상 BLACK
}

// 서브트리 교체 (u 자리를 v로 대체)
void transplant(void *u, void *v) {
    if (!GET_PARENT_PTR(u)) root = v;
    else if (u == GET_LEFT_PTR(GET_PARENT_PTR(u))) SET_LEFT_PTR(GET_PARENT_PTR(u), v);
    else SET_RIGHT_PTR(GET_PARENT_PTR(u), v);
    if (v) SET_PARENT_PTR(v, GET_PARENT_PTR(u));
}

// 최소값 노드 찾기
void *minimum(void *x) {
    while (GET_LEFT_PTR(x)) x = GET_LEFT_PTR(x);
    return x;
}

// 키로 노드 검색
void *findNode(void *root, int key) {
    while (root) {
        if (key == GET_SIZE(root)) return root;
        else if (key < GET_SIZE(root)) root = GET_LEFT_PTR(root);
        else root = GET_RIGHT_PTR(root);
    }
    return NULL;
}

// 노드 삭제
void deleteNode(int key) {
    void *z = findNode(root, key);
    if (!z) return;

    void *y = z;
    Color yOriginalColor = GET_COLOR(y);
    void *x;

    if (!GET_LEFT_PTR(z)) {
        // case 1: 왼쪽 자식이 없는 경우
        x = GET_RIGHT_PTR(z);
        transplant(z, GET_RIGHT_PTR(z));
    } else if (!GET_RIGHT_PTR(z)) {
        // case 2: 오른쪽 자식이 없는 경우
        x = GET_LEFT_PTR(z);
        transplant(z, GET_LEFT_PTR(z));
    } else {
        // case 3: 자식이 둘 다 있는 경우
        y = minimum(GET_RIGHT_PTR(z));
        yOriginalColor = GET_COLOR(y);
        x = GET_RIGHT_PTR(y);
        if (GET_PARENT_PTR(y) == z) {
            if (x) SET_PARENT_PTR(x, y);
        } else {
            transplant(y, GET_RIGHT_PTR(y));
            SET_RIGHT_PTR(y, GET_RIGHT_PTR(z));
            if (GET_RIGHT_PTR(y)) SET_PARENT_PTR(GET_RIGHT_PTR(y), y);
        }
        transplant(z, y);
        SET_LEFT_PTR(y, GET_LEFT_PTR(z));
        if (GET_LEFT_PTR(y)) SET_PARENT_PTR(GET_LEFT_PTR(y), y);
        SET_COLOR(y,GET_COLOR(z));
    }

    if (yOriginalColor == BLACK)
        deleteFixup(x, GET_PARENT_PTR(y));

    free(z);
}

// 삭제 후 균형 조정
void deleteFixup(void *x, void *parentOfX) {
    void *w;
    while ((x != root) && (!x || !GET_COLOR(x))) {
        if (!parentOfX) break;

        if (x == GET_LEFT_PTR(parentOfX)) {
            w = GET_RIGHT_PTR(parentOfX);
            if (w && GET_COLOR(w)) {
                // case 1: 형제가 RED인 경우
                SET_COLOR(w, 0);
                SET_COLOR(parentOfX, 1);
                leftRotate(parentOfX);
                w = GET_RIGHT_PTR(parentOfX);
            }
            if ((!w) || ((!GET_LEFT_PTR(w) || !GET_COLOR(GET_LEFT_PTR(w))) && (!GET_RIGHT_PTR(w) || !GET_COLOR(GET_RIGHT_PTR(w))))) {
                // case 2: 형제가 BLACK이고 두 자식도 BLACK인 경우
                if (w) SET_COLOR(w, 1);
                x = parentOfX;
                parentOfX = GET_PARENT_PTR(x);
            } else {
                if (!GET_RIGHT_PTR(w) || !GET_COLOR(GET_RIGHT_PTR(w))) {
                    // case 3: 형제가 BLACK, 왼쪽 자식이 RED, 오른쪽 자식이 BLACK인 경우
                    if (GET_LEFT_PTR(w)) SET_COLOR(GET_LEFT_PTR(w), 0);
                    SET_COLOR(w, 1);
                    rightRotate(w);
                    w = GET_RIGHT_PTR(parentOfX);
                }
                // case 4: 형제가 BLACK이고 오른쪽 자식이 RED인 경우
                if (w) SET_COLOR(w,GET_COLOR(parentOfX));
                SET_COLOR(parentOfX, 0);
                if (w && GET_RIGHT_PTR(w)) SET_COLOR(GET_RIGHT_PTR(w), 0);
                leftRotate(parentOfX);
                x = root;
                break;
            }
        } else {
            w = GET_LEFT_PTR(parentOfX);
            if (w && GET_COLOR(w)) {
                // case 1: 형제가 RED인 경우
                SET_COLOR(w, 0);
                SET_COLOR(parentOfX, 1);
                rightRotate(parentOfX);
                w = GET_LEFT_PTR(parentOfX);
            }
            if ((!w) || ((!GET_LEFT_PTR(w) || !GET_COLOR(GET_LEFT_PTR(w))) && (!GET_RIGHT_PTR(w) || !GET_COLOR(GET_RIGHT_PTR(w))))) {
                // case 2: 형제가 BLACK이고 두 자식도 BLACK인 경우
                if (w) SET_COLOR(w, 1);
                x = parentOfX;
                parentOfX = GET_PARENT_PTR(x);
            } else {
                if (!GET_LEFT_PTR(w) || !GET_COLOR(GET_LEFT_PTR(w))) {
                    // case 3: 형제가 BLACK, 오른쪽 자식이 RED, 왼쪽 자식이 BLACK인 경우
                    if (GET_RIGHT_PTR(w)) SET_COLOR(GET_RIGHT_PTR(w), 0);
                    SET_COLOR(w, 1);
                    leftRotate(w);
                    w = GET_LEFT_PTR(parentOfX);
                }
                // case 4: 형제가 BLACK이고 왼쪽 자식이 RED인 경우
                if (w) SET_COLOR(w,GET_COLOR(parentOfX));
                SET_COLOR(parentOfX, 0);
                if (w && GET_LEFT_PTR(w)) SET_COLOR(GET_LEFT_PTR(w), 0);
                rightRotate(parentOfX);
                x = root;
                break;
            }
        }
    }
    if (x) SET_COLOR(x, 0);
}

// 중위 순회 (inorder traversal)
void inorder(void *node) {
    if (!node) return;
    inorder(GET_LEFT_PTR(node));
    printf("%d ", GET_SIZE(node));
    inorder(GET_RIGHT_PTR(node));
}
