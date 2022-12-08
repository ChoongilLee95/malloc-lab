/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
// rbtree 헤더
#include <stddef.h>


#include "mm.h"
#include "memlib.h"




// key값에 맞는 노드 
// 노드 삭제
void rbtree_erase(void* deleting);
// 노드 추가
void rbtree_insert(const unsigned int inserting_key, void* inserting);

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */

/* rounds up to the nearest multiple of ALIGNMENT */

#define WSIZE 4           /* word & header/footer size(bytes) */
#define DSIZE 8           /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc) )  /* size, 할당 비트를 통합해서, header, footer에 저장할 수 있는 값 return */

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p)) /* p가 가리키는 워드를 읽어서 return */
#define PUT(p, val) (*(unsigned int *)(p) = (val)) /* p가 가리키는 word에 val을 저장 */

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) /* p에 있는 header/footer size return */
#define GET_ALLOC(p) (GET(p) & 0x1) /* p에 있는 header/footer 할당 비트 return */

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/*Given block ptr bp,compute address of next and previous blocks*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))



/*free_node_insides*/
#define RED (unsigned int)0
#define BLACK (unsigned int)1
#define ALLOC (unsigned int)1
#define FREE (unsigned int)0
#define COLOR_BIT (unsigned int)0b100 // 3번쨰 RB color bit
#define POSITION (unsigned int)(~(0b111)) //  4~29bit block num
#define TWO_BLOCK_BIT (unsigned int)0b10 // 2번쨰 2B free block check bit
#define ALLOC_BIT (unsigned int)0b1 // 1번째 Allov check bit

/*자신의 앞의 블럭이 16byte free block 인지*/
#define IS_FIRST(bp) ((bp == start_point) ? 1 : 0)// 시작점 포인터 이용
#define GET_COLOR(bp) ((*((unsigned int*)bp) & COLOR_BIT) ? BLACK : RED)
#define FRONT_CHECK_2BFREE(bp) ((*(unsigned int *)(bp) & TWO_BLOCK_BIT) ? 1 : 0 )
#define BLOCK_SIZE(bp) (*((unsigned int *)bp) & (POSITION))
#define NUM_BLOCK(bp) ((*((unsigned int *)bp) & (POSITION))/8)
#define GET_ALLOC(bp) ((*(unsigned int *)(bp) & (ALLOC_BIT)) ? ALLOC:FREE)
#define GET_FRONT_ALLOC(bp) ((*(((unsigned int *)bp)-1) & ALLOC_BIT) ? ALLOC : FREE)
#define GET_BACK_ALLOC(bp) ((*((unsigned int *)(((char*)bp)+BLOCK_SIZE(bp))) & ALLOC_BIT) ? ALLOC :FREE)

#define FOOTER_PTR(bp) (unsigned int*)(((char*)bp)+BLOCK_SIZE(bp)-4) //FOOTER포인터 (unsiged int*)
#define IS_END(bp) ((((char*)bp)+BLOCK_SIZE(bp) == end_point) ? 1 : 0) // 블럭의 끝의 포인터가 brk랑 같은지
#define PARENT_PTR(bp) ((void*)(*(((unsigned int *)bp)+1))) // 부모포인터(void pointer)
#define PARENT(bp) (((unsigned int *)bp)+1) //부모포인터의 값를 담고있는 4byte블럭의 unsigned포인터
#define child_L_PTR(bp) ((void *)(*((unsigned int *)(bp)+2))) // L자식포인터(int pointer)
#define child_L(bp) (((unsigned int *)bp)+2) //L포인터의 값를 담고있는 4byte블럭의 unsigned포인터
#define child_R_PTR(bp) ((void *)(*((unsigned int *)(bp)+3))) // R자식포인터(int pointer)
#define child_R(bp) (((unsigned int *)bp)+3) //R포인터의 값를 담고있는 4byte블럭의 unsigned포인터

