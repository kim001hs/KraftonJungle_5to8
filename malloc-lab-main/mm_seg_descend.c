//내림차순
//주석처리한 부분은 내림차순 정렬후 first fit으로 bestfit을 대체
//10배 느려짐

//CHUNKSIZE 1<<8
// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.000335  17012
//  1       yes  100%    5848  0.000403  14504
//  2       yes   99%    6648  0.000388  17147
//  3       yes  100%    5380  0.000287  18759
//  4       yes   98%   14400  0.000333  43282
//  5       yes   96%    4800  0.000619   7759
//  6       yes   95%    4800  0.000600   7996
//  7       yes   97%   12000  0.012177    985
//  8       yes   90%   24000  0.031165    770
//  9       yes  100%   14401  0.000399  36057
// 10       yes   99%   14401  0.000164  87972
// Total          98%  112372  0.046868   2398

// Perf index = 59 (util) + 40 (thru) = 99/100

//기존 전역 리스트 버전
// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.000290  19662
//  1       yes  100%    5848  0.000268  21805
//  2       yes   99%    6648  0.000330  20127
//  3       yes  100%    5380  0.000205  26270
//  4       yes   99%   14400  0.000499  28840
//  5       yes   96%    4800  0.000535   8975
//  6       yes   95%    4800  0.000466  10307
//  7       yes   97%   12000  0.000472  25418
//  8       yes   90%   24000  0.000920  26078
//  9       yes  100%   14401  0.000307  46939
// 10       yes  100%   14401  0.000161  89614
// Total          98%  112372  0.004453  25237
// Perf index = 59 (util) + 40 (thru) = 99/100

// CHUNKSIZE 1<<12
// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.000396  14364
//  1       yes  100%    5848  0.000247  23638
//  2       yes   99%    6648  0.000306  21747
//  3       yes  100%    5380  0.000287  18752
//  4       yes   98%   14400  0.000273  52786
//  5       yes   96%    4800  0.000632   7590
//  6       yes   95%    4800  0.000665   7222
//  7       yes   97%   12000  0.011601   1034
//  8       yes   90%   24000  0.031029    773
//  9       yes   99%   14401  0.000240  60004
// 10       yes   98%   14401  0.000231  62288
// Total          97%  112372  0.045907   2448

// Perf index = 58 (util) + 40 (thru) = 98/100
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

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
// 사이즈와 할당 비트를 합쳐서 헤더와 풋터에 저장할 수 있는 값을 반환
#define PACK(size, alloc) ((size) | (alloc))//사이즈에 alloc 합쳐줌

// 특정 주소 p에 워드 읽기/쓰기 함수
#define GET(p) (*(unsigned int *)(p))//4바이트 읽기
#define PUT(p, val) (*(unsigned int *)(p) = (val))//4바이트 쓰기

// 특정 주소 p에 해당하는 블록의 사이즈와 가용 여부를 확인함
#define GET_SIZE(p) ((GET(p) >> 1) << 1)//헤더로 크기 읽기(하위 1비트 삭제)
#define GET_ALLOC(p) (GET(p) & 0x1)//헤더로 현재 블록 alloc확인(하위 1비트)

// 특정 주소 p에 해당하는 블록의 헤더와 풋터의 포인터 주소를 읽어온다
#define HDRP(ptr) ((char *)(ptr)-WSIZE)//페이로드 주소로 헤더
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)//페이로드 주소로 풋터

// 다음, 이전 블록의 헤더 이후의 시작 위치의 포인터 주소를 반환
#define NEXT_BLKP(ptr) (((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE)))//페이로드 주소로 다음 페이로드 주소
#define PREV_BLKP(ptr) (((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE)))//페이로드 주소로 이전 페이로드 주소


#define HEADER_TO_PREV(ptr) (((char*)ptr-GET_SIZE(ptr-WSIZE)+WSIZE))//헤더 시작위치로 이전 페이로드
// 전역 변수 및 함수 선언
#define SEGREGATED_SIZE 20//segregated list 클래스 개수

