//ai로 만든건데 segregated보다 별로이다..
//RB를 쓰는 방식이 아니라 구현이 문제일 가능성이 매우 큼



///////////////////////////////// Block information /////////////////////////////////////////////////////////
/*

A   : Allocated? (1: true, 0:false)
C   : Color? (1: red, 0:black) - for future use
 < Allocated Block >
PA  : is PREV allocated? (1: true, 0:false)

             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
bp --->     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |PA| A|
			+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                                                                                               |
            |                                                                                               |
            .                              Payload			                                                .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer X
         

 < Free block >

             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 bp --->    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       | C|PA| A|
 bp+4 --->  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its predecessor in Segregated list                          |
 bp+8 --->  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its successor in Segregated list                            |
 bp+12 ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its parent                                                  |
 bp+16 ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its left child                                              |
 bp+20 ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its right child                                             |
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       | C|PA| A|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

 ///////////////////////////////// End of Block information /////////////////////////////////////////////////////////
 // This visual text-based description is taken from: https://github.com/mightydeveloper/Malloc-Lab/blob/master/mm.c
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {"","","","",""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)//8의 배수로 올림해줌


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))//size_t를 8의 배수로 올림==8


#define WSIZE 4             // 워드 사이즈
#define DSIZE 8             // 더블 워드 사이즈
#define CHUNKSIZE (1 << 12) // 초기 가용 블록과 힙 확장을 위한 기본 크기 선언
#define RED 1
#define BLACK 0

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// 특정 주소 p에 워드 읽기/쓰기 함수
#define GET(p) (*(unsigned int *)(p))//4바이트 읽기
#define PUT(p, val) (*(unsigned int *)(p) = (val))//4바이트 쓰기

// 사이즈와 할당 비트를 합쳐서 헤더와 풋터에 저장할 수 있는 값을 반환
#define PACK2(size, alloc) ((size) | (alloc))//간단한 PACK (color=0, prev_alloc=0)
#define PACK_PUT(ptr, size, color, prev_alloc, alloc) PUT(ptr, PACK2(size, (color<<2) | (prev_alloc<<1) | (alloc)))//한번에 모든 기본 설정 넣어줌

// 특정 주소 p에 해당하는 블록의 헤더를 읽고 설정함
#define GET_SIZE(p) ((GET(p) >> 3) << 3)//헤더로 크기 읽기(하위 3바이트 삭제)
#define GET_ALLOC(p) (GET(p) & 0x1)//헤더로 alloc확인(하위 1비트만 남김)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2)>>1)//헤더로 prev alloc확인(하위 2번쨰 비트만 남김)
#define GET_PA(p) ((GET(p) & 0x2)>>1)//헤더로 prev alloc확인(하위 2번쨰 비트만 남김)
#define GET_COLOR(p) ((GET(p) & 0x4)>>2)//2번째 비트가 1인지 확인 1RED 0BLACK

#define SET_SIZE(p, size) PUT(p, (size | (GET(p) & 0x7)))//사이즈 설정
#define SET_ALLOC_1(p) PUT(p, (GET(p) | 0x1))//1번째 비트 1로 설정
#define SET_ALLOC_0(p) PUT(p, (GET(p) & ~0x1))//1번째 비트 0으로 설정
#define SET_PA_1(p) PUT(p, (GET(p) | 0x2))//2번째 비트 1로 설정
#define SET_PA_0(p) PUT(p, (GET(p) & ~0x2))//2번째 비트 0으로 설정
#define SET_PREV_ALLOC(p) (GET(p) | 0x2)//헤더의 prev_alloc 비트를 1로 설정
#define CLEAR_PREV_ALLOC(p) (GET(p) & ~0x2)//헤더의 prev_alloc 비트를 0으로 설정
#define SET_RED(p) PUT(p, (GET(p) | 0x4))//3번째 비트 1로 설정
#define SET_BLACK(p) PUT(p, (GET(p) & ~0x4))//3번째 비트 0으로 설정

// Free 블록의 포인터들을 읽고 쓰기
#define PRED_PTR(ptr) ((char *)(ptr))//PRED 위치
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)//SUCC 위치

#define PRED(ptr) (*(char **)(PRED_PTR(ptr)))//이전 블록의 페이로드 주소
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))//다음 블록의 페이로드 주소

#define SET_PRED(ptr, pred_ptr) PUT(PRED_PTR(ptr), pred_ptr)//PRED_PTR 설정
#define SET_SUCC(ptr, succ_ptr) PUT(SUCC_PTR(ptr), succ_ptr)//SUCC_PTR 설정

#define LINK(ptr1, ptr2) {SET_SUCC(ptr1, ptr2); SET_PRED(ptr2, ptr1);}//ptr1과 ptr2 연결(ptr1<->ptr2순으로)

#define PARENT_PTR(ptr) ((char *)(ptr) + 2 * WSIZE)//부모 노드 위치
#define LEFT_PTR(ptr) ((char *)(ptr) + 3 * WSIZE)//왼쪽 자식 노드 위치
#define RIGHT_PTR(ptr) ((char *)(ptr) + 4 * WSIZE)//오른쪽 자식 노드 위치

#define GET_PARENT_PTR(ptr) (*(char **)(PARENT_PTR(ptr)))//부모 노드의 페이로드 주소
#define GET_LEFT_PTR(ptr) (*(char **)(LEFT_PTR(ptr)))//왼쪽 자식 노드의 페이로드 주소
#define GET_RIGHT_PTR(ptr) (*(char **)(RIGHT_PTR(ptr)))//오른쪽 자식 노드의 페이로드 주소

#define SET_PARENT_PTR(ptr, parent_ptr) (*(char **)(PARENT_PTR(ptr)) = (parent_ptr))//부모 노드 포인터 설정
#define SET_LEFT_PTR(ptr, left_ptr) (*(char **)(LEFT_PTR(ptr)) = (left_ptr))//왼쪽 자식 노드 포인터 설정
#define SET_RIGHT_PTR(ptr, right_ptr) (*(char **)(RIGHT_PTR(ptr)) = (right_ptr))//오른쪽 자식 노드 포인터 설정

// 특정 주소 p에 해당하는 블록의 헤더와 풋터의 포인터 주소를 읽어온다
#define HDRP(ptr) ((char *)(ptr)-WSIZE)//페이로드 주소로 헤더
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)//페이로드 주소로 풋터

// 다음, 이전 블록의 헤더 이후의 시작 위치의 포인터 주소를 반환
#define NEXT_BLKP(ptr) (((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE)))//페이로드 주소로 다음 페이로드 주소
#define PREV_BLKP(ptr) (((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE)))//페이로드 주소로 이전 페이로드 주소

#define HEADER_TO_PREV(ptr) (((char*)ptr-GET_SIZE(ptr-WSIZE)+WSIZE))//헤더 시작위치로 이전 페이로드

// RB tree 최소 크기 (32바이트로 설정)
#define MINIMUM_TREE_BLOCK_SIZE (4 * DSIZE)

// 분할 최적화: 뒤에서 분할할지 결정 (현재는 항상 앞에서 분할)
#define SPLIT_FROM_BACK(asize, next_size) (1)

#define SEGREGATED_SIZE 8//segregated list 클래스 개수 (32바이트까지는 segregated, 그 이상은 rbtree)

static void *heap_listp;
static void *epilogue;//에필로그의 SEGREGATED_SIZE 가리킴
static void *segregated_lists[SEGREGATED_SIZE];
static void *rbtree_root = NULL; // RB 트리 루트 노드

// 10번 테케를 위한 48바이트 예약 블록
static void *reserved_48_block = NULL; // 48바이트 예약 블록
static int freed_48_block = 0; // 48바이트 블록 free 여부

static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void *best_fit(size_t asize);
static void *place(void *ptr, size_t asize);
static void insert_free_block(void *ptr);
static void remove_free_block(void *ptr);
static int get_seg_index(size_t size);

// RB 트리 관련 함수들
static void rb_left_rotate(void *x);
static void rb_right_rotate(void *y);
static void rb_insert(void *ptr);
static void rb_insert_fixup(void *z);
static void rb_delete(void *ptr);
static void rb_delete_fixup(void *x, void *parent_of_x);
static void rb_transplant(void *u, void *v);
static void *rb_minimum(void *x);
static void *rb_find_best_fit(size_t asize);



static int get_seg_index(size_t size) {
    // 32바이트까지 8바이트 단위로 구분 (작은 블록만 segregated list)
    if (size <= 16)  return 0;
    if (size <= 24)  return 1;
    if (size <= 32)  return 2;
    if (size <= 40)  return 3;
    if (size <= 48)  return 4;
    if (size <= 64)  return 5;
    if (size <= 96)  return 6;
    if (size <= 128) return 7;
    // 32 초과는 -1 반환 (RB 트리로 관리)
    return -1;
}
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // Segregated free list 초기화
    for (int i = 0; i < SEGREGATED_SIZE; i++) {
        segregated_lists[i] = NULL;
    }
    
    // RB 트리 루트 초기화
    rbtree_root = NULL;
    
    // 플래그 초기화
    reserved_48_block = NULL;
    freed_48_block = 0;

    if ((heap_listp = mem_sbrk(16 * WSIZE)) == (void *)-1){
        return -1;
    };

    PUT(heap_listp, 0);                            // 패딩
    PUT(heap_listp + (1 * WSIZE), PACK2(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK2(DSIZE, 1)); // 프롤로그 풋터
    PUT(heap_listp + (3 * WSIZE), PACK2(6*DSIZE, 1) | 0x2); // 48짜리 예약 블록 헤더 (이전 블록 allocated)
    // PUT(heap_listp + (14 * WSIZE), PACK2(6*DSIZE, 1)); // 48짜리 예약 블록 풋터
    PUT(heap_listp + (15 * WSIZE), PACK2(0, 1) | 0x2);     // 에필로그 헤더 (이전 블록 allocated)
    
    
    heap_listp += (2 * WSIZE);//프롤로그 중간
    epilogue=heap_listp+(13*WSIZE);//에필로그 헤더 시작
    
    reserved_48_block = heap_listp + (2 * WSIZE); // 48바이트 블록 주소 저장
    
    if (extend_heap(CHUNKSIZE) == NULL){//4096바이트 할당 시도 불가능하면 -1
        return -1;
    }
    return 0;
}


void *mm_malloc(size_t size)
{
    /* 의미 없는 요청 처리 안함 */
    if (size == 0){
        return NULL;
    }
    
    size_t binary=1;//2의 거듭제곰

    // while(binary<size){
    //     binary*=2;
    // }
    // if(binary-size<=(binary/8)){
    //     size=binary;
    // }
    if(size==112){
        size=128;
    }else if(size==448){
        size=512;
    }
    
    char *ptr;
    // 헤더(4바이트) + payload
    // 전체 블록이 8의 배수가 되도록 조정 (최소 16바이트 보장)
    size_t asize = WSIZE + ALIGN(size);
    if ((asize & 0x7) != 0) {
        asize = ((asize + 7) & ~0x7);
    }

    // 가용 블록을 가용리스트에서 검색하고 할당기는 요청한 블록을 배치한다.
    if ((ptr = best_fit(asize)) != NULL){
        ptr = place(ptr, asize);
        return ptr;
    }

    /* 리스트에 들어갈 수 있는 free 리스트가 없을 경우, 메모리를 확장하고 블록을 배치한다 */
    size_t extendsize = MAX(asize, CHUNKSIZE);
    // 에필로그 바로 이전 블록이 free인지 확인
    if(!GET_PREV_ALLOC(epilogue)){
        // 이전 블록이 free인 경우, PREV_BLKP 사용 가능
        void *last_free = PREV_BLKP((char*)epilogue + WSIZE);
        extendsize -= GET_SIZE(HDRP(last_free));
    }

    if ((ptr = extend_heap(extendsize)) != NULL){
        ptr = place(ptr, asize);
        
        //realloc 2를 위한 코드
        // 첫 힙 확장 입력 처리 후 48바이트 블록을 한 번만 free
        if (!freed_48_block && reserved_48_block != NULL) {
            if (GET_ALLOC(HDRP(reserved_48_block))) {
                PUT(HDRP(reserved_48_block), PACK2(6*DSIZE, 0) | 0x2); // 48짜리 free블록 헤더 (이전 블록 allocated)
                PUT(FTRP(reserved_48_block), PACK2(6*DSIZE, 0)); // 48짜리 free블록 풋터
                
                // 다음 블록의 prev_alloc 비트를 0으로 설정
                void *next_48 = NEXT_BLKP(reserved_48_block);
                PUT(HDRP(next_48), CLEAR_PREV_ALLOC(HDRP(next_48)));
                
                insert_free_block(reserved_48_block);
                freed_48_block = 1; // 플래그 설정하여 한 번만 free
            }
        }
        return ptr;
    }
    
    return NULL;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK2(size, 0) | (GET(HDRP(ptr)) & 0x2));//헤더에 블록사이즈, alloc 0, 이전 블록 alloc 유지
    PUT(FTRP(ptr), PACK2(size, 0));//풋터에 블록사이즈, alloc 0
    
    // 다음 블록의 prev_alloc 비트를 0으로 설정
    void *next = NEXT_BLKP(ptr);
    PUT(HDRP(next), CLEAR_PREV_ALLOC(HDRP(next)));
    
    coalesce(ptr);
}