/* pointer at prologue block */
static char* heap_listp;
static int root;
unsigned long for_nil;
unsigned long* nil = &for_nil;
void *start_point;
char *end_point;
unsigned end_2 = 0;

void two_block_mark(void* bp) // 앞이 2free블럭인 할당된 블럭의 비트마킹 (2freeblock의 포인터 기준)
{
    unsigned* changer = ((unsigned*)bp) + 4;
    *changer = *changer | TWO_BLOCK_BIT;
}
void two_block_demark(void* bp)
{
    unsigned* changer = ((unsigned*)bp) + 4;
    *changer = *changer & (~TWO_BLOCK_BIT);
}

/*
 * first fit function: 
 */ 
// static void *find_fit(size_t asize)
// {
//     void* ptr=;
//     while ()
//     return NULL;
// }

/*
 * place function - allocate block which has 'asize' size
    할당하고 싶어 프리인 메모리가 얼마나 큰지 -> 
 */
// static void place(void *bp, size_t asize)
// {
//     size_t original_size = GET_SIZE(HDRP(bp));

//     if ( (original_size-asize) >= 2*DSIZE )
//     {
//         PUT(HDRP(bp), PACK(asize, 1));
//         PUT(FTRP(bp), PACK(asize, 1));
//         bp = NEXT_BLKP(bp);
//         PUT(HDRP(bp), PACK(original_size-asize, 0));
//         PUT(FTRP(bp), PACK(original_size-asize, 0));
//     }
//     else
//     {
//         PUT(HDRP(bp), PACK(original_size, 1));
//         PUT(FTRP(bp), PACK(original_size, 1));
//     }
// }


/*
 * coalesce 
 */

static void *coalesce(void *bp)
{
    size_t next_alloc;
    bp = (char*)bp + 4;
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); /* 0 or 1 */
    if (IS_END(bp - 4))
    {
        next_alloc = 1;
    }
    else
        next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        return (char*)bp-4;
    }

    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        rbtree_erase((char*)NEXT_BLKP(bp) - 4);
    }

    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        rbtree_erase((char*)NEXT_BLKP(bp) - 4);
        bp = PREV_BLKP(bp);
    }

    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return (char*)bp-4;
}

/*
 * extend_heap - When (1) initiate heap or, (2) malloc cannot find fit free blocks
                 Extend heap to make new free blocks
 */

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    /* allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; /* 조건식 ? 반환값1(true):반환값2(false) */
    if ((long)(bp = mem_sbrk(size)-4) == -1) /* bp는 old_brk 포인터. */
        return NULL;
    end_point += size;
    if (end_2 ==0)
    {
        /* initialize free block header,footer and the epilogue header */
        PUT(bp, PACK(size, 0));
        PUT(FTRP(bp+4), PACK(size, 0));
        // PUT(HDRP(NEXT_BLKP(bp+4)), PACK(0, 1)); /* new epilogue header */
        /* coalesce if the previous block was free */
        bp = coalesce(bp);
    }
    else
    {
        bp -= 16;
        make_hf(bp, (size / 8) + 2, 0);
        rbtree_erase(bp);
        end_2 = 0;
    }
    return bp;
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) /* word 4개짜리만큼 brk 옮기는 작업*/
        return -1;
    PUT(heap_listp, 0);                         /* Alignment Padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1)); /* Prologues footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += 2*WSIZE; /* heap의 시작점에서 2word 뒤에 둠. 항상 prologue block 중앙에 위치. */
    root = (unsigned int)(nil);
    start_point = heap_listp + 2 * WSIZE;
    end_point = start_point;

    /*extend the empty heap with a free block of CHUNKSIZE bytes*/
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

