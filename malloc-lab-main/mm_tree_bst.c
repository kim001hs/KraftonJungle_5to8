//segregated list + BST를 이용한 malloc 구현
//Binary Search Tree를 사용하여 큰 free 블록 관리

// trace  valid  util     ops      secs   Kops
//  0       yes   99%    5694  0.000285  20014
//  1       yes  100%    5848  0.000261  22441
//  2       yes  100%    6648  0.000366  18159
//  3       yes  100%    5380  0.000283  19024
//  4       yes   99%   14400  0.000293  49180
//  5       yes   96%    4800  0.000695   6904
//  6       yes   95%    4800  0.000776   6186
//  7       yes   95%   12000  0.003274   3665
//  8       yes   88%   24000  0.005386   4456
//  9       yes   99%   14401  0.000217  66333
// 10       yes   98%   14401  0.000233  61913
// Total          97%  112372  0.012068   9311

// Perf index = 58 (util) + 40 (thru) = 98/100

// https://github.com/coyorkdow/malloclab/blob/main/mm.c
//위 링크의 코드를 ai로 코드 스타일만 수정함

///////////////////////////////// Block information /////////////////////////////////////////////////////////
/*

A   : Allocated? (1: true, 0:false)
T   : is PREV allocated? (1: true, 0:false)
 < Allocated Block >

             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
bp --->     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  | T| A|
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
 Header :   |                              size of the block                                       |  | T| A|
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
 Footer :   |                              size of the block                                       |  | T| A|
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
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    "individual",
    "coyorkdow",
    "coyorkdow@outlook.com",
    "",
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MINIMUM_TREE_BLOCK_SIZE (3 * DSIZE)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// Pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// Read the size and allocation bit from address p
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// Tag for foot compression, which indicates whether the previous block
// was allocated or not.
#define GET_TAG(p) (GET(p) & 0x2)

// Set or clear allocation bit
#define SET_ALLOC(p) PUT(p, (GET(p) | 0x1))
#define CLR_ALLOC(p) PUT(p, (GET(p) & ~0x1))
#define SET_TAG(p) PUT(p, (GET(p) | 0x2));
#define CLR_TAG(p) PUT(p, (GET(p) & ~0x2));

// Set block's size
#define SET_SIZE(p, size) PUT(p, (size | (GET(p) & 0x7)))

// Address of block's header and footer
#define HDRP(ptr) ((char *)(ptr)-WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

// Address of (physically) next and previous blocks
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE))

// Address of free block's predecessor and successor entries
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

#define BP_LESS(bp, size) (GET_SIZE(HDRP(bp)) < size)
#define BP_GREATER(bp, size) (GET_SIZE(HDRP(bp)) > size)
#define BP_GEQ(bp, size) (!BP_LESS(bp, size))
#define BP_LEQ(bp, size) (!BP_GREATER(bp, size))

static char *heap_listp = 0;    /* Pointer to the prologue block */
static char *heap_epilogue = 0; /* Pointer to the epilogue block */

static char *free_list_head = 0; /* Pointer to the head of explicit free list*/

// Address of free block's predecessor and successor
#define PRED(ptr) (char *)(free_list_head + GET(PRED_PTR(ptr)))
#define SUCC(ptr) (char *)(free_list_head + GET(SUCC_PTR(ptr)))

#define OFFSET(ptr) ((char *)(ptr)-free_list_head)

// Set blocks's predecessor and successor entries
#define SET_PRED(self, ptr) PUT(PRED_PTR(self), OFFSET(ptr))
#define SET_SUCC(self, ptr) PUT(SUCC_PTR(self), OFFSET(ptr))

// Link ptr1 and ptr2 as ptr1 is predecessor of ptr2
#define LINK(ptr1, ptr2)  \
  do {                    \
    SET_SUCC(ptr1, ptr2); \
    SET_PRED(ptr2, ptr1); \
  } while (0)

// Remove ptr from linked list
#define ERASE(ptr) LINK(PRED(ptr), SUCC(ptr))

// Insert target as successor
#define INSERT(self, target)  \
  do {                        \
    LINK(target, SUCC(self)); \
    LINK(self, target);       \
  } while (0)

/*********************************************************
 *        Binary Search Tree Definition Begin
 ********************************************************/

#define LCH(ptr) PRED(ptr)
#define RCH(ptr) SUCC(ptr)