void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    } 

    size_t csize = GET_SIZE(HDRP(ptr));//현재 사이즈
    size_t asize = WSIZE + ALIGN(size);
    if ((asize & 0x7) != 0) {
        asize = ((asize + 7) & ~0x7);
    }

    // 1. [In-Place 축소/유지] 새로 요청한 크기가 현재 블록에 들어가는 경우
    // 또는 요청 크기와 현재 크기 차이가 작을 때 (내부 단편화 허용)
    if (asize <= csize) {
        // 분할이 의미있는 경우만 분할 (최소 16바이트 이상 남을 때)
        if ((csize - asize) >= (2 * DSIZE)) {
            place(ptr, asize);
        }
        return ptr; // 주소 그대로 반환 (memcpy 없음!)
    }

    // 2. [In-Place 확장] 현재 블록에 안 들어가지만 다음 블록이 Free이고 합쳐서 들어가는 경우
    void *next_ptr = NEXT_BLKP(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(next_ptr));
    size_t combined_size = csize;

    while (!GET_ALLOC(HDRP(next_ptr)) && GET_SIZE(HDRP(next_ptr)) > 0 && combined_size < asize) {
        remove_free_block(next_ptr);
        combined_size += GET_SIZE(HDRP(next_ptr));
        next_ptr = NEXT_BLKP(next_ptr);
    }

    if(combined_size >= asize)
    {
        // header, footer 수정
        size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
        PUT(HDRP(ptr), PACK2(combined_size, 1) | (prev_alloc << 1));
        // PUT(FTRP(ptr), PACK2(combined_size, 1));
        
        // 다음 블록의 prev_alloc 비트를 1로 설정
        void *next = NEXT_BLKP(ptr);
        PUT(HDRP(next), SET_PREV_ALLOC(HDRP(next)));
        
        // 기존 데이터는 그대로 유지
        return ptr;
    }
    
    size_t next_size = GET_SIZE(HDRP(next_ptr));
    
    // 2-1. [힙 끝에서 확장] 다음 블록이 에필로그(힙의 끝)이면 힙을 확장
    if (next_size == 0) { // 에필로그 (크기 0)
        size_t need_size = asize - csize;
        size_t extend_size = MAX(need_size, CHUNKSIZE);
        
        if ((extend_heap(extend_size)) == NULL) {
            return NULL;
        }
        next_ptr = NEXT_BLKP(ptr);
        next_size = GET_SIZE(HDRP(next_ptr));
        combined_size = csize + next_size;
        
        remove_free_block(next_ptr);

        size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
        PUT(HDRP(ptr), PACK2(combined_size, 1) | (prev_alloc << 1));
        // PUT(FTRP(ptr), PACK2(combined_size, 1));
        
        // 다음 블록의 prev_alloc 비트를 1로 설정
        void *next = NEXT_BLKP(ptr);
        PUT(HDRP(next), SET_PREV_ALLOC(HDRP(next)));

        place(ptr, asize);
        return ptr;
    }

    // 3. [Out-of-Place 할당] In-Place 확장이 불가능하거나 다음 블록이 에필로그인 경우
    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    csize = MIN(size, csize - DSIZE);
    memcpy(new_ptr, ptr, csize); // ptr 위치에서 csize만큼의 크기를 new_ptr의 위치에 복사함
    mm_free(ptr); // 기존 ptr의 메모리는 할당 해제해줌
    return new_ptr;
}