void* find_fit(unsigned int size)
{
    void* finder = NULL;
    void* prefinder = root;
    while ((unsigned long*)prefinder != nil)
    {
        if (NUM_BLOCK(prefinder) == size)
            return prefinder;
        else if (NUM_BLOCK(prefinder) > size)
        {
            finder = prefinder;
            prefinder = child_L_PTR(prefinder);
        }
        else
            prefinder = child_R_PTR(prefinder);
    }
    return finder;
}
void make_hf(unsigned* make_point, unsigned num_block, unsigned allocated)
{
    *make_point = num_block * 8 + ALLOC_BIT*allocated;
    *(make_point + num_block * 2 - 1) = num_block * 8 + ALLOC_BIT*allocated;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    unsigned int asize;
    size_t extendsize; /* fit 한 free block없으면 extend_heap 호출 */
    char *bp;
    unsigned abs_size;
    char* new_free;
    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2;
    else
        asize = (unsigned int)((DSIZE + DSIZE * ((size + DSIZE - 1) / DSIZE))/8);
    /* searching FIT free block */
    if ( (bp = find_fit(asize)) != NULL )
    {
        abs_size = NUM_BLOCK(bp) - asize;
        // 0. rbtree에서 해당 노드 삭제
        rbtree_erase(bp);
        // 1. asize == 2
        // end check 아니면 뒤에 블럭에 표시
        new_free = bp + abs_size *8;
        if (NUM_BLOCK(bp) == 2)
        {
            if (IS_END(bp) == 0)
            {
                two_block_demark(bp);
            }
            else
                end_2 = 0;
        }
        // inserting_bp 생성
        // 2. 할당 후 블록의 남는 길이 보고 노선 3개
        // 2-1. 길이 3이상 노선
            // 2-1-1. RBheader footer 생성
            // 2-1-2. 할당
            // 2-1-3. 새로운 노드 포인터 rbtree에 추가
        // 2-2. 길이 2 노선
            //2-2-1. RBheader 생성
            //2-2-2. free블록의 뒤에 header에 2BFREE수정 클리어
            //2-2-3. RBtree에 넣기
        // 2-3. 길이 1 노선
            //header 추가(RBtree 정보 x)
        // *bp &= ~ALLOC_BIT;
        else if (abs_size == 1)
        {
            make_hf(new_free, 1, 0);
        }
        else if (abs_size >= 2)
        {
            if (NUM_BLOCK(bp) - asize > 2)
            {
                make_hf(new_free, abs_size, 0);
            }
            else
            {
                *(unsigned*)new_free = abs_size * 8;
                if (IS_END(new_free) == 0)
                    two_block_mark(new_free);
                else
                    end_2 = 1;
            }
            rbtree_insert(abs_size, new_free);
        }
        make_hf(bp, asize,1);
        return bp+4;
    }

    /* NO Fit found. Extend memory and place the block*/
    extendsize = MAX(asize, CHUNKSIZE);
    if ( (bp = extend_heap(extendsize/WSIZE)) == NULL )
        return NULL;
    make_hf(bp + asize * 8, NUM_BLOCK(bp) - asize, 0);
    make_hf(bp, asize, 1);
    rbtree_erase(bp);
    rbtree_insert(NUM_BLOCK(bp) - asize, bp + asize * 8);
    return bp + 4;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    bp = (char*)bp - 4;
    if (NUM_BLOCK(bp) == 2)
    {
        if (IS_END(bp))
            end_2 = 1;
        else
            two_block_mark(bp);
    }
    *(unsigned*)bp = (*(unsigned*)bp) & ~(ALLOC_BIT + TWO_BLOCK_BIT);
    *(unsigned*)(bp + BLOCK_SIZE(bp) - 4) = *(unsigned*)bp;
    // size_t size = GET_SIZE(HDRP(bp));
    // PUT(HDRP(bp), PACK(size, 0));
    // PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr));

    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    
    return newptr;
}





// rbtree