#define PARENT_PTR(ptr) ((char *)(ptr) + (2 * WSIZE))
#define PARENT(ptr) (char *)(free_list_head + GET(PARENT_PTR(ptr)))

#define SET_LCH(self, ptr) SET_PRED(self, ptr)
#define SET_RCH(self, ptr) SET_SUCC(self, ptr)
#define SET_PARENT(self, ptr) PUT(PARENT_PTR(self), OFFSET(ptr))

char *NIL = 0; /* NIL must be initialized before use any of BST */

/* Replaces subtree u as a child of its parent with subtree v. */
#define TRANSPLANT(root, u, v)              \
  do {                                      \
    if (PARENT(u) == NIL) {                 \
      *(root) = v;                          \
    } else if (u == LCH(PARENT(u))) {       \
      SET_LCH(PARENT(u), v);                \
    } else {                                \
      SET_RCH(PARENT(u), v);                \
    }                                       \
    if (v != NIL) SET_PARENT(v, PARENT(u)); \
  } while (0)

static char *tree_lower_bound(char *root, size_t size) {
  char *r = NIL;
  char *ptr = root;
  while (ptr != NIL && size != GET_SIZE(HDRP(ptr))) {
    if (GET_SIZE(HDRP(ptr)) < size) {
      ptr = RCH(ptr);
    } else {
      r = ptr;
      ptr = LCH(ptr);
    }
  }
  if (ptr != NIL && GET_SIZE(HDRP(ptr)) == size) {
    return ptr;
  } else {
    return r;
  }
}

static void tree_insert(char **root, char *ptr) {
  SET_LCH(ptr, NIL);
  SET_RCH(ptr, NIL);
  char *y = NIL;
  char *x = *root;
  size_t size = GET_SIZE(HDRP(ptr));
  while (x != NIL) {    y = x;
    if (GET_SIZE(HDRP(x)) < size) {
      x = RCH(x);
    } else {
      x = LCH(x);
    }
  }
  SET_PARENT(ptr, y);
  if (y == NIL) {
    *root = ptr;
  } else if (GET_SIZE(HDRP(y)) < size) {
    SET_RCH(y, ptr);
  } else {
    SET_LCH(y, ptr);
  }
}

static void tree_erase(char **root, char *ptr) {
  if (LCH(ptr) == NIL) {
    TRANSPLANT(root, ptr, RCH(ptr));
  } else if (RCH(ptr) == NIL) {
    TRANSPLANT(root, ptr, LCH(ptr));
  } else {
    char *y = RCH(ptr);
    while (LCH(y) != NIL) y = LCH(y);
    // y is the minimum node in subtree RCH(ptr), it may has RCH but has no LCH.
    if (PARENT(y) != ptr) {
      TRANSPLANT(root, y, RCH(y));
      SET_RCH(y, RCH(ptr));
      SET_PARENT(RCH(y), y);
    }
    TRANSPLANT(root, ptr, y);
    SET_LCH(y, LCH(ptr));
    SET_PARENT(LCH(y), y);
  }
}
/*********************************************************
 *        Binary Search Tree Definition End
 ********************************************************/

static char *tree_root = 0;

#define ERASE_FROM_TREE_OR_LIST(root, bp)     \
  do {                                        \
    if (BP_LESS(bp, MINIMUM_TREE_BLOCK_SIZE)) \
      ERASE(bp);                              \
    else                                      \
      tree_erase(root, bp);                   \
  } while (0)

static void *extend_heap(size_t words);

static void *coalesce(void *bp);

static void *find_fit(size_t asize);

static void *place(void *bp, size_t asize, bool trivial_split);

