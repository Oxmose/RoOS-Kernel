/*******************************************************************************
 * @file Memory.h
 *
 * @see Memory.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 09/06/2024
 *
 * @version 3.0
 *
 * @brief Kernel physical memory manager.
 *
 * @details This module is used to detect the memory mapping of the system and
 * manage physical and virtual memory as well as peripherals memory.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_MEMORY_H_
#define __CPU_MEMORY_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <CtrlBlock.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Memory mapping flags: Read-Only mapping */
#define MEMMGR_MAP_RO 0x1ULL
/** @brief Memory mapping flags: Read-Write mapping */
#define MEMMGR_MAP_RW 0x2ULL
/** @brief Memory mapping flags: Execute mapping */
#define MEMMGR_MAP_EXEC 0x4ULL
/** @brief Memory mapping flags: Kernel access only  */
#define MEMMGR_MAP_KERNEL 0x8ULL
/** @brief Memory mapping flags: Kernel and user access */
#define MEMMGR_MAP_USER 0x10ULL
/** @brief Memory mapping flags: Cache disabled */
#define MEMMGR_MAP_CACHE_DISABLED 0x20ULL
/** @brief Memory mapping flags: Hardware */
#define MEMMGR_MAP_HARDWARE 0x40ULL
/** @brief Memory mapping flags: Write Combining */
#define MEMMGR_MAP_WRITE_COMBINING 0x80ULL
/** @brief Memory mapping flags: Copy On Write */
#define MEMMGR_MAP_COW 0x100ULL

/** @brief Kernel page size */
#define KERNEL_PAGE_SIZE 0x1000ULL
/** @brief Page size mask */
#define PAGE_SIZE_MASK 0xFFFULL

/** @brief Defines the error for physical address */
#define MEMMGR_PHYS_ADDR_ERROR ((uintptr_t)0xFFFFFFFFFFFFFFFFULL)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines a memory list */
typedef struct
{
  /** @brief The memory list structure  */
  S_KernelQueue* pQueue;
  /** @brief The memory list lock */
  S_KernelSpinlock lock;
} S_MemoryList;

 /**
 * @brief Defines the structure that contains the memory information for a
 * process.
 */
typedef struct
{
  /** @brief The physical address of the process page directory. */
  uintptr_t PDPhysAddress;
  /** @brief The free page table of the process. */
  S_MemoryList freePageTable;
  /** @brief The memory management lock for the process */
  S_KernelSpinlock lock;
} S_ProcessMemoryMetadata;

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
 * @brief Inititalizes the kernel's memory manager.
 *
 * @brief Initializes the kernel's memory manager while detecting the system's
 * memory organization.
 */
void MemoryInit(void);

/**
 * @brief Maps a physical region (memory or hardware) in the kernel address
 * space.
 *
 * @details Maps a kernel virtual memory region to a memory region. The function
 * does not check if the physical region is already mapped and will create a
 * new mapping. The physical address and the size must be aligned on page
 * boundaries. If not, the mapping fails and NULL is returned.
 *
 * @param[in] kPhysicalAddress The physical address to map. Must be aligned on
 * page boundaries.
 * @param[in] kSize The size of the region to map in bytes. Must be aligned on
 * page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[out] pError The error buffer to store the operation's result. If NULL,
 * does not set the error value.
 *
 * @return The function returns the virtual base address of the mapped region.
 * NULL is returned on error.
 *
 * @warning The mapping does not remove the physical address from the free
 * free memory. Thus, if the user wants to ensure this memory region not to be
 * used later (or already used) memoryAllocFrames must be used to get a free
 * physical memory region. This does not apply to hardware mapping.
 */
void* MemoryKernelMap(const void*    kPhysicalAddress,
                      const size_t   kSize,
                      const uint32_t kFlags,
                      E_Return*      pError);

/**
 * @brief Unmaps a virtual region (memory or hardware) from the kernel address
 * space.
 *
 * @details Unmaps a virtual region (memory or hardware) from the kernel address
 * space. The virtual address and the size must be aligned on page
 * boundaries. If not, the unmapping fails and an error is returned.
 *
 * @param[in] kVirtualAddress The virtual address to unmap. Must be aligned on
 * page boundaries.
 * @param[in] kSize The size of the region to unmap map in bytes. Must be
 * aligned on page boundaries.
 *
 * @return The function returns success or error status.
 */
E_Return MemoryKernelUnmap(const void* kVirtualAddress, const size_t kSize);

/**
 * @brief Returns the physical address of a virtual address mapped in the
 * current page directory.
 *
 * @details Returns the physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 *
 * @param[in] kVirtualAddress The virtual address to lookup.
 * @param[in] kpProcess The process to use for the search.
 * @param[out] pFlags The memory flags used for the mapping. Can be NULL.
 *
 * @returns The physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 */
