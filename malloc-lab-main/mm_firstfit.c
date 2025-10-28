//./mdriver -a -V         로 실행

//에필로그 없을 때
// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.006679    853
//  1       yes   99%    5848  0.006702    873
//  2       yes   99%    6648  0.009319    713
//  3       yes  100%    5380  0.006739    798
//  4       yes   66%   14400  0.000114 126095
//  5       yes   92%    4800  0.008484    566
//  6       yes   92%    4800  0.007939    605
//  7       yes   55%   12000  0.177909     67
//  8       yes   51%   24000  0.207963    115
//  9       yes   80%   14401  0.000161  89170
// 10       yes   46%   14401  0.000106 136373
// Total          80%  112372  0.432115    260

// Perf index = 48 (util) + 17 (thru) = 65/100

//에필로그 추가 후

// Results for mm malloc:
// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.006577    866
//  1       yes  100%    5848  0.006820    857
//  2       yes   99%    6648  0.009754    682
//  3       yes  100%    5380  0.006696    804
//  4       yes  100%   14400  0.000120 120301
//  5       yes   92%    4800  0.008841    543
//  6       yes   92%    4800  0.008355    574
//  7       yes   55%   12000  0.147716     81
//  8       yes   51%   24000  0.199282    120
//  9       yes   98%   14401  0.000120 119809
// 10       yes   86%   14401  0.000114 126547
// Total          88%  112372  0.394394    285

// Perf index = 53 (util) + 19 (thru) = 72/100


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

// 사이즈와 할당 비트를 합쳐서 헤더와 풋터에 저장할 수 있는 값을 반환
#define PACK(size, alloc) ((size) | (alloc))//사이즈에 alloc 합쳐줌

// 특정 주소 p에 워드 읽기/쓰기 함수
#define GET(p) (*(unsigned int *)(p))//4바이트 읽기
#define PUT(p, val) (*(unsigned int *)(p) = (val))//4바이트 쓰기

// 특정 주소 p에 해당하는 블록의 사이즈와 가용 여부를 확인함
#define GET_SIZE(p) ((GET(p) >> 1) << 1)//헤더로 크기 읽기(하위 1바이트 삭제)
#define GET_ALLOC(p) (GET(p) & 0x1)//헤더로 alloc확인(하위 1비트만 남김)

// 특정 주소 p에 해당하는 블록의 헤더와 풋터의 포인터 주소를 읽어온다
#define HDRP(ptr) ((char *)(ptr)-WSIZE)//페이로드 주소로 헤더
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)//페이로드 주소로 풋터

// 다음, 이전 블록의 헤더 이후의 시작 위치의 포인터 주소를 반환
#define NEXT_BLKP(ptr) (((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE)))//페이로드 주소로 다음 페이로드 주소
#define PREV_BLKP(ptr) (((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE)))//페이로드 주소로 이전 페이로드 주소


