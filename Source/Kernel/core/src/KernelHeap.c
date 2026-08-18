/*******************************************************************************
 * @file KernelHeap.c
 *
 * @see KernelHeap.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief Kernel's heap allocator.
 *
 * @details Kernel's heap allocator. Allow to dynamically allocate and dealocate
 * memory on the kernel's heap.
 *
 * @warning This allocator is not suited to allocate memory for the process, you
 * should only use it for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <Panic.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <Critical.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* None TODO */

/* Header file */
#include <KernelHeap.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Module name */
#define MODULE_NAME "KHEAP"

/** @brief Num size. */
#define NUM_SIZES 32

/** @brief Memory chunk alignement. */
#define ALIGN 4

/** @brief S_Chunk minimal size. */
#define MIN_SIZE sizeof(S_List)

/** @brief Header size. */
#define HEADER_SIZE __builtin_offsetof(S_Chunk, pData)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Kernel's heap allocator list node. */
typedef struct List
{
  /** @brief Next node of the list. */
  struct List* pNext;
  /** @brief Previous node of the list. */
  struct List* pPrev;
} S_List;

/** @brief Kernel's heap allocator memory chunk representation. */
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
  S_List* res = _pop(&head);                       \
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

/**
 * @brief Assert macro used by the driver manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the driver manager to ensure correctness of
 * execution. Due to the critical nature of the driver manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define KHEAP_ASSERT(COND, MSG, ERROR) {           \
  if ((COND) == false)                             \
  {                                                \
    PANIC(ERROR, MODULE_NAME, MSG, false);         \
  }                                                \
}
/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

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
static inline S_List* _pop(S_List** ppList);

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
 */
static inline void _RemoveFree(S_Chunk* pChunk);

/**
 * @brief Pushes a memory chunk in the free memory chunks list.
 *
 * @details Pushes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] pChunk The chunk to be placed in the list.
 */
static inline void _PushFree(S_Chunk *pChunk);

/**
 * @brief Allocate memory from the kernel heap with ability to free later.
 *
 * @details Allocate a chunk of memory form the kernel heap and returns the
 * start address of the chunk.
 *
 * @param[in] kSize The number of byte to allocate.
 * @param[in] kAlign The alignement in bytes.
 *
 * @return A pointer to the start address of the allocated memory is returned.
 * If the memory cannot be allocated, a kernel panic is raised.
 */
static void* _KMallocFree(const size_t       kSize,
                          const E_Alignement kAlign);

                         /**
 * @brief Allocate memory from the kernel heap without ability to free later.
 *
 * @details Allocate a chunk of memory form the kernel heap and returns the
 * start address of the chunk.
 *
 * @param[in] kSize The number of byte to allocate.
 * @param[in] kAlign The alignement in bytes.
 *
 * @return A pointer to the start address of the allocated memory is returned.
 * If the memory cannot be allocated, a kernel panic is raised.
 */
static void* _KMallocNoFree(const size_t       kSize,
                            const E_Alignement kAlign);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the kernel's heap. */
extern uint8_t _KERNEL_HEAP_BASE;
/** @brief End address of the kernel's heap. */
extern uint8_t _KERNEL_HEAP_SIZE;
/** @brief Start address of the kernel's non free heap. */
extern uint8_t _KERNEL_NON_FREE_HEAP_BASE;
/** @brief End address of the kernel's non free heap. */
extern uint8_t _KERNEL_NON_FREE_HEAP_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/* Heap data */
/** @brief Kernel's heap free memory chunks. */
static S_Chunk* spFreeChunk[NUM_SIZES] = { NULL };
/** @brief Kernel's heap spFirstChunk memory chunk. */
static S_Chunk* spFirstChunk = NULL;
/** @brief Kernel's heap spLastChunk memory chunk. */
static S_Chunk* spLastChunk = NULL;

/** @brief Quantity of free memory in the kernel's heap. */
static size_t sMemFree;
/** @brief Quantity of used memory in the kernel's heap. */
static size_t sMemUsed;
/** @brief Quantity of memory used to store meta data in the kernel's heap. */
static size_t sMemMeta;