uintptr_t MemoryMgrGetPhysAddr(const uintptr_t        kVirtualAddress,
                               const S_KernelProcess* kpProcess,
                               uint32_t*              pFlags);

/**
 * @brief Maps a stack in the process memory region and returns its address.
 *
 * @details Maps a stack in the process memory region and returns its address.
 * One more page after the stack is allocated but not mapped to catch overflows.
 * The required frames are also allocated.
 *
 * @param[in] kSize The size of the stack. If not aligned with the kernel page
 * size, the actual mapped size will be aligned up on page boundaries.
 * @param[in] kIsKernel Tells if the stack is a kernel or user stack.
 * @param[in, out] pProcess The process from which the stack should be
 * allocated.
 *
 * @return The base end of the stack in kernel memory is returned.
 */
uintptr_t MemoryMapStack(const size_t     kSize,
                         const bool       kIsKernel,
                         S_KernelProcess* pProcess);

/**
 * @brief Unmaps a stack in the process memory region and frees the associated
 * physical memory.
 *
 * @details Maps a stack in the process memory region and frees the associated
 * physical memory.
 * The additional overflow page is also freed.
 *
 * @param[in] kEndAddress The end address of the stack to unmap. If not
 * aligned with the kernel page size, a panic is generated.
 * @param[in] kSize The size of the stack. If not aligned with the kernel page
 * size, a panic is generated.
 * @param[in] kIsKernel Tells if the stack is a kernel stack.
 * @param[in, out] pProcess The process to which the stack should be released.
 */
void MemoryUnmapStack(const uintptr_t  kEndAddress,
                      const size_t     kSize,
                      const bool       kIsKernel,
                      S_KernelProcess* pProcess);

/**
 * @brief Maps a physical memory region in the kernel address space.
 *
 * @details Maps a physical memory region in the kernel address space. The
 * function allocates free memory frames to the kernel and creates a new
 * mapping. The size must be aligned on page boundaries. If not, the mapping
 * fails and NULL is returned.
 *
 * @param[in] kSize The size of the region to map in bytes. Must be aligned on
 * page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[out] pError The error buffer to store the operation's result. If NULL,
 * does not set the error value.
 *
 * @return The function returns the virtual base address of the mapped region.
 * NULL is returned on error.
 */
void* MemoryKernelAllocate(const size_t   kSize,
                           const uint32_t kFlags,
                           E_Return*      pError);

/**
 * @brief Releases a physical memory region in the kernel address space.
 *
 * @details Releases a physical memory region in the kernel address space. The
 * function releases the memory frames to the kernel and removes the existing
 * mapping. The size must be aligned on page boundaries. If not, the unmapping
 * fails and NULL is returned.
 *
 * @param[in] kVirtualAddress The virtual address of the mapping to release.
 * @param[in] kSize The size of the mapping to release.
 *
 * @return The function returns the success or error state.
 */
E_Return MemoryKernelFree(const void* kVirtualAddress, const size_t kSize);

/**
 * @brief Creates a process memory configuration.
 *
 * @details Creates a process memory configuration. The function will allocate
 * the required resources.
 *
 * @return The function return a pointer to the configuration on success or NULL
 * on error.
 */
void* MemoryCreateProcessData(void);

/**
 * @brief Destroys a process memory configuration.
 *
 * @details Destroys a process memory configuration. The function will release
 * the required resources.
 *
 * @param[in] pMemoryData The configuration to release.
 */
void MemoryDestroyProcessData(void* pMemoryData);

/**
 * @brief Returns the user space start address.
 *
 * @details Returns the user space start address.
 *
 * @return The function returns the user space start address.
 */
uintptr_t MemoryGetUserStartAddr(void);

/**
 * @brief Returns the user space end address.
 *
 * @details Returns the user space end address.
 *
 * @return The function returns the user space end address.
 */
uintptr_t MemoryGetUserEndAddr(void);

/**
 * @brief Kernel memory frame allocation.
 *
 * @details Kernel memory frame allocation. This method gets the desired number
 * of contiguous frames from the kernel frame pool and allocate them.
 *
 * @param[in] kFrameCount The number of desired frames to allocate.
 *
 * @return The address of the first frame of the contiguous block is
 * returned.
 */
uintptr_t MemoryAllocFrames(const size_t kFrameCount);

/**
 * @brief Memory frames release.
 *
 * @details Memory frames release. This method releases the memory frames
 * to the free frames pool. Releasing already free or out of bound frame will
 * generate a kernel panic.
 *
 * @param[in] kBaseAddress The base address of the contiguous frame pool to
 * release.
 * @param[in] kFrameCount The number of desired frames to release.
 */