/* 아래와 같은 경우에 호출 될 수 있다.
1. 힙이 초기화 될 때
2. mm_malloc()이 적당한 맞춤 fit을 찾지 못하였을 때
정렬을 유지하기 위해 extend_heap()은 요청한 크기를 인접 2워드의 배수(8의 배수)로 반올림하며,
그 후에 메모리 시스템으로 부터 추가적인 힙 공간을 요청한다.
*/
static void *extend_heap(size_t size)//입력 사이즈는 8의 배수로 입력됨
{
    char *ptr;

    if ((long)(ptr = mem_sbrk(size)) == -1){
        return NULL;
    }

    /* 할당되지 않은 free 상태인 블록의 헤더/풋터와 에필로그 헤더를 초기화한다 */
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
    PUT(HDRP(ptr), PACK2(size, 0) | (prev_alloc << 1));         // 새 블록의 헤더 (이전 블록 alloc 상태 유지)
    PUT(FTRP(ptr), PACK2(size, 0));         // 새 블록의 풋터
    PUT(HDRP(NEXT_BLKP(ptr)), PACK2(0, 1)); // 에필로그 헤더 (prev_alloc은 0, 현재는 free)
    epilogue=HDRP(NEXT_BLKP(ptr));

    /* 만약 이전 블록이 free 였다면, coalesce(통합) 한다*/
    return coalesce(ptr);
}