#define HEADER_TO_PREV(ptr) (((char*)ptr-GET_SIZE(ptr-WSIZE)+WSIZE))//헤더 시작위치로 이전 페이로드
// 전역 변수 및 함수 선언
static void *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void *best_fit(size_t asize);
static void place(void *ptr, size_t asize);
static void *epilogue;//에필로그의 헤더를 가리킴
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // mem_sbrk: 힙 영역을 incr(0이 아닌 양수) bytes 만큼 확장하고, 새로 할당된 힙 영역의 첫번째 byte를 가리키는 제네릭 포인터를 리턴함
    /* 비어있는 heap을 만든다.*/
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1){
        return -1;
    };

    PUT(heap_listp, 0);                            // 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 풋터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 헤더
    heap_listp += (2 * WSIZE);//프롤로그 중간
    epilogue=heap_listp+WSIZE;//에필로그 헤더 시작
    if (extend_heap(CHUNKSIZE) == NULL){//4096바이트 할당 시도 불가능하면 -1
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /* 의미 없는 요청 처리 안함 */
    if (size == 0){
        return NULL;
    }
    
    char *ptr;
    size_t asize=DSIZE+ALIGN(size);
    // 가용 블록을 가용리스트에서 검색하고 할당기는 요청한 블록을 배치한다.
    if ((ptr = best_fit(asize)) != NULL){
        place(ptr, asize);
        return ptr;
    }

    /* 리스트에 들어갈 수 있는 free 리스트가 없을 경우, 메모리를 확장하고 블록을 배치한다 */
    size_t extendsize = MAX(asize, CHUNKSIZE);
    void *last_free=HEADER_TO_PREV(epilogue);
    if(!GET_ALLOC(HDRP(last_free))){
        extendsize-=GET_SIZE(HDRP(last_free));
    }
    if ((ptr = extend_heap(extendsize)) != NULL){
        place(ptr, asize);
        return ptr;
    }
    return NULL;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));//헤더에 블록사이즈, alloc 0
    PUT(FTRP(ptr), PACK(size, 0));//풋터에 블록사이즈, alloc 0
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 
- 만약 ptr == NULL 이면, `mm_malloc(size)`과 동일한 동작을 수행한다.
- 만약 size가 0 이면, `mm_free(ptr)`와 동일한 동작을 수행한다.
- 만약 ptr != NULL 이면, 이전에 `mm_malloc()` 또는 `mm_realloc()`을 이용해 반환값이 존재하는 상태라고 볼 수 있다.
  이때 `mm_realloc()`을 호출하면, ptr이 가르키는 메모리 블록(이전 블록)의 메모리 크기가 바이트 단위로 변경된다. 이후 새 블록의 주소를 return 한다.
  새블록의 크기는 이전 ptr 블록의 크기와 최소한의 크기까지는 동일하고, 그 이외의 범위는 초기화되지 않는다. 예를 들어 이전 블록이 8바이트이고, 새 블록이 12바이트인 경우 첫 8바이트는 이전 블록과 동일하고 이후 4바이트는 초기화되지 않은 상태임을 의미한다.
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    } 

    size_t new_asize = DSIZE + ALIGN(size);
    size_t csize = GET_SIZE(HDRP(ptr));//현재 사이즈

    // 1. [In-Place 축소/유지] 새로 요청한 크기가 현재 블록에 들어가는 경우
    if (new_asize <= csize) {
        // 충분히 남는 공간이 있다면 축소 (분할)
        place(ptr,new_asize);
        return ptr; // 주소 그대로 반환 (memcpy 없음!)
    }

    // 2. [In-Place 확장] 현재 블록에 안 들어가지만 다음 블록이 Free이고 합쳐서 들어가는 경우
    void *next_ptr = NEXT_BLKP(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(next_ptr));
    size_t next_size = GET_SIZE(HDRP(next_ptr));
    size_t combined_size = csize + next_size;

    if (!next_alloc && combined_size >= new_asize) {// 다음 블록이 가용(Free)이고, 합친 크기가 요청 크기보다 크거나 같다면
        // 1. 두 블록 통합 (현재 블록 헤더/다음 블록 풋터 업데이트)
        PUT(HDRP(ptr), PACK(combined_size, 1)); // 새 헤더 (통합된 크기, 할당됨)
        PUT(FTRP(next_ptr), PACK(combined_size, 1)); // 새 풋터 (통합된 크기, 할당됨)

        // 2. 필요하다면 분할 (place 함수가 알아서 처리)
        place(ptr, new_asize); 

        return ptr; // 주소 그대로 반환 (memcpy 없음!)
    }
    
    // 2-1. [힙 끝에서 확장] 다음 블록이 에필로그(힙의 끝)이면 힙을 확장
    if (next_size == 0) { // 에필로그 (크기 0)
        size_t need_size = new_asize - csize;
        void *last_free = HEADER_TO_PREV(epilogue);
        
        // 마지막 free 블록이 있으면 그만큼 덜 확장
        if(!GET_ALLOC(HDRP(last_free))){
            need_size -= GET_SIZE(HDRP(last_free));
        }
        
        size_t extend_size = MAX(need_size, CHUNKSIZE);
        
        if (extend_heap(extend_size) != NULL) {
            // 확장 후 다시 다음 블록 확인
            next_ptr = NEXT_BLKP(ptr);
            next_size = GET_SIZE(HDRP(next_ptr));
            combined_size = csize + next_size;
            
            PUT(HDRP(ptr), PACK(combined_size, 1));
            PUT(FTRP(ptr), PACK(combined_size, 1));
            place(ptr, new_asize);
            
            return ptr;
        }
    }

    // 3. [Out-of-Place 할당] In-Place 확장이 불가능하거나 다음 블록이 에필로그인 경우
    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    if (size < csize) { // 재할당 요청에 들어온 크기보다, 기존 블록의 크기가 크다면
        csize = size; // 기존 블록의 크기를 요청에 들어온 크기 만큼으로 줄인다.
    }
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
static void *extend_heap(size_t size)
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