/** @brief Stores the head pointer of the non-free heap. */
static uintptr_t sNonFreeHeapHead;
/** @brief Stores the end pointer of the non-free heap. */
static uintptr_t sNonFreeHeapEnd;
/** @brief Stores the start pointer of the free heap. */
static uintptr_t sFreeHeapStart;
/** @brief Stores the end pointer of the free heap. */
static uintptr_t sFreeHeapEnd;

/** @brief Heap lock */
static S_KernelSpinlock sLock[2];

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

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

static inline S_List* _pop(S_List** ppList)
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
    _pop(ppList);
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

static inline void _RemoveFree(S_Chunk* pChunk)
{
  uint32_t len = _MemoryChunkSize(pChunk);
  int      n   = _MemoryChunkSlot(len);

  LIST_REMOVE_FROM(&spFreeChunk[n], pChunk, free);
  sMemFree -= len;
}

static inline void _PushFree(S_Chunk *pChunk)
{
  uint32_t len = _MemoryChunkSize(pChunk);
  int      n   = _MemoryChunkSlot(len);

  LIST_PUSH(&spFreeChunk[n], pChunk, free);
  sMemFree += len;
}

static void* _KMallocFree(const size_t kSize, const E_Alignement kAlign)
{
  size_t   n;
  size_t   size;
  S_Chunk* pChunk;
  S_Chunk* pChunk2;
  size_t   size2;
  size_t   len;
  void*    allocated;

  if (kSize != 0)
  {
    KERNEL_LOCK(sLock[0]);

    size = (kSize + kAlign - 1) & (~(kAlign - 1));

    if (size < MIN_SIZE)
    {
        size = MIN_SIZE;
    }

    n = _MemoryChunkSlot(size - 1) + 1;

    if (n < NUM_SIZES)
    {
      while (spFreeChunk[n] == 0)
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
        pChunk = LIST_POP(&spFreeChunk[n], free);
        size2 = _MemoryChunkSize(pChunk);
        len = 0;

        if (size + sizeof(S_Chunk) <= size2)
        {
          pChunk2 = (S_Chunk*)(((int8_t*)pChunk) + HEADER_SIZE + size);

          _MemoryChunkInit(pChunk2);
          _InsertAfter(&pChunk->all, &pChunk2->all);

          len = _MemoryChunkSize(pChunk2);
          n   = _MemoryChunkSlot(len);

          LIST_PUSH(&spFreeChunk[n], pChunk2, free);

          sMemMeta += HEADER_SIZE;
          sMemFree += len;
        }

        pChunk->used = true;

        sMemFree -= size2;
        sMemUsed += size2 - len - HEADER_SIZE;

        allocated = pChunk->pData;
      }
    }
    else
    {
      allocated = NULL;
    }

    KERNEL_UNLOCK(sLock[0]);
  }
  else
  {
    allocated = NULL;
  }

  return allocated;
}

static void* _KMallocNoFree(const size_t kSize, const E_Alignement kAlign)
{
  void*     allocated;
  uintptr_t allocAlign;

  allocated = NULL;

  if (kSize != 0)
  {
    KERNEL_LOCK(sLock[1]);

    allocAlign = (sNonFreeHeapHead + (kAlign - 1)) & ~((uintptr_t)kAlign - 1);
    if (allocAlign + kSize <= sNonFreeHeapEnd)
    {
      allocated = (void*)allocAlign;
      sNonFreeHeapHead = kSize + allocAlign;
    }

    KERNEL_UNLOCK(sLock[1]);
  }

  return allocated;
}