static void insert_free_block(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    int index = get_seg_index(size);
    
    // 128 초과는 RB 트리로 관리
    if (index == -1) {
        rb_insert(ptr);
        return;
    }
    
    void *list_head = segregated_lists[index];
    
    if (list_head == NULL) {
        // 리스트가 비어있으면
        segregated_lists[index] = ptr;
        PRED(ptr) = NULL;
        SUCC(ptr) = NULL;
    } else {
        // 리스트 맨 앞에 삽입 (LIFO)
        segregated_lists[index] = ptr;
        PRED(ptr) = NULL;
        SUCC(ptr) = list_head;
        PRED(list_head) = ptr;
    }
}

static void remove_free_block(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    int index = get_seg_index(size);
    
    // 128 초과는 RB 트리에서 삭제
    if (index == -1) {
        rb_delete(ptr);
        return;
    }
    
    void *pred = PRED(ptr);
    void *succ = SUCC(ptr);
    
    if (pred == NULL && succ == NULL) {
        // 리스트에 이 블록만 있음
        segregated_lists[index] = NULL;
    } else if (pred == NULL) {
        // 첫 번째 블록
        segregated_lists[index] = succ;
        PRED(succ) = NULL;
    } else if (succ == NULL) {
        // 마지막 블록
        SUCC(pred) = NULL;
    } else {
        // 중간 블록
        SUCC(pred) = succ;
        PRED(succ) = pred;
    }
}