/* Check if split free block from the back of the old block */
#define SPLIT_BACK(asize, next_size) (asize < 96 || next_size < 48)

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  /* Create the initial empty heap */
  if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *)-1) {
    return -1;
  }
  free_list_head = heap_listp + WSIZE; /* Init head ptr */
  NIL = free_list_head;                /* Init NIL, which is used in BST */
  tree_root = NIL;
  PUT(heap_listp, 0); /* Alignment padding */
  PUT(heap_listp + WSIZE, 0);
  PUT(heap_listp + (2 * WSIZE), 0);
  PUT(heap_listp + (3 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (4 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(heap_listp + (5 * WSIZE), PACK(0, 3));     /* Epilogue header */
  heap_listp += (4 * WSIZE);

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
    return -1;
  }
  SET_TAG(HDRP(NEXT_BLKP(heap_listp)));
  if (BP_LESS(NEXT_BLKP(heap_listp), MINIMUM_TREE_BLOCK_SIZE)) {
    INSERT(free_list_head, NEXT_BLKP(heap_listp));
  } else {
    tree_insert(&tree_root, NEXT_BLKP(heap_listp));
  }
  return 0;
}

static inline size_t size_in_need(size_t size) {
  return MAX(DSIZE * 2, (ALIGN(size) - size >= WSIZE) ? ALIGN(size)
                                                      : (ALIGN(size) + DSIZE));
}

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1) {
    return NULL;
  }

  /* Initialize free block header/footer and the epilogue header */
  SET_SIZE(HDRP(bp), size); /* Free block header */
  CLR_ALLOC(HDRP(bp));
  PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
  heap_epilogue = HDRP(NEXT_BLKP(bp));

  /* Coalesce if the previous block was free */
  return coalesce(bp);
}

void *mm_malloc_wrapped(size_t size, bool realloc) {
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;

  /* Ignore spurious requests */
  if (size == 0) {
    return NULL;
  }

  /* Adjust block size to include overhead and alignment reqs. */
  asize = size_in_need(size);

  /* Search the free list for a fit */
  if (asize < MINIMUM_TREE_BLOCK_SIZE && (bp = find_fit(asize)) != NULL) {
    return place(bp, asize, false);
  }

  /* Search the BST */
  if ((bp = tree_lower_bound(tree_root, asize)) != NIL) {
    tree_erase(&tree_root, bp);
    return place(bp, asize, false);
  }

  /* No fit found. Get more memory and place the block */

  extendsize = MAX(asize, CHUNKSIZE);
  if (!realloc && !GET_TAG(heap_epilogue)) {
    extendsize -= GET_SIZE(heap_epilogue - WSIZE);
  }
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
    return NULL;
  }
  return place(bp, asize, false);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) { return mm_malloc_wrapped(size, false); }

/*
 * find_fit - Perform a first-fit search of the implicit free list.
 */
void *find_fit(size_t asize) {
  char *ptr = free_list_head;
  while ((ptr = SUCC(ptr)) != free_list_head) {
    if (GET_SIZE(HDRP(ptr)) >= asize) {
      ERASE(ptr);
      return ptr;
    }
  }
  return NULL;
}

/*
 * place - Place the requested block at the beginning of the free block,
 * splitting only if the size of the remainder would equal or exceed the minimum
 * block size.
 */
void *place(void *bp, size_t asize, bool trivial_split) {
  SET_ALLOC(HDRP(bp));
  size_t size = GET_SIZE(HDRP(bp));
  if (size > asize && size - asize >= 2 * DSIZE) {
    size_t next_size = size - asize;
    char *next;
    if (trivial_split || SPLIT_BACK(asize, next_size)) {
      SET_SIZE(HDRP(bp), asize);
      next = NEXT_BLKP(bp);
      PUT(HDRP(next), PACK(next_size, 0));
      PUT(FTRP(next), PACK(next_size, 0));
    } else {
      next = bp;
      SET_SIZE(HDRP(next), next_size);
      PUT(FTRP(next), PACK(next_size, 0));
      bp = NEXT_BLKP(next);
      SET_ALLOC(HDRP(bp));
      SET_SIZE(HDRP(bp), asize);
    }
    SET_TAG(HDRP(NEXT_BLKP(bp)));
    next = coalesce(next);
    SET_TAG(HDRP(next));
    CLR_ALLOC(HDRP(next));
    CLR_TAG(HDRP(NEXT_BLKP(next)));
    if (BP_LESS(next, MINIMUM_TREE_BLOCK_SIZE)) {
      INSERT(free_list_head, next);
    } else {
      tree_insert(&tree_root, next);
    }
  }
  SET_TAG(HDRP(NEXT_BLKP(bp)));
  return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  size_t size = GET_SIZE(HDRP(ptr));

  SET_SIZE(HDRP(ptr), size);
  CLR_ALLOC(HDRP(ptr));
  CLR_TAG(HDRP(NEXT_BLKP(ptr)));
  PUT(FTRP(ptr), PACK(size, 0));
  ptr = coalesce(ptr);
  if (BP_LESS(ptr, MINIMUM_TREE_BLOCK_SIZE)) {
    INSERT(free_list_head, ptr);
  } else {
    tree_insert(&tree_root, ptr);
  }
}