void change_to_black(unsigned* bp)
{
    *bp = (*bp) || COLOR_BIT;
}
void change_to_red(unsigned* bp)
{
    *bp = (*bp) & ~COLOR_BIT;
}
void change_color(unsigned* bp,unsigned* from)
{
    if (GET_COLOR(from))
        change_to_black(bp);
    else change_to_red(bp);
}


static void left_rotate(void* ex_parent)
{
    // 중요한건 참조를 위한 변수수정 순서다
    // 정보 중 필요한 것을 늦게 수정하는것
    void* ex_son = child_R_PTR(ex_parent);
    *child_R(ex_parent) = child_L_PTR(ex_son);
    if (child_L_PTR(ex_son) != nil)
        *PARENT(child_L_PTR(ex_son)) = ex_parent;
    *PARENT(ex_son) = PARENT_PTR(ex_parent);
    if (PARENT_PTR(ex_parent) == nil)
    {
        root = ex_son;
    }
    else if (ex_parent == child_R_PTR(PARENT_PTR(ex_parent)))
    {
        *child_R(PARENT_PTR(ex_parent)) = ex_son;
    }
    else *child_L(PARENT_PTR(ex_parent)) = ex_son;
    *PARENT(ex_parent) = ex_son;
    *child_L(ex_son) = ex_parent;
}
static void right_rotate(void* ex_parent)
{
    // 중요한건 참조를 위한 변수수정 순서다
    // 정보 중 필요한 것을 늦게 수정하는것
    void* ex_son = child_L_PTR(ex_parent);
    *child_L(ex_parent) = child_R_PTR(ex_son);
    if (child_R_PTR(ex_son) != nil)
        *PARENT(child_R_PTR(ex_son)) = ex_parent;
    *PARENT(ex_son) = PARENT_PTR(ex_parent);
    if (PARENT_PTR(ex_parent) == nil)
    {
        root = ex_son;
    }
    else if (ex_parent == child_L_PTR(PARENT_PTR(ex_parent)))
    {
        *child_L(PARENT_PTR(ex_parent)) = ex_son;
    }
    else *child_R(PARENT_PTR(ex_parent)) = ex_son;
    *PARENT(ex_parent) = ex_son;
    *child_R(ex_son) = ex_parent;
}

static void insert_fixup(void* trouble)
{
    while (GET_COLOR(PARENT_PTR(trouble)) == RED)
    {
        // 아빠가 우자식일때
        if (child_R_PTR(PARENT_PTR(PARENT_PTR(trouble))) == PARENT_PTR(trouble))
        {
            // 삼촌도 red일 경우!
            // 문제를 전세대로 유기
            if (GET_COLOR(child_L_PTR(PARENT_PTR(PARENT_PTR(trouble)))) == RED)
            {
                change_to_black(child_L_PTR(PARENT_PTR(PARENT_PTR(trouble))));
                change_to_black(PARENT_PTR(trouble));
                change_to_red(PARENT_PTR(PARENT_PTR(trouble)));
                trouble = PARENT_PTR(PARENT_PTR(trouble));
            }
            // 삼촌이 red가 아닐경우!
            // 회전 한번으로 trouble부터 할아버지까지 일렬로 만들고
            // 스왑 회전
            // 일렬로 만드는 이유 생각하기
            else
            {
                // 일자가 아니라면
                if (trouble == child_L_PTR(PARENT_PTR(trouble)))
                {
                    trouble = PARENT_PTR(trouble);
                    right_rotate(trouble);
                }
                change_to_black(PARENT_PTR(trouble));
                change_to_red(PARENT_PTR(PARENT_PTR(trouble)));
                left_rotate(PARENT_PTR(PARENT_PTR(trouble)));
            }
        }
        // 아빠가 좌자식일때
        else
        {
            // 삼촌도 red일 경우!
            // 문제를 전세대로 유기
            if (GET_COLOR(child_R_PTR(PARENT_PTR(PARENT_PTR(trouble)))) == RED)
            {
                change_to_black(child_R_PTR(PARENT_PTR(PARENT_PTR(trouble))));
                change_to_black(PARENT_PTR(trouble));
                change_to_red(PARENT_PTR(PARENT_PTR(trouble)));
                trouble = PARENT_PTR(PARENT_PTR(trouble));
            }
            // 삼촌이 red가 아닐경우!
            // 회전 한번으로 trouble부터 할아버지까지 일렬로 만들고
            // 스왑 회전
            // 일렬로 만드는 이유 생각하기
            else
            {
                // 일자가 아니라면
                if (trouble == child_R_PTR(PARENT_PTR(trouble)))
                {
                    trouble = PARENT_PTR(trouble);
                    left_rotate(trouble);
                }
                change_to_black(PARENT_PTR(trouble));
                change_to_red(PARENT_PTR(PARENT_PTR(trouble)));
                right_rotate(PARENT_PTR(PARENT_PTR(trouble)));
            }
        }
    }
    change_to_black((void*)root);
}