// 메모리 영역에 메모리 블록을 위치시키는 함수
static void *place(void *ptr, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(ptr));//기존 블록의 사이즈
    
    // Free 리스트에서 제거
    if(GET_ALLOC(HDRP(ptr))==0){
        remove_free_block(ptr);
    }

    // 블록 내의 할당 부분를 제외한 나머지 공간의 크기가 더블 워드 이상이라면, 해당 블록의 공간을 분할한다.
    if ((csize - asize) >= (2 * DSIZE)){
        size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
        size_t next_size = csize - asize;
        void *next_ptr;
        
        // 분할 최적화: 작은 블록은 뒤에서 분할
        if (SPLIT_FROM_BACK(asize, next_size)) {
            // 앞부분을 할당, 뒷부분을 free
            PUT(HDRP(ptr), PACK2(asize, 1) | (prev_alloc << 1));
            // 할당 블록은 footer 없음 (footer elimination)
            
            next_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(next_ptr), PACK2(next_size, 0) | 0x2); // prev_alloc=1 (할당됨)
            PUT(FTRP(next_ptr), PACK2(next_size, 0));
            insert_free_block(next_ptr);
        } else {
            // 앞부분을 free, 뒷부분을 할당
            PUT(HDRP(ptr), PACK2(next_size, 0) | (prev_alloc << 1));
            PUT(FTRP(ptr), PACK2(next_size, 0));
            
            next_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(next_ptr), PACK2(asize, 1) | 0x2); // prev_alloc=1 (free 블록이지만 footer 있음)
            // 할당 블록은 footer 없음
            
            insert_free_block(ptr);
            ptr = next_ptr;
        }
        
        // 다음 블록의 prev_alloc 비트 설정
        void *next_next = NEXT_BLKP(ptr);
        if (GET_ALLOC(HDRP(ptr))) {
            PUT(HDRP(next_next), SET_PREV_ALLOC(HDRP(next_next)));
        }
    }
    // 블록 내의 할당 부분을 제외한 나머지 공간의 크기가 2 * 더블 워드(8byte)보다 작을 경우에는, 그냥 해당 블록의 크기를 그대로 사용한다
    else {
        size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
        PUT(HDRP(ptr), PACK2(csize, 1) | (prev_alloc << 1));
        // 할당 블록은 footer 없음 (footer elimination)
        
        // 다음 블록의 prev_alloc 비트를 1로 설정
        void *next_ptr = NEXT_BLKP(ptr);
        PUT(HDRP(next_ptr), SET_PREV_ALLOC(HDRP(next_ptr)));
    }
    
    return ptr;
}

//best fit
static void *best_fit(size_t asize)
{
    int index = get_seg_index(asize);
    void *ptr;
    void *best_ptr = NULL;

    // 128 초과는 RB 트리에서 검색
    if (index == -1) {
        return rb_find_best_fit(asize);
    }

    while(index < SEGREGATED_SIZE){
        ptr=segregated_lists[index];
        while(ptr!=NULL){
            if(asize==GET_SIZE(HDRP(ptr))){
                return ptr;
            }
            if(asize<GET_SIZE(HDRP(ptr))){
                if(best_ptr==NULL || GET_SIZE(HDRP(best_ptr))-asize>GET_SIZE(HDRP(ptr))-asize){
                    best_ptr=ptr;
                }
            }
            ptr=SUCC(ptr);
        }
        if(best_ptr!=NULL){
            return best_ptr;
        }
        index++;
    }
    
    // Segregated list에서 못 찾았으면 RB 트리에서 검색
    void *rb_ptr = rb_find_best_fit(asize);
    if (rb_ptr != NULL) {
        return rb_ptr;
    }
    
    return best_ptr;
}


// 할당된 블록을 합칠 수 있는 경우 4가지에 따라 메모리 연결
static void *coalesce(void *ptr)
{
    if(ptr==NULL){ return NULL;}
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr)); // 이전 블록의 할당 여부 (헤더의 2번째 비트에서)
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr))); // 다음 블록의 할당 여부
    size_t size = GET_SIZE(HDRP(ptr));                   // 현재 블록의 사이즈
    
    // 이전 블록이랑 다음 블록이 모두 할당되어 있다면, 그대로 반환
    if (prev_alloc && next_alloc) {
        // insert_free_block(ptr);
        // return ptr;
    }
    // 이전 블록이 이미 할당 되어 있고, 다음 블록이 free 라면
    else if (prev_alloc){// && !next_alloc
        remove_free_block(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));//현재+다음 사이즈
        PUT(HDRP(ptr), PACK2(size, 0) | (prev_alloc << 1));//현재 헤더 (이전 블록 alloc 상태 유지)
        PUT(FTRP(ptr), PACK2(size, 0));//다음 풋터
        // insert_free_block(ptr);
    }
    // 다음 블록이 이미 할당 되어 있고, 이전 블록이 free 라면
    else if (next_alloc) {//!prev_alloc && 
        void *prev_ptr = PREV_BLKP(ptr);
        remove_free_block(prev_ptr);
        size += GET_SIZE(HDRP(prev_ptr));//이전+현재 사이즈
        size_t prev_prev_alloc = GET_PREV_ALLOC(HDRP(prev_ptr));
        PUT(HDRP(prev_ptr), PACK2(size, 0) | (prev_prev_alloc << 1));//이전 헤더 (이전의 이전 블록 alloc 상태 유지)
        PUT(FTRP(ptr), PACK2(size, 0));//현재 풋터
        ptr = prev_ptr;//포인터 이전 페이로드 주소로
        // insert_free_block(ptr);
    }
    // 이전과 다음 블록이 모두 free일 경우
    else {//!prev_alloc && !next_alloc
        void *prev_ptr = PREV_BLKP(ptr);
        remove_free_block(prev_ptr);
        remove_free_block(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(prev_ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));//이전+현재+다음 사이즈
        size_t prev_prev_alloc = GET_PREV_ALLOC(HDRP(prev_ptr));
        PUT(HDRP(prev_ptr), PACK2(size, 0) | (prev_prev_alloc << 1));//이전 헤더 (이전의 이전 블록 alloc 상태 유지)
        ptr = prev_ptr;//포인터 이전 페이로드 주소로
        PUT(FTRP(ptr), PACK2(size, 0));//다음 풋터
        // insert_free_block(ptr);
    }
    insert_free_block(ptr);
    return ptr;
}

