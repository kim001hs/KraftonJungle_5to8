// best_fit

// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.007515    758
//  1       yes  100%    5848  0.008099    722
//  2       yes   99%    6648  0.010261    648
//  3       yes  100%    5380  0.007217    745
//  4       yes  100%   14400  0.000135 106430
//  5       yes   96%    4800  0.018919    254
//  6       yes   95%    4800  0.017067    281
//  7       yes   55%   12000  0.158243     76
//  8       yes   51%   24000  0.234138    103
//  9       yes   33%   14401  0.005256   2740
// 10       yes   32%   14401  0.000172  83775
// Total          78%  112372  0.467023    241

// Perf index = 47 (util) + 16 (thru) = 63/100

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
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) ((GET(p) >> 1) << 1)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(ptr) ((char *)(ptr)-WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

#define NEXT_BLKP(ptr) (((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE)))
#define PREV_BLKP(ptr) (((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE)))

#define HEADER_TO_PREV(ptr) (((char*)ptr-GET_SIZE(ptr-WSIZE)+WSIZE))
static void *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
// static void *first_fit(size_t asize);
static void *best_fit(size_t asize);
static void place(void *ptr, size_t asize);
static void *epilogue;

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1){
        return -1;
    };

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);
    epilogue=heap_listp+WSIZE;
    if (extend_heap(CHUNKSIZE) == NULL){
        return -1;
    }
    return 0;
}

void *mm_malloc(size_t size)
{
    if (size == 0){
        return NULL;
    }
    
    char *ptr;
    size_t asize=DSIZE+ALIGN(size);
    
    if ((ptr = best_fit(asize)) != NULL){
        place(ptr, asize);
        return ptr;
    }

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

void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
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

    size_t new_asize = DSIZE + ALIGN(size);
    size_t csize = GET_SIZE(HDRP(ptr));

    if (new_asize <= csize) {
        place(ptr,new_asize);
        return ptr;
    }

    void *next_ptr = NEXT_BLKP(ptr);
    size_t next_alloc = GET_ALLOC(HDRP(next_ptr));
    size_t next_size = GET_SIZE(HDRP(next_ptr));
    size_t combined_size = csize + next_size;

    if (!next_alloc && combined_size >= new_asize) {
        PUT(HDRP(ptr), PACK(combined_size, 1));
        PUT(FTRP(next_ptr), PACK(combined_size, 1));
        place(ptr, new_asize); 
        return ptr;
    }
    
    if (next_size == 0) {
        size_t need_size = new_asize - csize;
        void *last_free = HEADER_TO_PREV(epilogue);
        
        if(!GET_ALLOC(HDRP(last_free))){
            need_size -= GET_SIZE(HDRP(last_free));
        }
        
        size_t extend_size = MAX(need_size, CHUNKSIZE);
        
        if (extend_heap(extend_size) != NULL) {
            next_ptr = NEXT_BLKP(ptr);
            next_size = GET_SIZE(HDRP(next_ptr));
            combined_size = csize + next_size;
            
            PUT(HDRP(ptr), PACK(combined_size, 1));
            PUT(FTRP(ptr), PACK(combined_size, 1));
            place(ptr, new_asize);
            
            return ptr;
        }
    }

    void *new_ptr = mm_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    if (size < csize) {
        csize = size;
    }
    memcpy(new_ptr, ptr, csize);
    mm_free(ptr);
    return new_ptr;
}

static void *extend_heap(size_t size)
{
    char *ptr;

    if ((long)(ptr = mem_sbrk(size)) == -1){
        return NULL;
    }

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
    epilogue=HDRP(NEXT_BLKP(ptr));

    return coalesce(ptr);
}

static void place(void *ptr, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(ptr));

    if ((csize - asize) >= (2 * DSIZE)){
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        ptr = NEXT_BLKP(ptr);
        PUT(HDRP(ptr), PACK(csize - asize, 0));
        PUT(FTRP(ptr), PACK(csize - asize, 0));
    }
    else {
        PUT(HDRP(ptr), PACK(csize, 1));
        PUT(FTRP(ptr), PACK(csize, 1));
    }
}

static void *best_fit(size_t asize)
{
    void *ptr;
    void *best_ptr = NULL;
    size_t min_size = 0;

    for (ptr = heap_listp; GET_SIZE(HDRP(ptr)) > 0; ptr = NEXT_BLKP(ptr)){
        if (!GET_ALLOC(HDRP(ptr)) && (asize <= GET_SIZE(HDRP(ptr)))){
            if (best_ptr == NULL || GET_SIZE(HDRP(ptr)) < min_size) {
                best_ptr = ptr;
                min_size = GET_SIZE(HDRP(ptr));
            }
        }
    }
    return best_ptr;
}

static void *coalesce(void *ptr)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc) {
        return ptr;
    }
    else if (prev_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }
    else if (next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(FTRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
        PUT(FTRP(ptr), PACK(size, 0));
    }
    return coalesce(ptr);
}