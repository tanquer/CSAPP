/*
 * reference : https://github.com/mightydeveloper/Malloc-Lab/blob/master/mm.c
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Tanquer",
    /* First member's full name */
    "Qiye Tan",
    /* First member's email address */
    "qiye.c.tan@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst
#define INITCHUNK   (1<<6)
#define LISTNUM     20 
#define MIN_FRNODE  2

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)          (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)     (GET(p) = (val))                  //line:vm:mm:put

/* Read and write an address at pointer p; in i386 it is the same as macros GET/PUT above */
#define GET_ADDR(p)         (*(char **)(p))
#define PUT_ADDR(p, val)    (GET_ADDR(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p)    (GET(p) & 0x1)                    //line:vm:mm:getalloc
#define GET_REALLOC(p)  (GET(p) & 0x2)
#define SET_REALLOC(p)  (GET(p) |= 0x2)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp
#define PREV_FOOT(bp)   ((char *)(bp) - DSIZE)

/* Given free block ptr bp, compute address of next and previous free blocks pointer */
#define PREV_FREE_P(bp) ((char *)(bp))
#define NEXT_FREE_P(bp) ((char *)(bp) + WSIZE)
/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
void *free_lists[LISTNUM];

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *coalesce(void *bp, int flag);
static void insert_node(void *ptr);
static void delete_node(void *ptr);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);

// static void check_free_blocks_marked_free();
// static void check_contiguous_free_block_coalesced();
// static void check_all_free_blocks_in_free_list();
// static void check_all_free_blocks_valid_ftr_hdr();
// static void check_ptrs_valid_heap_address();
// static int mm_check();

static void insert_node(void *ptr) {
    if (ptr == NULL) return;

    int i = 0;
    size_t size = GET_SIZE(HDRP(ptr));
    if (size == 0) return;

    while (i < (LISTNUM - 1) && size > MIN_FRNODE) {
        i++;
        size >>= 1;
    }

    char *pb = (char *)free_lists[i];
    char *pb_prev = NULL;
    if (pb == NULL) {
        PUT_ADDR(NEXT_FREE_P(ptr), NULL);
        PUT_ADDR(PREV_FREE_P(ptr), NULL);
        free_lists[i] = ptr;
    } else {
        while (pb != NULL && GET_SIZE(HDRP(pb)) < size) {
            pb_prev = pb;
            pb = GET_ADDR(NEXT_FREE_P(pb));
        }
        if (pb == NULL) {
            PUT_ADDR(PREV_FREE_P(ptr), pb_prev);
            PUT_ADDR(NEXT_FREE_P(pb_prev), ptr);
            PUT_ADDR(NEXT_FREE_P(ptr), NULL);
        } else if (pb_prev == NULL) {
            PUT_ADDR(PREV_FREE_P(ptr), NULL);
            PUT_ADDR(NEXT_FREE_P(ptr), pb);
            PUT_ADDR(PREV_FREE_P(pb), ptr);
            free_lists[i] = ptr;
        } else {
            PUT_ADDR(NEXT_FREE_P(pb_prev), ptr);
            PUT_ADDR(PREV_FREE_P(pb), ptr);
            PUT_ADDR(PREV_FREE_P(ptr), pb_prev);
            PUT_ADDR(NEXT_FREE_P(ptr), pb);
        }
    }
}