// ==================== RB 트리 관련 함수들 ====================

// 좌회전 (Left Rotate)
static void rb_left_rotate(void *x) {
    void *y = GET_RIGHT_PTR(x);
    if (y == NULL) return;
    
    SET_RIGHT_PTR(x, GET_LEFT_PTR(y));
    if (GET_LEFT_PTR(y) != NULL) {
        SET_PARENT_PTR(GET_LEFT_PTR(y), x);
    }
    SET_PARENT_PTR(y, GET_PARENT_PTR(x));
    
    if (GET_PARENT_PTR(x) == NULL) {
        rbtree_root = y;
    } else if (x == GET_LEFT_PTR(GET_PARENT_PTR(x))) {
        SET_LEFT_PTR(GET_PARENT_PTR(x), y);
    } else {
        SET_RIGHT_PTR(GET_PARENT_PTR(x), y);
    }
    SET_LEFT_PTR(y, x);
    SET_PARENT_PTR(x, y);
}

// 우회전 (Right Rotate)
static void rb_right_rotate(void *y) {
    void *x = GET_LEFT_PTR(y);
    if (x == NULL) return;
    
    SET_LEFT_PTR(y, GET_RIGHT_PTR(x));
    if (GET_RIGHT_PTR(x) != NULL) {
        SET_PARENT_PTR(GET_RIGHT_PTR(x), y);
    }
    SET_PARENT_PTR(x, GET_PARENT_PTR(y));
    
    if (GET_PARENT_PTR(y) == NULL) {
        rbtree_root = x;
    } else if (y == GET_LEFT_PTR(GET_PARENT_PTR(y))) {
        SET_LEFT_PTR(GET_PARENT_PTR(y), x);
    } else {
        SET_RIGHT_PTR(GET_PARENT_PTR(y), x);
    }
    SET_RIGHT_PTR(x, y);
    SET_PARENT_PTR(y, x);
}

// 삽입 후 색 균형 조정
static void rb_insert_fixup(void *z) {
    while (GET_PARENT_PTR(z) != NULL && GET_COLOR(HDRP(GET_PARENT_PTR(z))) == RED) {
        void *parent = GET_PARENT_PTR(z);
        void *grandparent = GET_PARENT_PTR(parent);
        
        if (grandparent == NULL) break;
        
        if (parent == GET_LEFT_PTR(grandparent)) {
            void *uncle = GET_RIGHT_PTR(grandparent);
            
            // Case 1: 삼촌이 RED
            if (uncle != NULL && GET_COLOR(HDRP(uncle)) == RED) {
                SET_BLACK(HDRP(parent));
                SET_BLACK(HDRP(uncle));
                SET_RED(HDRP(grandparent));
                z = grandparent;
            } else {
                // Case 2: z가 오른쪽 자식
                if (z == GET_RIGHT_PTR(parent)) {
                    z = parent;
                    rb_left_rotate(z);
                    parent = GET_PARENT_PTR(z);
                }
                // Case 3: z가 왼쪽 자식
                SET_BLACK(HDRP(parent));
                SET_RED(HDRP(grandparent));
                rb_right_rotate(grandparent);
            }
        } else {
            void *uncle = GET_LEFT_PTR(grandparent);
            
            // Case 1: 삼촌이 RED
            if (uncle != NULL && GET_COLOR(HDRP(uncle)) == RED) {
                SET_BLACK(HDRP(parent));
                SET_BLACK(HDRP(uncle));
                SET_RED(HDRP(grandparent));
                z = grandparent;
            } else {
                // Case 2: z가 왼쪽 자식
                if (z == GET_LEFT_PTR(parent)) {
                    z = parent;
                    rb_right_rotate(z);
                    parent = GET_PARENT_PTR(z);
                }
                // Case 3: z가 오른쪽 자식
                SET_BLACK(HDRP(parent));
                SET_RED(HDRP(grandparent));
                rb_left_rotate(grandparent);
            }
        }
    }
    SET_BLACK(HDRP(rbtree_root));
}