static void *coalesce(void *bp) {
  size_t prev_alloc = GET_TAG(HDRP(bp));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) { /* Case 1 */
    return bp;
  } else if (prev_alloc && !next_alloc) { /* Case 2 */
    ERASE_FROM_TREE_OR_LIST(&tree_root, NEXT_BLKP(bp));
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    SET_SIZE(HDRP(bp), size);
    PUT(FTRP(bp), PACK(size, 0));
  } else if (!prev_alloc && next_alloc) { /* Case 3 */
    ERASE_FROM_TREE_OR_LIST(&tree_root, PREV_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    SET_SIZE(HDRP(PREV_BLKP(bp)), size);
    bp = PREV_BLKP(bp);
  } else { /* Case 4 */
    ERASE_FROM_TREE_OR_LIST(&tree_root, NEXT_BLKP(bp));
    ERASE_FROM_TREE_OR_LIST(&tree_root, PREV_BLKP(bp));
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    SET_SIZE(HDRP(PREV_BLKP(bp)), size);
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return mm_malloc(size);
  }
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }

  void *newptr;
  size_t asize = size_in_need(size);

  size_t next_available_size =
      GET_ALLOC(HDRP(NEXT_BLKP(ptr))) ? 0 : GET_SIZE(HDRP(NEXT_BLKP(ptr)));
  size_t prev_available_size =
      GET_TAG(HDRP(ptr)) ? 0 : GET_SIZE(HDRP(PREV_BLKP(ptr)));
  size_t blocksize = GET_SIZE(HDRP(ptr));

#define COALESCE_NEXT                                    \
  do {                                                   \
    ERASE_FROM_TREE_OR_LIST(&tree_root, NEXT_BLKP(ptr)); \
    blocksize += next_available_size;                    \
    SET_SIZE(HDRP(ptr), blocksize);                      \
    SET_SIZE(FTRP(ptr), blocksize);                      \
    SET_TAG(HDRP(NEXT_BLKP(ptr)));                       \
  } while (0)

#define COALESCE_PREV                                    \
  do {                                                   \
    ERASE_FROM_TREE_OR_LIST(&tree_root, PREV_BLKP(ptr)); \
    char *oldptr = ptr;                                  \
    ptr = PREV_BLKP(ptr);                                \
    memmove(ptr, oldptr, blocksize - WSIZE);             \
    blocksize += prev_available_size;                    \
    SET_SIZE(HDRP(ptr), blocksize);                      \
    SET_SIZE(FTRP(ptr), blocksize);                      \
  } while (0)

#define TRIVAL_PLACE(bp)          \
  do {                            \
    SET_ALLOC(HDRP(bp));          \
    SET_TAG(HDRP(NEXT_BLKP(bp))); \
  } while (0)

  if (blocksize >= asize) {
    TRIVAL_PLACE(ptr);
    return ptr;
  } else if (blocksize + next_available_size >= asize) {
    COALESCE_NEXT;
    TRIVAL_PLACE(ptr);
    return ptr;
  } else if (blocksize + prev_available_size >= asize) {
    COALESCE_PREV;
    TRIVAL_PLACE(ptr);
    return ptr;
  } else if (blocksize + next_available_size + prev_available_size >= asize) {
    COALESCE_NEXT;
    COALESCE_PREV;
    TRIVAL_PLACE(ptr);
    return ptr;
  }

  if (prev_available_size) {
    COALESCE_PREV;
  }
  if (next_available_size) {
    COALESCE_NEXT;
  }
  // Check if ptr points to the last block of the heap, ptr may be NULL
  if (HDRP(NEXT_BLKP(ptr)) == heap_epilogue) {
    TRIVAL_PLACE(ptr);
    char *bp;
    if ((bp = extend_heap((asize - blocksize) / WSIZE)) == NULL) {
      return NULL;
    }
    blocksize += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    SET_SIZE(HDRP(ptr), blocksize);
    SET_SIZE(FTRP(ptr), blocksize);
    SET_TAG(HDRP(NEXT_BLKP(ptr)));
    return ptr;
  }

  if ((newptr = mm_malloc_wrapped(size, true)) == NULL) {
    return newptr;
  }
  memcpy(newptr, ptr, blocksize - WSIZE);
  mm_free(ptr);
  return newptr;
#undef TRIVAL_PLACE
#undef COALESCE_PREV
#undef COALESCE_NEXT
}