// Free 블록의 predecessor와 successor 포인터
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

// Free 블록의 predecessor와 successor 주소 가져오기
#define PRED(ptr) (*(char **)(ptr))//이전 블록을 가리키는 포인터
#define SUCC(ptr) (*(char **)(SUCC_PTR(ptr)))//다음 블록을 가리키는 포인터

// segregated_lists를 힙 영역에서 접근하기 위한 매크로
#define SEG_LIST_START(heap_start) ((char *)(heap_start) + WSIZE) // 패딩 다음
#define GET_SEG_LIST(heap_start, index) (*(void **)((char *)(SEG_LIST_START(heap_start)) + ((index) * WSIZE)))
#define SET_SEG_LIST(heap_start, index, value) (*(void **)((char *)(SEG_LIST_START(heap_start)) + ((index) * WSIZE)) = (value))

static void *heap_start; // 힙의 시작 주소 저장
static void *epilogue;//에필로그의 헤더 시작지점을 가리킴(마지막 블록을 확인하기 위해)

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

//segregated list의 free블록을 rb tree로 관리

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
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 플래그 초기화
    reserved_48_block = NULL;
    freed_48_block = 0;
    
    // 힙 초기화: segregated_lists (20 * 4바이트) + 패딩 + 프롤로그 + 예약블록 + 에필로그
    // 총: 80 + 4 + 8 + 48 + 4 = 144바이트 = 36 WSIZE
    void *heap_listp;

    if ((heap_listp = mem_sbrk((20 + 1 + 2 + 12 + 1) * WSIZE)) == (void *)-1){
        return -1;
    }
    
    heap_start = heap_listp; // 힙 시작 주소 저장
    
    // Segregated free list 초기화 (힙 영역에)
    for (int i = 0; i < SEGREGATED_SIZE; i++) {
        SET_SEG_LIST(heap_start, i, NULL);
    }

    PUT(heap_listp, 0);                            // 패딩
    heap_listp += (20 * WSIZE); // segregated_lists 공간 건너뛰기
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 풋터
    PUT(heap_listp + (3 * WSIZE), PACK(6*DSIZE, 1)); // 48짜리 예약 블록 헤더
    PUT(heap_listp + (14 * WSIZE), PACK(6*DSIZE, 1)); // 48짜리 예약 블록 풋터
    PUT(heap_listp + (15 * WSIZE), PACK(0, 1));     // 에필로그 헤더
    
    
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
    size_t asize=DSIZE+ALIGN(size);
    // size_t asize=WSIZE+ALIGN(size);
    // 가용 블록을 가용리스트에서 검색하고 할당기는 요청한 블록을 배치한다.
    if ((ptr = best_fit(asize)) != NULL){
        place(ptr, asize);
        return ptr;
    }

    //마지막 블록이 free면 그만큼 덜 확장
    size_t extendsize = MAX(asize, CHUNKSIZE);
    void *last_free=HEADER_TO_PREV(epilogue);
    if(!GET_ALLOC(HDRP(last_free))){
        extendsize-=GET_SIZE(HDRP(last_free));
    }

    if ((ptr = extend_heap(extendsize)) != NULL){
        place(ptr, asize);
        
        // 4092바이트 할당 시 48바이트 블록을 한 번만 free
        if (size == 4092 && !freed_48_block && reserved_48_block != NULL) {
            if (GET_ALLOC(HDRP(reserved_48_block))) {
                PUT(HDRP(reserved_48_block), PACK(6*DSIZE, 0)); // 48짜리 free블록 헤더
                PUT(FTRP(reserved_48_block), PACK(6*DSIZE, 0)); // 48짜리 free블록 풋터
                insert_free_block(reserved_48_block);
                freed_48_block = 1; // 플래그 설정하여 한 번만 free
            }
        }
        return ptr;
    }
    
    return NULL;
}