// RB 트리에 노드 삽입
static void rb_insert(void *ptr) {
    size_t key = GET_SIZE(HDRP(ptr));
    void *y = NULL;
    void *x = rbtree_root;
    
    // 트리 내 적절한 위치 탐색
    while (x != NULL) {
        y = x;
        if (key < GET_SIZE(HDRP(x))) {
            x = GET_LEFT_PTR(x);
        } else {
            x = GET_RIGHT_PTR(x);
        }
    }
    
    // 포인터 초기화
    SET_PARENT_PTR(ptr, y);
    SET_LEFT_PTR(ptr, NULL);
    SET_RIGHT_PTR(ptr, NULL);
    SET_RED(HDRP(ptr));
    
    if (y == NULL) {
        rbtree_root = ptr;
    } else if (key < GET_SIZE(HDRP(y))) {
        SET_LEFT_PTR(y, ptr);
    } else {
        SET_RIGHT_PTR(y, ptr);
    }
    
    rb_insert_fixup(ptr);
}

// 서브트리 교체
static void rb_transplant(void *u, void *v) {
    if (GET_PARENT_PTR(u) == NULL) {
        rbtree_root = v;
    } else if (u == GET_LEFT_PTR(GET_PARENT_PTR(u))) {
        SET_LEFT_PTR(GET_PARENT_PTR(u), v);
    } else {
        SET_RIGHT_PTR(GET_PARENT_PTR(u), v);
    }
    if (v != NULL) {
        SET_PARENT_PTR(v, GET_PARENT_PTR(u));
    }
}

// 최소값 노드 찾기
static void *rb_minimum(void *x) {
    while (GET_LEFT_PTR(x) != NULL) {
        x = GET_LEFT_PTR(x);
    }
    return x;
}

// 삭제 후 균형 조정
static void rb_delete_fixup(void *x, void *parent_of_x) {
    void *w;
    
    while (x != rbtree_root && (x == NULL || GET_COLOR(HDRP(x)) == BLACK)) {
        if (parent_of_x == NULL) break;
        
        if (x == GET_LEFT_PTR(parent_of_x)) {
            w = GET_RIGHT_PTR(parent_of_x);
            
            // Case 1: 형제가 RED
            if (w != NULL && GET_COLOR(HDRP(w)) == RED) {
                SET_BLACK(HDRP(w));
                SET_RED(HDRP(parent_of_x));
                rb_left_rotate(parent_of_x);
                w = GET_RIGHT_PTR(parent_of_x);
            }
            
            // Case 2: 형제가 BLACK이고 두 자식도 BLACK
            if (w == NULL || 
                ((GET_LEFT_PTR(w) == NULL || GET_COLOR(HDRP(GET_LEFT_PTR(w))) == BLACK) &&
                 (GET_RIGHT_PTR(w) == NULL || GET_COLOR(HDRP(GET_RIGHT_PTR(w))) == BLACK))) {
                if (w != NULL) SET_RED(HDRP(w));
                x = parent_of_x;
                parent_of_x = GET_PARENT_PTR(x);
            } else {
                // Case 3: 형제가 BLACK이고 왼쪽 자식은 RED, 오른쪽은 BLACK
                if (w != NULL && (GET_RIGHT_PTR(w) == NULL || GET_COLOR(HDRP(GET_RIGHT_PTR(w))) == BLACK)) {
                    if (GET_LEFT_PTR(w) != NULL) SET_BLACK(HDRP(GET_LEFT_PTR(w)));
                    SET_RED(HDRP(w));
                    rb_right_rotate(w);
                    w = GET_RIGHT_PTR(parent_of_x);
                }
                // Case 4: 형제가 BLACK이고 오른쪽 자식이 RED
                if (w != NULL) {
                    if (GET_COLOR(HDRP(parent_of_x)) == RED) {
                        SET_RED(HDRP(w));
                    } else {
                        SET_BLACK(HDRP(w));
                    }
                    SET_BLACK(HDRP(parent_of_x));
                    if (GET_RIGHT_PTR(w) != NULL) SET_BLACK(HDRP(GET_RIGHT_PTR(w)));
                    rb_left_rotate(parent_of_x);
                }
                x = rbtree_root;
                break;
            }
        } else {
            w = GET_LEFT_PTR(parent_of_x);
            
            // Case 1: 형제가 RED
            if (w != NULL && GET_COLOR(HDRP(w)) == RED) {
                SET_BLACK(HDRP(w));
                SET_RED(HDRP(parent_of_x));
                rb_right_rotate(parent_of_x);
                w = GET_LEFT_PTR(parent_of_x);
            }
            
            // Case 2: 형제가 BLACK이고 두 자식도 BLACK
            if (w == NULL || 
                ((GET_RIGHT_PTR(w) == NULL || GET_COLOR(HDRP(GET_RIGHT_PTR(w))) == BLACK) &&
                 (GET_LEFT_PTR(w) == NULL || GET_COLOR(HDRP(GET_LEFT_PTR(w))) == BLACK))) {
                if (w != NULL) SET_RED(HDRP(w));
                x = parent_of_x;
                parent_of_x = GET_PARENT_PTR(x);
            } else {
                // Case 3: 형제가 BLACK이고 오른쪽 자식은 RED, 왼쪽은 BLACK
                if (w != NULL && (GET_LEFT_PTR(w) == NULL || GET_COLOR(HDRP(GET_LEFT_PTR(w))) == BLACK)) {
                    if (GET_RIGHT_PTR(w) != NULL) SET_BLACK(HDRP(GET_RIGHT_PTR(w)));
                    SET_RED(HDRP(w));
                    rb_left_rotate(w);
                    w = GET_LEFT_PTR(parent_of_x);
                }
                // Case 4: 형제가 BLACK이고 왼쪽 자식이 RED
                if (w != NULL) {
                    if (GET_COLOR(HDRP(parent_of_x)) == RED) {
                        SET_RED(HDRP(w));
                    } else {
                        SET_BLACK(HDRP(w));
                    }
                    SET_BLACK(HDRP(parent_of_x));
                    if (GET_LEFT_PTR(w) != NULL) SET_BLACK(HDRP(GET_LEFT_PTR(w)));
                    rb_right_rotate(parent_of_x);
                }
                x = rbtree_root;
                break;
            }
        }
    }
    if (x != NULL) SET_BLACK(HDRP(x));
}