void rbtree_insert(const unsigned int inserting_key, void* inserting)
{
    
    // 새로 들어갈 노드의 기본설정(끝단 설정, red, key)
    *child_L(inserting)= nil;
    *child_R(inserting)= nil;
    change_to_black(inserting);
    
    // inserting_point : 실재 들어갈 위치
    // inserting_parent : 들어갈 위치의 부모
    void* inserting_point = root;
    void* inserting_parent = nil ;
    
    // inserting_parent를 구하는 작업
    // while 문에 진입하지 못한다면 그것은 inserting이 root가 된다는 것
    // 왜 변수를 두개 선언하여 조사해야하는지 생각해보자
    // 자신이 끝단 노드인지 확인하는것이 까다롭다!
    while (inserting_point != nil)
    {
        inserting_parent = inserting_point;
        if (NUM_BLOCK(inserting_point) > inserting_key)
            inserting_point = child_L_PTR(inserting_point);
        // 이 경우에 key 값이 같은 node를 만나는 경우도 포함됨을 의식하자
        else inserting_point = child_R_PTR(inserting_point);
    }
    
    // inserting의 관점에서 부모를 설정한다.
    *PARENT(inserting) = inserting_parent;

    // inserting의 부모가 될 사람 입장에서 inserting을 자식설정
    if (inserting_parent == nil)
    {
        root = inserting;
    }
    else if (NUM_BLOCK(inserting_parent) > inserting_key)
    {
        *child_L(inserting_parent) = inserting;
    }
    else
        *child_R(inserting_parent) = inserting;
    insert_fixup(inserting);
}






// sub_root를 root로 하는 서브 트리의 최솟값을 갖는 노드의 포인터 리턴,
// succesor을 찾는 함수에서 호출
static void* subtree_min(void* sub_root)
{
    while (child_L_PTR(sub_root) != nil)
    {
        sub_root = child_L_PTR(sub_root);
    }
    return sub_root;
}


// 특정 노드 위치에 부분트리 끼우기
// delete에서 삭제노드의 자식이 2일때 x의 부분드리를 올려주는 과정에서 사용
static void RB_transplant(void* u, void* v)
{
	// (u의 부모)의 자식을 v로 변경
    if (PARENT_PTR(u) == nil)
        root = v;
    else if (child_L_PTR(PARENT_PTR(u)) == u)
        *child_L(PARENT_PTR(u)) = v;
    else *child_R(PARENT_PTR(u)) = v;
	// v의 부모를 (u의 부모)로 변경
	*PARENT(v) = PARENT_PTR(u);
}