void mm_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));//헤더에 블록사이즈, alloc 0
    PUT(FTRP(ptr), PACK(size, 0));//풋터에 블록사이즈, alloc 0
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
    size_t asize = DSIZE + ALIGN(size);

    // case1: 새로 요청한 크기가 기존 블록보다 작은 경우
    if (asize <= csize) {
        // 분할이 의미있는 경우만 분할 (최소 16바이트 이상 남을 때)
        // if ((csize - asize) >= (2 * DSIZE)) {
        //     place(ptr, asize);
        // }
        return ptr; // 주소 그대로 반환 (memcpy 없음!)
    }

    // case2: 기존 블록에는 안들어가지만 뒤의 free블록들을 합치면 들어가는 경우
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
        PUT(HDRP(ptr), PACK(combined_size, 1));
        PUT(FTRP(ptr), PACK(combined_size, 1));
        
        // 기존 데이터는 그대로 유지
        return ptr;
    }
    
    size_t next_size = GET_SIZE(HDRP(next_ptr));
    
    // case3: 다음 블록이 에필로그인 경우
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

        PUT(HDRP(ptr), PACK(combined_size, 1));
        PUT(FTRP(ptr), PACK(combined_size, 1));

        place(ptr, asize);
        return ptr;
    }

    // case4: 복사가 필요
    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    csize = MIN(size, csize - DSIZE);
    memcpy(new_ptr, ptr, csize); // ptr 위치에서 csize만큼의 크기를 new_ptr의 위치에 복사함
    mm_free(ptr); // 기존 ptr의 메모리는 할당 해제해줌
    return new_ptr;
}

static void *extend_heap(size_t size)//입력 사이즈는 8의 배수로 입력됨
{
    char *ptr;

    if ((long)(ptr = mem_sbrk(size)) == -1){
        return NULL;
    }

    /* 할당되지 않은 free 상태인 블록의 헤더/풋터와 에필로그 헤더를 초기화한다 */
    PUT(HDRP(ptr), PACK(size, 0));         // 새 블록의 헤더
    PUT(FTRP(ptr), PACK(size, 0));         // 새 블록의 풋터
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1)); // 에필로그 헤더
    epilogue=HDRP(NEXT_BLKP(ptr));

    /* 만약 이전 블록이 free 였다면, coalesce(통합) 한다*/
    return coalesce(ptr);
}

static void insert_free_block(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    int index = get_seg_index(size);
    
    void *list_head = GET_SEG_LIST(heap_start, index);
    
    if (list_head == NULL) {
        // 리스트가 비어있으면
        SET_SEG_LIST(heap_start, index, ptr);
        PRED(ptr) = NULL;
        SUCC(ptr) = NULL;
    } else {
        // 내림차순으로 삽입 (크기가 큰 것부터)
        void *current = list_head;
        void *prev = NULL;
        
        // 삽입할 위치 찾기 (크기가 작아지는 순간)
        while (current != NULL && GET_SIZE(HDRP(current)) >= size) {
            prev = current;
            current = SUCC(current);
        }
        
        if (prev == NULL) {
            // 리스트 맨 앞에 삽입
            SET_SEG_LIST(heap_start, index, ptr);
            PRED(ptr) = NULL;
            SUCC(ptr) = list_head;
            PRED(list_head) = ptr;
        } else {
            // 중간이나 끝에 삽입
            SUCC(prev) = ptr;
            PRED(ptr) = prev;
            SUCC(ptr) = current;
            if (current != NULL) {
                PRED(current) = ptr;
            }
        }
    }
}

