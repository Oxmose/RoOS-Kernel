/*******************************************************************************
 * @file iofunc.c
 *
 * @see unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief IO function familly for the roOs kernel.
 *
 * @details IO functions familly for the roOs kernel. Those functions might
 * rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <Libcinternal.h>

/* Header file */
#include <stdlib.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Num size. */
#define NUM_SIZES 32

/** @brief Memory chunk alignement. */
#define ALIGN 8

/** @brief S_Chunk minimal size. */
#define MIN_SIZE sizeof(S_List)

/** @brief Header size. */
#define HEADER_SIZE __builtin_offsetof(S_Chunk, pData)

/** @brief Defines the size of the inital heap in bytes */
#define LIBC_MALLOC_INIT_SIZE 0x100000

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

 /** @brief User's heap allocator list node. */
typedef struct List
{
  /** @brief Next node of the list. */
  struct List* pNext;
  /** @brief Previous node of the list. */
  struct List* pPrev;
} S_List;

/** @brief User's heap allocator memory chunk representation. */
typedef struct
{
  /** @brief Memory chunk list. */
  S_List all;

  /** @brief Used flag. */
  int32_t used;

  /**
   * @brief If used, the union contains the chunk's data, else a list of free
   * memory.
   */
  union
  {
    uint8_t pData[0];
    S_List  free;
  };
} S_Chunk;

/** @brief Defines the information for a process heap. */
typedef struct
{
  /** @brief The base address of the heap. */
  uintptr_t baseAddress;
  /** @brief The end address of the heap. */
  uintptr_t endAddress;

  /** @brief User's heap free memory chunks. */
  S_Chunk* spFreeChunk[NUM_SIZES];
  /** @brief User's heap spFirstChunk memory chunk. */
  S_Chunk* spFirstChunk;
  /** @brief User's heap spLastChunk memory chunk. */
  S_Chunk* spLastChunk;

  /** @brief Quantity of free memory in the user's heap. */
  size_t sMemFree;
  /** @brief Quantity of used memory in the user's heap. */
  size_t sMemUsed;
  /** @brief Quantity of memory used to store meta data in the user's heap. */
  size_t sMemMeta;
} S_ProcessHeap;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/** @brief Get the container */
#define CONTAINER(C, l, v) ((C*)(((char*)v) - (uintptr_t)&(((C*)0)->l)))

/** @brief Initializes the list */
#define LIST_INIT(v, l) _ListInit(&v->l)

/** @brief Remove element from the list */
#define LIST_REMOVE_FROM(h, d, l)               \
{                                               \
  __typeof__(**h) **h_ = h, *d_ = d;            \
  S_List* head = &(*h_)->l;                     \
  _RemoveFrom(&head, &d_->l);                   \
  if (head == NULL)                             \
  {                                             \
    *h_ = NULL;                                 \
  }                                             \
  else                                          \
  {                                             \
    *h_ = CONTAINER(__typeof__(**h), l, head);  \
  }                                             \
}

/** @brief Remove element to the list */
#define LIST_PUSH(h, v, l)                  \
{                                           \
  __typeof__(*v) **h_ = h, *v_ = v;         \
  S_List* head = &(*h_)->l;                 \
  if (*h_ == NULL)                          \
  {                                         \
    head = NULL;                            \
  }                                         \
  _Push(&head, &v_->l);                     \
  *h_ = CONTAINER(__typeof__(*v), l, head); \
}

/** @brief Pop element from the list */
#define LIST_POP(h, l)                             \
__extension__                                      \
({                                                 \
  __typeof__(**h) **h_ = h;                        \
  S_List* head = &(*h_)->l;                        \
  S_List* res = _Pop(&head);                       \
  if (head == NULL)                                \
  {                                                \
    *h_ = NULL;                                    \
  }                                                \
  else                                             \
  {                                                \
    *h_ = CONTAINER(__typeof__(**h), l, head);     \
  }                                                \
  CONTAINER(__typeof__(**h), l, res);              \
})