static void del_fixup(void* gray)
{
    // case 1과 case 2-1을 거른다
    while (gray != (void*)root && GET_COLOR(gray) == BLACK)
    {
        // gray가 좌자식일때, 우자식일때를 나눈다
        // gray가 좌자식일때 시작
        if (gray == child_L_PTR(PARENT_PTR(gray)))
        {
            void* bro = child_R_PTR(PARENT_PTR(gray));
            // case 2-2-1 : 스왑회전을 통해 문제를 남겨둔체 2-2-2로 연결
            if (GET_COLOR(bro) == RED)
            {
                // bro가 red면 gray -> parent 는 black
                change_to_black(bro);
                change_to_red(PARENT_PTR(gray));
                left_rotate(PARENT_PTR(gray));
                bro = child_R_PTR(PARENT_PTR(gray));
            }
            // case 2-2-2 : 다음세대로 문제를 넘길지 말지 정함 진입 시 이전 세대로 문제(gray) 옮긴다
            // 진입 조건 : bro, bro의 자식들이 모두 black인 경우 실행
            // bro의 색을 red로 바꿔 bro의 부분트리의 bh를 1 낮추고 결국 gray -> parent의 부분트리의 bh를 1낮춘다
            // gray의 개념을 살려 gray -> parent에게 black을 하나 위에 올린다는 의미를 담아 gray=gray ->parent 해준다
            // 루프 다시 돌린다.
            if (GET_COLOR(child_R_PTR(bro)) == BLACK && GET_COLOR(child_L_PTR(bro)) == BLACK)
            {
                change_to_red(bro);
                gray = PARENT_PTR(gray);
                continue;
            }
            // case 2-2-3 : 2-2-4를 위한 밑간 작업 핵심은 bro->right를 red로 만드는 것이다(2-2-4진입조건)
            // bro->right이 black이라면 위의 거름막에 의해 bro -> left == red;
            // 사용 원리는 bro의 스왑회전으로 red인 자식을 올려주는 것이다
            // 여기까지 왔다면 while문 더 이상 안돈다
            if (GET_COLOR(child_R_PTR(bro)) == BLACK)
            {
                change_to_red(bro);
                change_to_black(child_L_PTR(bro));
                right_rotate(bro);
                bro = child_R_PTR(PARENT_PTR(gray));
            }
            // case 2-2-4 : 종료
            // 조건 : gray -> color == black, bro->color == black, bro->right->color == red
            // 원리 : 회전을 이해하는 것이 중요하다
            // gray->parent를 기준으로 left_rotate한다 => gray문제 해결, bro->left 변화 없음, bro->right는 black이 올려짐(이론상 gray)
            // bro -> right가 case 1의 gray가 된다 따라서 bro->right의 색을 black으로 바꿔준다.
            change_color(bro, PARENT_PTR(gray));
            change_to_black(PARENT_PTR(gray));
            change_to_black(child_R_PTR(bro));
            left_rotate(PARENT_PTR(gray));
            gray = (void*)root; // 이게 좀 참신한데 맨 아래줄에서 영향을 받지 않기 위해 root로 초기화 그냥 리턴해도 되긴 할듯
        }
        // 좌자식 끝 우자식 시작
        else
        {
            void* bro = child_L_PTR(PARENT_PTR(gray));
            // case 2-2-1 : 스왑회전을 통해 문제를 남겨둔체 2-2-2로 연결
            if (GET_COLOR(bro) == RED)
            {
                // bro가 red면 gray -> parent 는 black
                change_to_black(bro);
                change_to_red(PARENT_PTR(gray));
                right_rotate(PARENT_PTR(gray));
                bro = child_L_PTR(PARENT_PTR(gray));
            }
            // case 2-2-2 : 다음세대로 문제를 넘길지 말지 정함 진입 시 이전 세대로 문제(gray) 옮긴다
            // 진입 조건 : bro, bro의 자식들이 모두 black인 경우 실행
            // bro의 색을 red로 바꿔 bro의 부분트리의 bh를 1 낮추고 결국 gray -> parent의 부분트리의 bh를 1낮춘다
            // gray의 개념을 살려 gray -> parent에게 black을 하나 위에 올린다는 의미를 담아 gray=gray ->parent 해준다
            // 루프 다시 돌린다.
            if (GET_COLOR(child_L_PTR(bro)) == BLACK && GET_COLOR(child_R_PTR(bro)) == BLACK)
            {
                change_to_red(bro);
                gray = PARENT_PTR(gray);
                continue;
            }
            // case 2-2-3 : 2-2-4를 위한 밑간 작업 핵심은 bro->right를 red로 만드는 것이다(2-2-4진입조건)
            // bro->right이 black이라면 위의 거름막에 의해 bro -> left == red;
            // 사용 원리는 bro의 스왑회전으로 red인 자식을 올려주는 것이다
            // 여기까지 왔다면 while문 더 이상 안돈다
            if (GET_COLOR(child_L_PTR(bro)) == BLACK)
            {
                change_to_red(bro);
                change_to_black(child_R_PTR(bro));
                left_rotate(bro);
                bro = child_L_PTR(PARENT_PTR(gray));
            }
            // case 2-2-4 : 종료
            // 조건 : gray -> color == black, bro->color == black, bro->right->color == red
            // 원리 : 회전을 이해하는 것이 중요하다
            // gray->parent를 기준으로 left_rotate한다 => gray문제 해결, bro->left 변화 없음, bro->right는 black이 올려짐(이론상 gray)
            // bro -> right가 case 1의 gray가 된다 따라서 bro->right의 색을 black으로 바꿔준다.
            change_color(bro, PARENT_PTR(gray));
            change_to_black(PARENT_PTR(gray));
            change_to_black(child_L_PTR(bro));
            right_rotate(PARENT_PTR(gray));
            gray = (void*)root; // 이게 좀 참신한데 맨 아래줄에서 영향을 받지 않기 위해 root로 초기화 그냥 리턴해도 되긴 할듯
        }
    }
    change_to_black(gray);
}