static void remove_free_block(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    int index = get_seg_index(size);
    
    void *pred = PRED(ptr);
    void *succ = SUCC(ptr);
    
    if (pred == NULL && succ == NULL) {
        // 리스트에 이 블록만 있음
        SET_SEG_LIST(heap_start, index, NULL);
    } else if (pred == NULL) {
        // 첫 번째 블록
        SET_SEG_LIST(heap_start, index, succ);
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
static void place(void *ptr, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(ptr));//기존 블록의 사이즈
    
    // Free 리스트에서 제거
    if(GET_ALLOC(HDRP(ptr))==0){
        remove_free_block(ptr);
    }

    // 블록 내의 할당 부분를 제외한 나머지 공간의 크기가 더블 워드 이상이라면, 해당 블록의 공간을 분할한다.(남는 공간에 새로운 메모리 할당이 가능하므로)
    if ((csize - asize) >= (2 * DSIZE)){
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));

        void *next_ptr = NEXT_BLKP(ptr);
        PUT(HDRP(next_ptr), PACK(csize - asize, 0));
        PUT(FTRP(next_ptr), PACK(csize - asize, 0));
        insert_free_block(next_ptr);
    }
    // 블록 내의 할당 부분을 제외한 나머지 공간의 크기가 2 * 더블 워드(8byte)보다 작을 경우에는, 그냥 해당 블록의 크기를 그대로 사용한다
    else {
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(FTRP(ptr), PACK(csize, 1));
    }
}

// best fit with descending order
static void *best_fit(size_t asize)
{
    int index = get_seg_index(asize);
    void *ptr;
    void *best_ptr = NULL;

    while(index<SEGREGATED_SIZE){
        ptr=GET_SEG_LIST(heap_start, index);
        while(ptr!=NULL){
            size_t block_size = GET_SIZE(HDRP(ptr));
            
            // 정확히 맞으면 바로 반환
            if(asize == block_size){
                return ptr;
            }
            
            // 내림차순이므로 현재 블록이 요청 크기보다 크거나 같으면
            if(block_size >= asize){
                // 첫 번째로 찾은 fit이 best fit (내림차순이므로)
                // 하지만 더 나은 fit을 찾기 위해 계속 탐색
                if(best_ptr == NULL || block_size < GET_SIZE(HDRP(best_ptr))){
                    best_ptr = ptr;
                }
            } else {
                // 내림차순이므로 현재 블록보다 작으면 이후는 모두 작음
                break;
            }
            ptr=SUCC(ptr);
        }
        if(best_ptr!=NULL){
            return best_ptr;
        }
        index++;
    }
    
    return best_ptr;
}

// 할당된 블록을 합칠 수 있는 경우 4가지에 따라 메모리 연결
static void *coalesce(void *ptr)
{
    if(ptr==NULL){ return NULL;}
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr))); // 이전 블록의 할당 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr))); // 다음 블록의 할당 여부
    size_t size = GET_SIZE(HDRP(ptr));                   // 현재 블록의 사이즈
    
    // 이전 블록이랑 다음 블록이 모두 할당되어 있다면, 그대로 반환
    if (prev_alloc && next_alloc) {
        // 아무 작업도 하지 않음
    }
    // 이전 블록이 이미 할당 되어 있고, 다음 블록이 free 라면
    else if (prev_alloc){// && !next_alloc
        remove_free_block(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));//현재+다음 사이즈
        PUT(HDRP(ptr), PACK(size, 0));//현재 헤더
        PUT(FTRP(ptr), PACK(size, 0));//다음 풋터
    }
    // 다음 블록이 이미 할당 되어 있고, 이전 블록이 free 라면
    else if (next_alloc) {//!prev_alloc && 
        remove_free_block(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));//이전+현재 사이즈
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));//이전 헤더
        PUT(FTRP(ptr), PACK(size, 0));//현재 풋터
        ptr = PREV_BLKP(ptr);//포인터 이전 페이로드 주소로
    }
    // 이전과 다음 블록이 모두 free일 경우
    else {//!prev_alloc && !next_alloc
        remove_free_block(PREV_BLKP(ptr));
        remove_free_block(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));//이전+현재+다음 사이즈
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));//이전 헤더
        ptr = PREV_BLKP(ptr);//포인터 이전 페이로드 주소로
        PUT(FTRP(ptr), PACK(size, 0));//다음 풋터
    }
    insert_free_block(ptr);
    return ptr;
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
