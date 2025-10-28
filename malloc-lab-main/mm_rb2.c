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
#define PACK(size, color, prev_alloc, alloc) ((size) | (color<<2) | (prev_alloc<<1) | (alloc))//한번에 모든 기본 설정
#define PACK_PUT(ptr, size, color, prev_alloc, alloc) PUT(ptr, PACK(size, color, prev_alloc, alloc))//한번에 모든 기본 설정 넣어줌

// 특정 주소 p에 해당하는 블록의 헤더를 읽고 설정함
#define GET_SIZE(p) ((GET(p) >> 3) << 3)//헤더로 크기 읽기(하위 3바이트 삭제)
#define GET_ALLOC(p) (GET(p) & 0x1)//헤더로 alloc확인(하위 1비트만 남김)
#define GET_PA(p) ((GET(p) & 0x2)>>1)//헤더로 prev alloc확인(하위 2번쨰 비트만 남김)
#define GET_COLOR(p) ((GET(p) & 0x4)>>2)//2번째 비트가 1인지 확인 1RED 0BLACK

#define SET_SIZE(p, size) PUT(p, (size | (GET(p) & 0x7)))//사이즈 설정
#define SET_ALLOC_1(p) PUT(p, (GET(p) | 0x1))//1번째 비트 1로 설정
#define SET_ALLOC_0(p) PUT(p, (GET(p) & ~0x1))//1번째 비트 0으로 설정
#define SET_PA_1(p) PUT(p, (GET(p) | 0x2))//2번째 비트 1로 설정
#define SET_PA_0(p) PUT(p, (GET(p) & ~0x2))//2번째 비트 0으로 설정
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

#define PARENT(ptr) (*(char **)(PARENT_PTR))//부모 노드의 페이로드 주소
#define LEFT(ptr) (*(char **)(LEFT_PTR))//왼쪽 자식 노드의 페이로드 주소
#define RIGHT(ptr) (*(char **)(RIGHT_PTR))//오른쪽 자식 노드의 페이로드 주소

#define SET_PARENT(ptr, parent_ptr) PUT(PARENT_PTR(ptr), parent_ptr)//부모 노드 포인터 설정
#define SET_LEFT(ptr, left_ptr) PUT(LEFT_PTR(ptr), left_ptr)//왼쪽 자식 노드 포인터 설정
#define SET_RIGHT(ptr, right_ptr) PUT(RIGHT_PTR(ptr), right_ptr)//오른쪽 자식 노드 포인터 설정

// 특정 주소 p에 해당하는 블록의 헤더와 풋터의 포인터 주소를 읽어온다
#define HDRP(ptr) ((char *)(ptr)-WSIZE)//페이로드 주소로 헤더
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)//페이로드 주소로 풋터

// 다음, 이전 블록의 헤더 이후의 시작 위치의 포인터 주소를 반환
#define NEXT_BLKP(ptr) (((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE)))//페이로드 주소로 다음 페이로드 주소
#define PREV_BLKP(ptr) (((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE)))//페이로드 주소로 이전 페이로드 주소

#define HEADER_TO_PREV(ptr) (((char*)ptr-GET_SIZE(ptr-WSIZE)+WSIZE))//헤더 시작위치로 이전 페이로드

// 전역 변수 및 함수 선언
#define SEGREGATED_SIZE 14//segregated list 클래스 개수

static void *heap_listp;
static void *epilogue;//에필로그의 SEGREGATED_SIZE 가리킴
static void *segregated_lists[SEGREGATED_SIZE];

// 10번 테케를 위한 48바이트 예약 블록
static void *reserved_48_block = NULL; // 48바이트 예약 블록
static int freed_48_block = 0; // 48바이트 블록 free 여부

static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void *best_fit(size_t asize);
static void place(void *ptr, size_t asize);
static void insert_free_block(void *ptr);
static void remove_free_block(void *ptr);
static int get_seg_index(size_t size);

static int get_seg_index(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 48) return 2;
    if (size <= 64) return 3;
    if (size <= 80) return 4;
    if (size <= 96) return 5;
    if (size <= 128) return 6;
    if (size <= 192) return 7;
    if (size <= 256) return 8;
    if (size <= 384) return 9;
    if (size <= 512) return 10;
    if (size <= 1024) return 11;
    if (size <= 2048) return 12;
    if (size <= 4096) return 13;
    if (size <= 8192) return 14;
    if (size <= 16384) return 15;
    if (size <= 32768) return 16;
    if (size <= 65536) return 17;
    if (size <= 131072) return 18;
    return 19;
}

int mm_init(void)
{
    // Segregated free list 초기화
    for (int i = 0; i < SEGREGATED_SIZE; i++) {
        segregated_lists[i] = NULL;
    }
    
    // 플래그 초기화
    reserved_48_block = NULL;
    freed_48_block = 0;

    if ((heap_listp = mem_sbrk(16 * WSIZE)) == (void *)-1){
        return -1;
    };

    PUT(heap_listp, 0);                            // 패딩
    PACK_PUT(heap_listp + (1 * WSIZE), DSIZE, 0, 1, 1); // 프롤로그 헤더
    PACK_PUT(heap_listp + (2 * WSIZE), DSIZE, 0, 1, 1); // 프롤로그 풋터
    PACK_PUT(heap_listp + (3 * WSIZE), 6*DSIZE, 0, 1, 1); // 48짜리 예약 블록 헤더 (이전 블록 allocated)
    PACK_PUT(heap_listp + (15 * WSIZE), DSIZE, 0, 1, 1);     // 에필로그 헤더 (이전 블록 allocated)
    
    heap_listp += (2 * WSIZE);//프롤로그 중간
    epilogue=heap_listp+(13*WSIZE);//에필로그 헤더 시작
    
    reserved_48_block = heap_listp + (2 * WSIZE); // 48바이트 블록 주소 저장
    
    if (extend_heap(CHUNKSIZE) == NULL){//4096바이트 할당 시도 불가능하면 -1
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t size)//입력 사이즈는 8의 배수로 입력됨
{
    char *ptr;

    if ((long)(ptr = mem_sbrk(size)) == -1){
        return NULL;
    }

    /* 할당되지 않은 free 상태인 블록의 헤더/풋터와 에필로그 헤더를 초기화한다 */
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
    PACK_PUT(HDRP(ptr), size, 1, 1, 0);// 새 블록의 헤더 (이전 블록 alloc 상태 유지)
    PUT(FTRP(ptr), PACK(size, 0));         // 새 블록의 풋터
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1)); // 에필로그 헤더 (prev_alloc은 0, 현재는 free)
    epilogue=HDRP(NEXT_BLKP(ptr));

    /* 만약 이전 블록이 free 였다면, coalesce(통합) 한다*/
    return coalesce(ptr);
}