// 삭제 
void rbtree_erase(void* deleting)
{
    void* damaged;
    void* color_loser = deleting;
    unsigned origin_color = GET_COLOR(color_loser);

    //편자녀 case 분리
    if (child_L_PTR(deleting) == nil)
    {
        damaged = child_R_PTR(deleting);
        
        RB_transplant(deleting, damaged);
    }
    else if (child_R_PTR(deleting) == nil)
    {
        damaged = child_L_PTR(deleting);
        
        RB_transplant(deleting, damaged);
    }
	// 편자녀구간 끝
    else
    {
		//color_loser == deleting의 직후노드
        color_loser = subtree_min(child_R_PTR(deleting));
        
        origin_color = GET_COLOR(color_loser);
        damaged = child_R_PTR(color_loser);
		// color_loser의 부모가 deleting이 아닐때 작업구간
        // color_loser의 자식 damaged 위치 변경,color_loser의 우자식 설정
        if (PARENT_PTR(color_loser) == deleting)
        {
            *PARENT(damaged) = color_loser;
        }
        else
        {
            // x==y->right
            // damaged를 color_loser의 부모와 연결한다. 
            // 이때 color_loser의 구조체는 건들지 않는다.
            RB_transplant(color_loser, child_R_PTR(color_loser));
            
            // deleting의 오른자식과 color_loswer을 연결한다.
            // 왜 Transplant 안쓰지? 못쓰나? if문 피할라고?
            *PARENT(child_R_PTR(deleting)) = color_loser; // 
            *child_R(color_loser) = child_R_PTR(deleting);
        }
		// color_loser의 부모가 deleting이 아닐때 작업구간 끝

        // y(color_loser) deleting의 부모와 연결
        RB_transplant(deleting, color_loser);
        // y(color_loser) deleting의 왼자식과 연결
        *child_L(color_loser) = child_L_PTR(deleting);
        *PARENT(child_L_PTR(deleting)) = color_loser;
        // color_loser가 deleting의 색을 상속
        change_color(color_loser, deleting);
    }
    if (origin_color == BLACK)
        del_fixup(damaged);
}





