void MemoryReleaseFrame(const uintptr_t kBaseAddress,
                        const size_t    kFrameCount);

/**
 * @brief Maps a physical region (memory or hardware) in the user address
 * space to the requested virtual address.
 *
 * @details Maps a user virtual memory region to a memory region. The function
 * does not check if the physical region is already mapped and will create a
 * new mapping. The physical address and the size must be aligned on page
 * boundaries. If not, the mapping fails and NULL is returned.
 *
 * @param[in] kPhysicalAddress The physical address to map. Must be aligned on
 * page boundaries.
 * @param[in] kVirtualAddress The virtual address to map. Must be aligned on
 * page boundaries.
 * @param[in] kSize The size of the region to map in bytes. Must be aligned on
 * page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[in] kRemoveFromPagePool Tells if the mapped virtual addresses shall
 * be removed from the process free page pool.
 * @param[in, out] pProcess The process for which the mapping should be
 * effective.
 *
 * @return The function returns the success or error status.
 *
 * @warning The mapping does not remove the physical address from the free
 * free memory. Thus, if the user wants to ensure this memory region not to be
 * used later (or already used) memoryAllocFrames must be used to get a free
 * physical memory region. This does not apply to hardware mapping.
 */
E_Return MemoryUserMapDirect(const void*      kPhysicalAddress,
                             const void*      kVirtualAddress,
                             const size_t     kSize,
                             const uint32_t   kFlags,
                             const bool       kRemoveFromPagePool,
                             S_KernelProcess* pProcess);

/**
 * @brief Unmaps a virtual region (memory or hardware) from the user address
 * space.
 *
 * @details Unmaps a virtual region (memory or hardware) from the user address
 * space. The virtual address and the size must be aligned on page
 * boundaries. If not, the unmapping fails and an error is returned.
 *
 * @param[in] kVirtualAddress The virtual address to unmap. Must be aligned on
 * page boundaries.
 * @param[in] kSize The size of the region to unmap map in bytes. Must be
 * aligned on page boundaries.
 * @param[in] kAddToPagePool Tells if the ubmapped virtual addresses shall
 * be released to the process free page pool.
 * @param[in, out] pProcess The process for which the mapping should be
 * removed.
 *
 * @return The function returns success or error status.
 */
E_Return MemoryUserUnmap(const void*      kVirtualAddress,
                         const size_t     kSize,
                         const bool       kAddToPagePool,
                         S_KernelProcess* pProcess);

/**
 * @brief Maps a physical memory region in the user address space.
 *
 * @details Maps a physical memory region in the user address space. The
 * function allocates free memory frames to the user and creates a new
 * mapping. The size must be aligned on page boundaries. If not, the mapping
 * fails and NULL is returned.
 *
 * @param[in] kSize The size of the region to map in bytes. Must be aligned on
 * page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[in] pProcess The process to which the memory shall be allocated.
 * @param[out] pError The error buffer to store the operation's result. If NULL,
 * does not set the error value.
 *
 *
 * @return The function returns the virtual base address of the mapped region.
 * NULL is returned on error.
 */
void* MemoryUserAllocate(const size_t     kSize,
                         const uint32_t   kFlags,
                         S_KernelProcess* pProcess,
                         E_Return*        pError);

/**
 * @brief Releases a physical memory region in the user address space.
 *
 * @details Releases a physical memory region in the user address space. The
 * function releases the memory frames to the user and removes the existing
 * mapping. The size must be aligned on page boundaries. If not, the unmapping
 * fails and NULL is returned.
 *
 * @param[in] kVirtualAddress The virtual address of the mapping to release.
 * @param[in] kSize The size of the mapping to release.
 * @param[in] pProcess The process from which the memory shall be released.
 *
 * @return The function returns the success or error state.
 */
E_Return MemoryUserFree(const void*      kVirtualAddress,
                        const size_t     kSize,
                        S_KernelProcess* pProcess);

/**
 * @brief Tells if a memory region is already mapped.
 *
 * @details Tells if a memory region is already mapped.
 * Returns false if the region is not mapped, true otherwise.
 *
 * @param[in] kVirtualAddress The base virtual address to check for mapping.
 * @param[in] kPageCount The number of pages to check for mapping.
 * @param[in] pProcess The process whose page directory to use for the search.
 * @param[in] kCheckFull Tells if the full range shall be mapped to return true.
 *
 * @return Returns false if the region is not mapped, true otherwise.
 */
bool MemoryIsMapped(const uintptr_t  kVirtualAddress,
                    const size_t     kPageCount,
                    S_KernelProcess* pProcess,
                    const bool       kCheckFull);
#endif /* #ifndef __CPU_MEMORY_H_ */

/************************************ EOF *************************************/