/*******************************************************************************
 * @file KernelHeap.h
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

#ifndef __CORE_KHEAP_H_
#define __CORE_KHEAP_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <Critical.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Num size. */
#define NUM_SIZES 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the allowed memory allignement for memory allocation. */
typedef enum
{
  /** @brief 1 byte alignment boundary. */
  ALIGN_1_BYTE = 1,
  /** @brief 2 bytes alignment boundary. */
  ALIGN_2_BYTES = 2,
  /** @brief 4 bytes alignment boundary. */
  ALIGN_4_BYTES = 4,
  /** @brief 8 bytes alignment boundary. */
  ALIGN_8_BYTES = 8,
  /** @brief 16 bytes alignment boundary. */
  ALIGN_16_BYTES = 16,
  /** @brief Address size bytes alignment boundary. */
  ALIGN_ADDRESS = sizeof(uintptr_t)
} E_Alignement;

/** @brief Defines the allowed memory pool for memory allocation. */
typedef enum
{
  /** @brief Free non allowed pool. */
  KMALLOC_NO_FREE_POOL,
  /** @brief Free allowed pool. */
  KMALLOC_FREE_POOL,
  /** @brief Current process heap. */
  KMALLOC_PROCESS_HEAP
} E_KMallocPool;

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

/** @brief Defines the information for a process heap. */
typedef struct
{
  /** @brief The base address of the heap. */
  uintptr_t baseAddress;
  /** @brief The end address of the heap. */
  uintptr_t endAddress;

  /** @brief The current offset of the heap. */
  uintptr_t currentHead;

  /** @brief Kernel's heap free memory chunks. */
  S_Chunk* spFreeChunk[NUM_SIZES];
  /** @brief Kernel's heap spFirstChunk memory chunk. */
  S_Chunk* spFirstChunk;
  /** @brief Kernel's heap spLastChunk memory chunk. */
  S_Chunk* spLastChunk;

  /** @brief Quantity of free memory in the kernel's heap. */
  size_t sMemFree;
  /** @brief Quantity of used memory in the kernel's heap. */
  size_t sMemUsed;
  /** @brief Quantity of memory used to store meta data in the kernel's heap. */
  size_t sMemMeta;

  /** @brief Kernel's heap spinlock. */
  S_KernelSpinlock lock;
} S_ProcessHeap;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Initializes the kernel's heap.
 *
 * @details Setups kernel heap management. It will also allign kernel heap start
 * and initialize the basic heap parameters such as its size.
 */
void KernelHeapInit(void);

/**
 * @brief Create a Process Heap for the provided process.
 *
 * @details Create a Process Heap for the provided process. This will initialize
 * the heap and allocate it from free memory. The heap will be mapped to the
 * process address space.
 *
 * @param ppHeap[out] The process heap pointer.
 *
 * @return The success or error status is returned.
 */
E_Return CreateProcessHeap(S_ProcessHeap** ppHeap);

/**
 * @brief Destroys a process heap.
 *
 * @details Destroys a process heap. This will release the allocated memory
 * back to the kernel space.
 *
 * @param[out] pHeap The heap to destroy.
 */
void DestroyProcessHeap(S_ProcessHeap* pHeap);

/**
 * @brief Allocate memory from the kernel heap.
 *
 * @details Allocate a chunk of memory form the kernel heap and returns the
 * start address of the chunk.
 *
 * @param[in] kSize The number of byte to allocate.
 * @param[in] kAlign The alignement in bytes.
 * @param[in] kPool The heap pool to use for the allocation.
 *
 * @return A pointer to the start address of the allocated memory is returned.
 * If the memory cannot be allocated, a kernel panic is raised.
 */
void* KMalloc(const size_t        kSize,
              const E_Alignement  kAlign,
              const E_KMallocPool kPool);

/**
 * @brief Free allocated memory.
 *
 * @details Releases allocated memory.IOf the pointer is NULL or has not been
 * allocated previously from the heap, nothing is done.
 *
 * @param[in, out] ptr The start address of the memory area to free.
 * @param[in] kPool The heap pool to use for the deallocation.
 */
void KFree(void* ptr, const E_KMallocPool kPool);

#endif /* #ifndef __CORE_KHEAP_H_ */

/************************************ EOF *************************************/