void KernelHeapInit(void)
{
  S_Chunk* pSecond;
  uint32_t len;
  int32_t  n;
  void*    pMem      = &_KERNEL_HEAP_BASE;
  uint32_t size      = (uint32_t)(uintptr_t)&_KERNEL_HEAP_SIZE;
  int8_t*  pMemStart = (int8_t*)
                       (((uintptr_t)pMem + ALIGN - 1) & (~(ALIGN - 1)));
  int8_t*  pMemEnd   = (int8_t*)(((uintptr_t)pMem + size) & (~(ALIGN - 1)));

  /* Initialize the pools */
  sNonFreeHeapHead = (uintptr_t)&_KERNEL_NON_FREE_HEAP_BASE;
  sNonFreeHeapEnd  = sNonFreeHeapHead + (uintptr_t)&_KERNEL_NON_FREE_HEAP_SIZE;
  sFreeHeapStart   = (uintptr_t)pMemStart;
  sFreeHeapEnd     = (uintptr_t)pMemEnd;


  /* Initialize the free pool */
  pMemStart    = (int8_t*)sFreeHeapStart;
  pMemEnd      = (int8_t*)sFreeHeapEnd;
  sMemFree     = 0;
  sMemMeta     = 0;
  spFirstChunk = NULL;
  spLastChunk  = NULL;

  spFirstChunk = (S_Chunk*)pMemStart;
  pSecond      = spFirstChunk + 1;
  spLastChunk = ((S_Chunk*)pMemEnd) - 1;

  _MemoryChunkInit(spFirstChunk);
  _MemoryChunkInit(pSecond);
  _MemoryChunkInit(spLastChunk);

  _InsertAfter(&spFirstChunk->all, &pSecond->all);
  _InsertAfter(&pSecond->all, &spLastChunk->all);

  spFirstChunk->used = true;
  spLastChunk->used  = true;

  len = _MemoryChunkSize(pSecond);
  n   = _MemoryChunkSlot(len);

  LIST_PUSH(&spFreeChunk[n], pSecond, free);
  sMemFree = len - HEADER_SIZE;
  sMemMeta = sizeof(S_Chunk) * 2 + HEADER_SIZE;

  KERNEL_SPINLOCK_INIT(sLock[0]);
  KERNEL_SPINLOCK_INIT(sLock[1]);
}

void* KMalloc(const size_t        kSize,
              const E_Alignement  kAlign,
              const E_KMallocPool kPool)
{
  void* allocated;

  allocated = NULL;

  if (kPool == KMALLOC_NO_FREE_POOL)
  {
    allocated = _KMallocNoFree(kSize, kAlign);
  }
  else if (kPool == KMALLOC_FREE_POOL)
  {
    allocated = _KMallocFree(kSize, kAlign);
  }

  while (allocated == NULL) {}

  KHEAP_ASSERT(allocated != NULL, "Failed to allocate memory.", ERR_NO_MEMORY);

  return allocated;
}

void KFree(void *ptr)
{
  S_Chunk* pChunk;
  S_Chunk* pNext;
  S_Chunk* pPrev;


  KHEAP_ASSERT(
    sFreeHeapStart <= (uintptr_t)ptr && sFreeHeapEnd > (uintptr_t)ptr,
    "Failed to allocate memory.",
    ERR_NO_MEMORY);


  KERNEL_LOCK(sLock[0]);

  pChunk = (S_Chunk*)((int8_t*)ptr - HEADER_SIZE);

  pNext = CONTAINER(S_Chunk, all, pChunk->all.pNext);
  pPrev = CONTAINER(S_Chunk, all, pChunk->all.pPrev);

  sMemUsed -= _MemoryChunkSize(pChunk);

  if (pNext->used == false)
  {
    _RemoveFree(pNext);
    _Remove(&pNext->all);

    sMemMeta -= HEADER_SIZE;
    sMemFree += HEADER_SIZE;
  }

  if (pPrev->used == false)
  {
    _RemoveFree(pPrev);
    _Remove(&pChunk->all);

    _PushFree(pPrev);
    sMemMeta -= HEADER_SIZE;
    sMemFree += HEADER_SIZE;
  }
  else
  {
    pChunk->used = false;
    LIST_INIT(pChunk, free);
    _PushFree(pChunk);
  }

  KERNEL_UNLOCK(sLock[0]);
}

/************************************ EOF *************************************/