/** @brief Get spFirstChunk iterator of a list */
#define LIST_ITERATOR_BEGIN(h, l, it)                               \
{                                                                   \
  __typeof__(*h) *h_ = h;                                           \
  S_List* last_##it = h_->l.prev, *iter_##it = &h_->l, *next_##it;  \
  do                                                                \
  {                                                                 \
    if (iter_##it == last_##it)                                     \
    {                                                               \
      next_##it = NULL;                                             \
    }                                                               \
    else                                                            \
    {                                                               \
      next_##it = iter_##it->pNext;                                 \
    }                                                               \
    __typeof__(*h)* it = CONTAINER(__typeof__(*h), l, iter_##it);

/** @brief Get end iterator of a list */
#define LIST_ITERATOR_END(it)       \
  }while ((iter_##it = next_##it));  \
}


/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief The thread-local current heap variable. */
static S_ProcessHeap* spCurrentHeap = NULL;

/** @brief The current heap's locking mechanism. */
static T_Spinlock sCurrentHeapLock = SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Initialize the process heap.
 *
 * @details Initialize the process heap. This function is called when the
 * process heap is first used. It allocates the heap memory and sets up the
 * heap structure.
 *
 * @return A pointer to the initialized process heap is returned. If the heap
 * cannot be initialized, NULL is returned.
*/
static S_ProcessHeap* _InitializeProcessHeap(void);

/**
 * @brief Initializes the memory list.
 *
 * @details Initializes the memory list with the basic node value.
 *
 * @param[out] pNode The list's node to initialize.
 */
static inline void _ListInit(S_List* pNode);

/**
 * @brief Inserts a node before the current node in the list.
 *
 * @details Inserts a node before the current node in the list.
 *
 * @param[in,out] pCurrent The current node.
 * @param[in,out] pNew The new node to insert before the current node.
 */
static inline void _InsertBefore(S_List* pCurrent, S_List* pNew);

/**
 * @brief Inserts a node after the current node in the list.
 *
 * @param[in,out] pCurrent The current node.
 * @param[in,out] pNew The new node to insert after the current node.
 */
static inline void _InsertAfter(S_List* pCurrent, S_List* pNew);

/**
 * @brief Removes a node from the list.
 *
 * @details Removes a node from the list.
 *
 * @param[out] pNode The node to remove from the list.
 */
static inline void _Remove(S_List* pNode);

/**
 * @brief Pushes a node at the end of the list.
 *
 * @details Pushes a node at the end of the list.
 *
 * @param[out] ppList The list to be pushed.
 * @param[in] pNode The node to push to the list.
 */
static inline void _Push(S_List** ppList, S_List* pNode);

/**
 * @brief Pops a node from the list.
 *
 * @details Pops a node from the list and returns it.
 *
 * @param[out] ppList The list to be poped from.
 *
 * @return The node poped from the list is returned.
 */
static inline S_List* _Pop(S_List** ppList);

/**
 * @brief Removes a node from the list.
 *
 * @details Removes a node from the list.
 *
 * @param[out] ppList The list to remove the node from.
 * @param[out] pNode The node to remove from the list.
 */
static inline void _RemoveFrom(S_List** ppList, S_List* pNode);

/**
 * @brief Initializes a memory chunk structure.
 *
 * @details Initializes a memory chunk structure.
 *
 * @param[out] pChunk The chunk structure to initialize.
 */
static inline void _MemoryChunkInit(S_Chunk* pChunk);

/**
 * @brief Returns the size of a memory chunk.
 *
 * @param pChunk The chunk to get the size of.
 *
 * @return The size of the memory chunk is returned.
 */
static inline uint32_t _MemoryChunkSize(const S_Chunk* pChunk);

/**
 * @brief Returns the slot of a memory chunk for the desired size.
 *
 * @details Returns the slot of a memory chunk for the desired size.
 *
 * @param[in] size The size of the chunk to get the slot of.
 *
 * @return The slot of a memory chunk for the desired size.
 */
static inline int32_t _MemoryChunkSlot(uint32_t size);

/**
 * @brief Removes a memory chunk in the free memory chunks list.
 *
 * @details Removes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] pChunk The chunk to be removed from the list.
 * @param[in, out] pHeap The process heap containing the free chunk list.
 */
static inline void _RemoveFree(S_Chunk* pChunk, S_ProcessHeap* pHeap);

/**
 * @brief Pushes a memory chunk in the free memory chunks list.
 *
 * @details Pushes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] pChunk The chunk to be placed in the list.
 * @param[in, out] pHeap The process heap containing the free chunk list.
 */
static inline void _PushFree(S_Chunk *pChunk, S_ProcessHeap* pHeap);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static S_ProcessHeap* _InitializeProcessHeap(void)
{
  S_Chunk*       pSecond;
  uint32_t       len;
  int32_t        n;
  void*          pMem;
  uint32_t       size;
  int8_t*        pMemStart;
  int8_t*        pMemEnd;
  S_ProcessHeap* pHeap;

  /* Request memory blocks */
  pMem = mmap(NULL,
              LIBC_MALLOC_INIT_SIZE,
              PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS,
              -1,
              0);
  if (pMem != MAP_FAILED)
  {
    /* Allocate internal structures */
    size               = LIBC_MALLOC_INIT_SIZE;
    pHeap              = (S_ProcessHeap*)pMem;
    pHeap->baseAddress = (uintptr_t)pMem;
    pHeap->endAddress  = (uintptr_t)pMem + size;
    pMem               += sizeof(S_ProcessHeap);
    size               -= sizeof(S_ProcessHeap);

    /* Get actual memory bounds */
    pMemStart = (int8_t*)(((uintptr_t)pMem + ALIGN - 1) & (~(ALIGN - 1)));
    pMemEnd   = (int8_t*)(((uintptr_t)pMem + size) & (~(ALIGN - 1)));
    pHeap->sMemUsed     = 0;
    pHeap->sMemFree     = 0;
    pHeap->sMemMeta     = 0;

    pHeap->spFirstChunk = (S_Chunk*)pMemStart;
    pHeap->spLastChunk  = ((S_Chunk*)pMemEnd) - 1;
    pSecond             = pHeap->spFirstChunk + 1;

    _MemoryChunkInit(pHeap->spFirstChunk);
    _MemoryChunkInit(pSecond);
    _MemoryChunkInit(pHeap->spLastChunk);

    _InsertAfter(&pHeap->spFirstChunk->all, &pSecond->all);
    _InsertAfter(&pSecond->all, &pHeap->spLastChunk->all);

    pHeap->spFirstChunk->used = true;
    pHeap->spLastChunk->used  = true;

    len = _MemoryChunkSize(pSecond);
    n   = _MemoryChunkSlot(len);

    LIST_PUSH(&pHeap->spFreeChunk[n], pSecond, free);
    pHeap->sMemFree = len - HEADER_SIZE;
    pHeap->sMemMeta = sizeof(S_Chunk) * 2 + HEADER_SIZE;
  }
  else
  {
    pHeap = NULL;
  }

  return pHeap;
}

static inline void _ListInit(S_List* pNode)
{
  pNode->pNext = pNode;
  pNode->pPrev = pNode;
}

static inline void _InsertBefore(S_List* pCurrent, S_List* pNew)
{
  S_List* pCurrentPrev = pCurrent->pPrev;
  S_List* pNewPrev     = pNew->pPrev;

  pCurrentPrev->pNext = pNew;
  pNew->pPrev         = pCurrentPrev;
  pNewPrev->pNext     = pCurrent;
  pCurrent->pPrev     = pNewPrev;
}

static inline void _InsertAfter(S_List* pCurrent, S_List* pNew)
{
  S_List* pCurrentNext = pCurrent->pNext;
  S_List* pNewPrev     = pNew->pPrev;

  pCurrent->pNext     = pNew;
  pNew->pPrev         = pCurrent;
  pNewPrev->pNext     = pCurrentNext;
  pCurrentNext->pPrev = pNewPrev;
}

static inline void _Remove(S_List* pNode)
{
  pNode->pPrev->pNext = pNode->pNext;
  pNode->pNext->pPrev = pNode->pPrev;

  pNode->pNext = pNode;
  pNode->pPrev = pNode;
}

static inline void _Push(S_List** ppList, S_List* pNode)
{
  if (*ppList != NULL)
  {
    _InsertBefore(*ppList, pNode);
  }

  *ppList = pNode;
}

static inline S_List* _Pop(S_List** ppList)
{

  S_List* top = *ppList;
  S_List* nextTop = top->pNext;

  _Remove(top);

  if (top == nextTop)
  {
    *ppList = NULL;
  }
  else
  {
    *ppList = nextTop;
  }

  return top;
}

static inline void _RemoveFrom(S_List** ppList, S_List* pNode)
{
  if (*ppList == pNode)
  {
    _Pop(ppList);
  }
  else
  {
    _Remove(pNode);
  }
}

static inline void _MemoryChunkInit(S_Chunk* pChunk)
{
  LIST_INIT(pChunk, all);
  pChunk->used = false;
  LIST_INIT(pChunk, free);
}

static inline uint32_t _MemoryChunkSize(const S_Chunk* pChunk)
{
  return ((int8_t*)(pChunk->all.pNext) - (int8_t*)(&pChunk->all)) - HEADER_SIZE;
}

static inline int32_t _MemoryChunkSlot(uint32_t size)
{
  int32_t n = -1;

  while (size > 0)
  {
    ++n;
    size /= 2;
  }
  return n;
}

static inline void _RemoveFree(S_Chunk* pChunk, S_ProcessHeap* pHeap)
{
  uint32_t len = _MemoryChunkSize(pChunk);
  int      n   = _MemoryChunkSlot(len);

  LIST_REMOVE_FROM(&pHeap->spFreeChunk[n], pChunk, free);
  pHeap->sMemFree -= len;
}

static inline void _PushFree(S_Chunk *pChunk, S_ProcessHeap* pHeap)
{
  uint32_t len = _MemoryChunkSize(pChunk);
  int      n   = _MemoryChunkSlot(len);

  LIST_PUSH(&pHeap->spFreeChunk[n], pChunk, free);
  pHeap->sMemFree += len;
}

void* malloc(size_t size)
{
  size_t    n;
  S_Chunk*  pChunk;
  S_Chunk*  pChunk2;
  size_t    size2;
  size_t    len;
  void*     allocated;
  size_t    alignLeft;

  LIBC_SPINLOCK_LOCK(sCurrentHeapLock);

  if (spCurrentHeap != NULL)
  {
    if (size != 0)
    {
      /* Ensure to stay aligned */
      alignLeft = (HEADER_SIZE + size) & (ALIGN - 1);
      if (alignLeft != 0)
      {
        size = size + ALIGN - alignLeft;
      }

      if (size < MIN_SIZE)
      {
        size = MIN_SIZE;
      }

      n = _MemoryChunkSlot(size - 1) + 1;

      if (n < NUM_SIZES)
      {
        while (spCurrentHeap->spFreeChunk[n] == 0)
        {
          ++n;
          if (n >= NUM_SIZES)
          {
            allocated = NULL;
            break;
          }
        }

        if (n < NUM_SIZES)
        {
          pChunk = LIST_POP(&spCurrentHeap->spFreeChunk[n], free);

          size2 = _MemoryChunkSize(pChunk);
          len = 0;

          if (size + sizeof(S_Chunk) <= size2)
          {
            pChunk2 = (S_Chunk*)(((int8_t*)pChunk) + HEADER_SIZE + size);

            _MemoryChunkInit(pChunk2);
            _InsertAfter(&pChunk->all, &pChunk2->all);

            len = _MemoryChunkSize(pChunk2);
            n   = _MemoryChunkSlot(len);

            LIST_PUSH(&spCurrentHeap->spFreeChunk[n], pChunk2, free);

            spCurrentHeap->sMemMeta += HEADER_SIZE;
            spCurrentHeap->sMemFree += len;
          }

          pChunk->used = true;

          spCurrentHeap->sMemFree -= size2;
          spCurrentHeap->sMemUsed += size2 - len - HEADER_SIZE;

          allocated = (void*)pChunk->pData;
        }
      }
      else
      {
        allocated = NULL;
      }
    }
    else
    {
      allocated = NULL;
    }
    LIBC_SPINLOCK_UNLOCK(sCurrentHeapLock);
  }
  else
  {
    spCurrentHeap = _InitializeProcessHeap();
    if (spCurrentHeap == NULL)
    {
      LIBC_SPINLOCK_UNLOCK(sCurrentHeapLock);
      errno     = ENOMEM;
      allocated = NULL;
    }
    else
    {
      LIBC_SPINLOCK_UNLOCK(sCurrentHeapLock);
      allocated = malloc(size);
    }
  }


  return allocated;
}

void free(void* ptr)
{
  S_Chunk* pChunk;
  S_Chunk* pNext;
  S_Chunk* pPrev;

  LIBC_SPINLOCK_LOCK(sCurrentHeapLock);
  if (spCurrentHeap != NULL)
  {
    pChunk = (S_Chunk*)((int8_t*)ptr - HEADER_SIZE);

    pNext = CONTAINER(S_Chunk, all, pChunk->all.pNext);
    pPrev = CONTAINER(S_Chunk, all, pChunk->all.pPrev);

    spCurrentHeap->sMemUsed -= _MemoryChunkSize(pChunk);

    if (pNext->used == false)
    {
      _RemoveFree(pNext, spCurrentHeap);
      _Remove(&pNext->all);

      spCurrentHeap->sMemMeta -= HEADER_SIZE;
      spCurrentHeap->sMemFree += HEADER_SIZE;
    }

    if (pPrev->used == false)
    {
      _RemoveFree(pPrev, spCurrentHeap);
      _Remove(&pChunk->all);

      _PushFree(pPrev, spCurrentHeap);
      spCurrentHeap->sMemMeta -= HEADER_SIZE;
      spCurrentHeap->sMemFree += HEADER_SIZE;
    }
    else
    {
      pChunk->used = false;
      LIST_INIT(pChunk, free);
      _PushFree(pChunk, spCurrentHeap);
    }
  }
  LIBC_SPINLOCK_UNLOCK(sCurrentHeapLock);
}

/************************************ EOF *************************************/