static void delete_node(void *ptr) {
    if (ptr == NULL) return;
    int i = 0;
    size_t size = GET_SIZE(HDRP(ptr));
    if (size == 0) return;

    while (i < (LISTNUM - 1) && size > MIN_FRNODE) {
        i++;
        size >>= 1;
    }

    char *pb_next = GET_ADDR(NEXT_FREE_P(ptr));
    char *pb_prev = GET_ADDR(PREV_FREE_P(ptr));
    
    if (pb_prev == NULL) {
        free_lists[i] = pb_next;
        if (pb_next != NULL)
            PUT_ADDR(PREV_FREE_P(pb_next), NULL);
    } else {
        if (pb_next == NULL) {
            PUT_ADDR(NEXT_FREE_P(pb_prev), NULL);
        } else {
            PUT_ADDR(NEXT_FREE_P(pb_prev), pb_next);
            PUT_ADDR(PREV_FREE_P(pb_next), pb_prev);
        }
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) 
{
    int i;
    for (i = 0; i < LISTNUM; i++) {
        free_lists[i] = NULL;
    }
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) //line:vm:mm:begininit
        return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     //line:vm:mm:endinit  

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(INITCHUNK) == NULL) 
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) 
{   
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp = NULL;      

    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
        asize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
    else
        asize = ALIGN(size + DSIZE); //line:vm:mm:sizeadjust3

    /* Search the free list for a fit */
    int i = 0, tmpsize = asize;
    while (i < LISTNUM) {
        if ((tmpsize <= MIN_FRNODE && free_lists[i] != NULL) || i == (LISTNUM - 1)) {
            bp = free_lists[i];
            while (bp != NULL && asize > GET_SIZE(HDRP(bp))) {
                bp = GET_ADDR(NEXT_FREE_P(bp));
            }
            if (bp != NULL) break;
        }
        i++;
        tmpsize >>= 1;
    }

    /* No fit found. Get more memory and place the block */
    if (bp == NULL) {
        extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
        if ((bp = extend_heap(extendsize)) == NULL)  
            return NULL;                                  //line:vm:mm:growheap2
    }
    
    bp = place(bp, asize);                                 //line:vm:mm:growheap3
    return bp;
} 
/* $end mmmalloc */

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{   
    if (bp == 0) 
        return;

    size_t size = GET_SIZE(HDRP(bp));
    if (heap_listp == 0){
        mm_init();
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_node(bp);
    coalesce(bp, 1);
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin coalesce */
static void *coalesce(void *bp, int flag) 
{
    size_t prev_alloc = GET_ALLOC(PREV_FOOT(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    }
    if (flag)
        delete_node(bp);

    if (prev_alloc && !next_alloc) {      /* Case 2 */
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    if (flag)
        insert_node(bp);
    return bp;
}
/* $end coalesce */

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{   
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    size_t oldsize = GET_SIZE(HDRP(ptr));
    size = ALIGN(size + DSIZE);
    if (oldsize >= size) {
        return ptr;
    } else {
        void *new_ptr = coalesce(ptr, 0);
        size_t new_size = GET_SIZE(HDRP(new_ptr));
        if (new_size >= size) {
            memmove(new_ptr, ptr, oldsize);         // The memory areas may overlap
            PUT(HDRP(new_ptr), PACK(new_size, 1));
            PUT(FTRP(new_ptr), PACK(new_size, 1));
            return new_ptr;
        } else {
            void *new_malloc_ptr = mm_malloc(size);
            /* If realloc() fails the original block is left untouched  */
            if(!new_malloc_ptr) {
                return 0;
            }
            memcpy(new_malloc_ptr, ptr, oldsize);
            insert_node(new_ptr);
            return new_malloc_ptr;
        }
    }
}

/* 
 * mm_checkheap - Check the heap for correctness
 */
void mm_checkheap(int verbose)  
{ 
    checkheap(verbose);
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t asize) 
{   
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = ALIGN(asize); //line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        //line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr
    insert_node(bp);

    /* Coalesce if the previous block was free */
    return coalesce(bp, 1);                                          //line:vm:mm:returnblock
}
/* $end mmextendheap */

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void *place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    size_t remain = csize - asize;
    
    delete_node(bp);

    if (remain <= (2*DSIZE)) { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    else if (asize >= 120){             // optimize for binary-bal.rep & binary2-bal.rep
        PUT(HDRP(bp), PACK(remain, 0));
        PUT(FTRP(bp), PACK(remain, 0));
        insert_node(bp);
        PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
        return NEXT_BLKP(bp);
    } else {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remain, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remain, 0));
        insert_node(NEXT_BLKP(bp));
    }
    return bp;
}
/* $end mmplace */

static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp, 
           hsize, (halloc ? 'a' : 'f'), 
           fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}

/* 
 * checkheap - Minimal check of the heap for consistency 
 */
void checkheap(int verbose) 
{
    char *bp = heap_listp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose) 
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}