// 메모리 영역에 메모리 블록을 위치시키는 함수
static void place(void *ptr, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(ptr));//기존 블록의 사이즈

    // 블록 내의 할당 부분를 제외한 나머지 공간의 크기가 더블 워드 이상이라면, 해당 블록의 공간을 분할한다.(남는 공간에 새로운 메모리 할당이 가능하므로)
    if ((csize - asize) >= (2 * DSIZE)){
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        ptr = NEXT_BLKP(ptr);
        PUT(HDRP(ptr), PACK(csize - asize, 0));
        PUT(FTRP(ptr), PACK(csize - asize, 0));
    }
    // 블록 내의 할당 부분을 제외한 나머지 공간의 크기가 2 * 더블 워드(8byte)보다 작을 경우에는, 그냥 해당 블록의 크기를 그대로 사용한다
    else {
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(FTRP(ptr), PACK(csize, 1));
    }
}

//first fit
static void *best_fit(size_t asize)
{
    void *ptr;

    // 에필로그 헤더(힙의 끝) 까지 탐색한다
    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)){
        // 할당 X and 여유 공간의 크기가 할당 할 크기보다 넉넉할 경우에만
        if (!GET_ALLOC(HDRP(ptr)) && (asize <= GET_SIZE(HDRP(ptr)))){
            return ptr;
        }
    }
    return NULL;
}


// 할당된 블록을 합칠 수 있는 경우 4가지에 따라 메모리 연결
static void *coalesce(void *ptr)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr))); // 이전 블록의 할당 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr))); // 다음 블록의 할당 여부
    size_t size = GET_SIZE(HDRP(ptr));                   // 현재 블록의 사이즈

    // 이전 블록이랑 다음 블록이 모두 할당되어 있다면, 그대로 반환
    if (prev_alloc && next_alloc) {
        return ptr;
    }
    // 이전 블록이 이미 할당 되어 있고, 다음 블록이 free 라면
    else if (prev_alloc){// && !next_alloc
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));//현재+다음 사이즈
        PUT(HDRP(ptr), PACK(size, 0));//현재 헤더
        PUT(FTRP(ptr), PACK(size, 0));//다음 풋터
    }
    // 다음 블록이 이미 할당 되어 있고, 이전 블록이 free 라면
    else if (next_alloc) {//!prev_alloc && 
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));//이전+현재 사이즈
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));//이전 헤더
        PUT(FTRP(ptr), PACK(size, 0));//현재 풋터
        ptr = PREV_BLKP(ptr);//포인터 이전 페이로드 주소로
    }
    // 이전과 다음 블록이 모두 free일 경우
    else {//!prev_alloc && !next_alloc
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(FTRP(NEXT_BLKP(ptr)));//이전+현재+다음 사이즈
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));//이전 헤더
        ptr = PREV_BLKP(ptr);//포인터 이전 페이로드 주소로
        PUT(FTRP(ptr), PACK(size, 0));//다음 풋터
    }
    // return ptr;
    return coalesce(ptr);//계속 합침
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