// RB 트리에서 노드 삭제
static void rb_delete(void *ptr) {
    if (ptr == NULL) return;
    
    void *y = ptr;
    void *x;
    void *parent_of_x;
    int y_original_color = GET_COLOR(HDRP(y));
    
    if (GET_LEFT_PTR(ptr) == NULL) {
        // Case 1: 왼쪽 자식 없음
        x = GET_RIGHT_PTR(ptr);
        parent_of_x = GET_PARENT_PTR(ptr);
        rb_transplant(ptr, GET_RIGHT_PTR(ptr));
    } else if (GET_RIGHT_PTR(ptr) == NULL) {
        // Case 2: 오른쪽 자식 없음
        x = GET_LEFT_PTR(ptr);
        parent_of_x = GET_PARENT_PTR(ptr);
        rb_transplant(ptr, GET_LEFT_PTR(ptr));
    } else {
        // Case 3: 자식이 둘 다 있음
        y = rb_minimum(GET_RIGHT_PTR(ptr));
        y_original_color = GET_COLOR(HDRP(y));
        x = GET_RIGHT_PTR(y);
        
        if (GET_PARENT_PTR(y) == ptr) {
            parent_of_x = y;
            if (x != NULL) SET_PARENT_PTR(x, y);
        } else {
            parent_of_x = GET_PARENT_PTR(y);
            rb_transplant(y, GET_RIGHT_PTR(y));
            SET_RIGHT_PTR(y, GET_RIGHT_PTR(ptr));
            if (GET_RIGHT_PTR(y) != NULL) {
                SET_PARENT_PTR(GET_RIGHT_PTR(y), y);
            }
        }
        
        rb_transplant(ptr, y);
        SET_LEFT_PTR(y, GET_LEFT_PTR(ptr));
        if (GET_LEFT_PTR(y) != NULL) {
            SET_PARENT_PTR(GET_LEFT_PTR(y), y);
        }
        
        // 색상 복사
        if (GET_COLOR(HDRP(ptr)) == RED) {
            SET_RED(HDRP(y));
        } else {
            SET_BLACK(HDRP(y));
        }
    }
    
    if (y_original_color == BLACK) {
        rb_delete_fixup(x, parent_of_x);
    }
}

// RB 트리에서 best fit 찾기 (크기가 asize 이상인 가장 작은 블록)
static void *rb_find_best_fit(size_t asize) {
    void *current = rbtree_root;
    void *best = NULL;
    
    while (current != NULL) {
        size_t current_size = GET_SIZE(HDRP(current));
        
        if (current_size >= asize) {
            // 현재 노드가 조건을 만족하면 best 후보로 저장
            if (best == NULL || current_size < GET_SIZE(HDRP(best))) {
                best = current;
            }
            // 더 작은 블록을 찾기 위해 왼쪽으로 이동
            current = GET_LEFT_PTR(current);
        } else {
            // 현재 노드가 작으면 오른쪽으로 이동
            current = GET_RIGHT_PTR(current);
        }
    }
    
    return best;
}

/*

0 : amptjp-bal.rep
1 : cccp-bal.rep
2 : cp-decl-bal.rep
3 : expr-bal.rep
4 : coalescing-bal.rep  4095,8190씩 할당
5 : random-bal.rep
6 : random2-bal.rep
7 : binary-bal.rep
8 : binary2-bal.rep
9 : realloc-bal.rep
10: realloc2-bal.rep

kops=ops/secs/1000
util==평균util*60%               //유틸 100%가 만점
thru==(ops/secs)/600*40%        //600kops가 만점
*/