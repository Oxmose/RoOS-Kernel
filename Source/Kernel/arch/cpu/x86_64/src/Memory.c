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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <stdint.h>
#include <stddef.h>
#include <X64Cpu.h>
#include <Critical.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <Memory.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86_64 MEM"

/** @brief Kernel's page directory entry count */
#define KERNEL_PGDIR_ENTRY_COUNT 512

/** @brief Kernel PML4 entry offset. */
#define PML4_ENTRY_OFFSET 39
/** @brief Kernel PML3 entry offset. */
#define PML3_ENTRY_OFFSET 30
/** @brief Kernel PML2 entry offset. */
#define PML2_ENTRY_OFFSET 21
/** @brief Kernel PML1 entry offset. */
#define PML1_ENTRY_OFFSET 12
/** @brief Kernel page entry mask */
#define PG_ENTRY_OFFSET_MASK 0x1FF

/** @brief Page directory flag: 4KB page size. */
#define PAGE_FLAG_PAGE_SIZE_4KB 0x0000000000000000
/** @brief Page directory flag: 2MB page size. */
#define PAGE_FLAG_PAGE_SIZE_2MB 0x0000000000000080
/** @brief Page directory flag: 1GB page size. */
#define PAGE_FLAG_PAGE_SIZE_1GB 0x0000000000000080

/** @brief Page flag: executable page. */
#define PAGE_FLAG_XD 0x8000000000000000
/** @brief Page flag: page accessed. */
#define PAGE_FLAG_ACCESSED 0x0000000000000020
/** @brief Page flag: cache disabled for the page. */
#define PAGE_FLAG_CACHE_DISABLED 0x0000000000000010
/** @brief Page flag: cache write policy set to write through. */
#define PAGE_FLAG_CACHE_WT 0x0000000000000008
/** @brief Page flag: cache write policy set to write back. */
#define PAGE_FLAG_CACHE_WB 0x0000000000000000
/** @brief Page flag: access permission set to user. */
#define PAGE_FLAG_USER_ACCESS 0x0000000000000004
/** @brief Page flag: access permission set to kernel. */
#define PAGE_FLAG_SUPER_ACCESS 0x0000000000000000
/** @brief Page flag: access permission set to read and write. */
#define PAGE_FLAG_READ_WRITE 0x0000000000000002
/** @brief Page flag: access permission set to read only. */
#define PAGE_FLAG_READ_ONLY 0x0000000000000000
/** @brief Page flag: page present. */
#define PAGE_FLAG_PRESENT 0x0000000000000001
/** @brief Page flag: page present. */
#define PAGE_FLAG_IS_HW 0x0000000000000400
/** @brief Page flag: page global. */
#define PAGE_FLAG_GLOBAL 0x0000000000000100
/** @brief Page flag: PAT */
#define PAGE_FLAG_PAT 0x0000000000000080
/** @brief Page flag: Copy On Write */
#define PAGE_FLAG_COW 0x0000000000000200
/** @brief Page flag: Write Combining */
#define PAGE_FLAG_CACHE_WC (PAGE_FLAG_CACHE_DISABLED |  \
                            PAGE_FLAG_CACHE_WT       |  \
                            PAGE_FLAG_PAT)

/** @brief Defines the physical memory linear paging entry */
#define KERNEL_MEM_PML4_ENTRY 510
/** @brief Defines the the kernel directory entry */
#define KERNEL_PML4_KERNEL_ENTRY 511

/** @brief Defines the the kernel temporary boot directory entry */
#define KERNEL_PML4_BOOT_TMP_ENTRY 1

/** @brief Page fault error code: page protection violation. */
#define PAGE_FAULT_ERROR_PROT_VIOLATION 0x1
/** @brief Page fault error code: fault on a write. */
#define PAGE_FAULT_ERROR_WRITE 0x2
/** @brief Page fault error code: fault in user mode. */
#define PAGE_FAULT_ERROR_USER 0x4
/** @brief Page fault error code: fault on instruction fetch. */
#define PAGE_FAULT_ERROR_EXEC 0x10

/** @brief Defines the maximal physical address for memory */
#define KERNEL_MAX_MEM_PHYS 0x7FC0000000ULL
/** @brief Represents 1GB */
#define KERNEL_MEM_1GB 0x40000000ULL
/** @brief Represents 2MB */
#define KERNEL_MEM_2MB 0x200000ULL

/** @brief User total memory start. */
#define USER_MEMORY_START 0x0000000000100000ULL
/** @brief User total memory end. */
#define USER_MEMORY_END 0xFFFFFF0000000000ULL

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines a frame metadata structure */
typedef struct
{
  /** @brief Frame reference count */
  uint16_t refCount;
  /** @brief The frame metadata lock */
  S_KernelSpinlock lock;
} S_FrameMetadata;

/**
 * @brief Defines a table of contiguous physical memory used for reference and
 * metadata management.
 */
typedef struct S_FrameMetadataTable
{
  /** @brief First frame in the table. */
  uintptr_t firstFrame;
  /** @brief Last frame in the table. */
  uintptr_t lastFrame;
  /** @brief Reference count table */
  S_FrameMetadata* pRefCountTable;
  /** @brief Next table in a list */
  struct S_FrameMetadataTable* pNext;
} S_FrameMetadataTable;

/** @brief Defines a memory range */
typedef struct
{
  /** @brief Range's base address. */
  uintptr_t base;
  /** @brief Range's limit. */
  uintptr_t limit;
} S_MemoryRange;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the memory manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the memory manager to ensure correctness of
 * execution. Due to the critical nature of the memory manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define MEM_ASSERT(COND, MSG, ERROR) {                    \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
  }                                                       \
}

/**
 * @brief Gets the virtual address in the linear physical-to-virtual memory
 * space.
 *
 * @param[in] PHYS_MEM_ADDR The physical memory address to convert.
 */
#define GET_VIRT_MEM_ADDR(PHYS_MEM_ADDR) (                            \
  _MakeCanonical(((uintptr_t)(PHYS_MEM_ADDR) +                        \
                  (KERNEL_MEM_PML4_ENTRY * 512ULL * KERNEL_MEM_1GB)), \
                  false)                                              \
)

/**
 * @brief Gets the physical address in the linear physical-to-virtual memory
 * space.
 *
 * @param[in] VIRT_MEM_ADDR The virtual memory address to convert.
 */
#define GET_PHYS_MEM_ADDR(VIRT_MEM_ADDR) (                            \
  _MakeCanonical(((uintptr_t)(VIRT_MEM_ADDR) -                        \
                  (KERNEL_MEM_PML4_ENTRY * 512ULL * KERNEL_MEM_1GB)), \
                  true)                                               \
)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Page fault handler.
 *
 * @details Page fault handler. Manages page fault occuring while a thread is
 * running. The handler might call a panic if the fault cannot be resolved.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _PageFaultHandler(void);

/**
 * @brief Makes the address passed as parameter canonical.
 *
 * @details Makes the address passed as parameter canonical. This means it
 * extends its last address bit sign.
 *
 * @param[in] kAddress The address to make canonical.
 * @param[in] kIsPhysical Tells if the address is physical or virtual.
 *
 * @return The canonical address is returned;
 */
static inline uintptr_t _MakeCanonical(const uintptr_t kAddress,
                                       const bool      kIsPhysical);

/**
 * @brief Adds a free memory block to a memory list.
 *
 * @details Adds a free memory block to a memory list. The list will be kept
 * sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to add the block to.
 * @param[in] kBaseAddress The first address of the memory block to add.
 * @param[in] kLength The size, in bytes of the memory region to add.
 */
static void _AddBlock(S_MemoryList*   pList,
                      const uintptr_t kBaseAddress,
                      const size_t    kLength);

/**
 * @brief Removes a memory block to a memory list.
 *
 * @details Removes a memory block to a memory list. The list will be kept
 * sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to remove the block from.
 * @param[in] kBaseAddress The first address of the memory block to remove.
 * @param[in] kLength The size, in bytes of the memory region to remove.
 */
static void _RemoveBlock(S_MemoryList*   pList,
                         const uintptr_t kBaseAddress,
                         const size_t    kLength);

/**
 * @brief Returns a block for a memory list and removes it.
 *
 * @details Returns a block for a memory list and removes it. The list will be
 * kept sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to get the block from.
 * @param[in] kLength The size, in bytes of the memory region to get.
 */
static uintptr_t _GetBlock(S_MemoryList* pList, const size_t kLength);

/**
 * @brief Returns a block from the end of a memory list and removes it.
 *
 * @details Returns a block from the end of a memory list and removes it.
 * The list will be kept sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to get the block from.
 * @param[in] kLength The size, in bytes of the memory region to get.
 */
static uintptr_t _GetBlockFromEnd(S_MemoryList* pList, const size_t kLength);

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
static uintptr_t _AllocateFrames(const size_t kFrameCount);

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
static void _ReleaseFrames(const uintptr_t kBaseAddress,
                           const size_t    kFrameCount);

/**
 * @brief Kernel memory pages allocation.
 *
 * @details Kernel memory pages allocation. This method gets the desired number
 * of contiguous pages from the kernel pages pool and allocate them.
 *
 * @param[in] kPageCount The number of desired pages to allocate.
 *
 * @return The address of the first page of the contiguous block is
 * returned.
 */
static uintptr_t _AllocateKernelPages(const size_t kPageCount);

/**
 * @brief Kernel memory page release.
 *
 * @details Kernel memory page release. This method rekeases the kernel pages
 * to the kernel pages pool. Releasing already free or out of bound pages will
 * generate a kernel panic.
 *
 * @param[in] kBaseAddress The base address of the contiguous pages pool to
 * release.
 * @param[in] kPageCount The number of desired pages to release.
 */
static void _ReleaseKernelPages(const uintptr_t kBaseAddress,
                                const size_t    kPageCount);


/**
 * @brief Tells if a memory region is already mapped in the a page table.
 *
 * @details Tells if a memory region is already mapped in the a page table.
 * Returns false if the region is not mapped, true otherwise.
 *
 * @param[in] kVirtualAddress The base virtual address to check for mapping.
 * @param[in] kPageCount The number of pages to check for mapping.
 * @param[in] kPageDir The page directory to use for the search
 * @param[in] kCheckFull Tells if the full range shall be mapped to return true.
 *
 * @return Returns false if the region is not mapped, true otherwise.
 */
static bool _IsMapped(const uintptr_t kVirtualAddress,
                      const size_t    kPageCount,
                      const uintptr_t kPageDir,
                      const bool      kCheckFull);

/**
 * @brief Maps the virtual address to the physical address in a page directory.
 *
 * @details Maps the virtual address to the physical address in a page
 * directory. If the addresses or sizes are not correctly aligned, or if the
 * mapping already exists, an error is retuned. If the address is not in the
 * free memory frames pool and the klags does not mention hardware mapping, an
 * error is returned.
 *
 * @param[in] kVirtualAdderss The virtual address to map.
 * @param[in] kPhysicalAddress The physical address to map.
 * @param[in] kPageCount The number of pages to map.
 * @param[in] kFlags The flags used for mapping.
 * @param[in] kPageDir The page directory to use.
 *
 * @return The success or error status is returned.
 */
static E_Return _MemoryMap(const uintptr_t kVirtualAddress,
                           const uintptr_t kPhysicalAddress,
                           const size_t    kPageCount,
                           const uint32_t  kFlags,
                           const uintptr_t kPageDir);

/**
 * @brief Unmaps the virtual address in a page directory.
 *
 * @details Unmaps the virtual address to the physical address in in a page
 * directory.
 *
 * @param[in] kVirtualAdderss The virtual address to unmap.
 * @param[in] kPageCount The number of pages to unmap.
 * @param[in] kPageDir The page directory to use.
 */
static E_Return _MemoryUnmap(const uintptr_t kVirtualAddress,
                             const size_t    kPageCount,
                             const uintptr_t kPageDir);

/**
 * @brief Returns the physical address of a virtual address mapped in the
 * current page directory.
 *
 * @details Returns the physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 *
 * @param[in] kVirtualAddress The virtual address to lookup.
 * @param[in] kPageDir The page directory to use.
 * @param[out] pFlags The memory flags used for the mapping. Can be NULL.
 *
 * @returns The physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 */
static uintptr_t _GetPhysAddr(const uintptr_t kVirtualAddress,
                              const uintptr_t kPageDir,
                              uint32_t*       pFlags);

/**
 * @brief Detects the hardware memory presents in the system.
 *
 * @details Detects the hardware memory presents in the system. Each
 * memory range is tagged as memory. This function uses the FDT to
 * register the available memory and reserved memory.
 */
static void _DetectMemory(void);

/**
 * @brief Creates the translation entries in the flat mapping.
 *
 * @details Creates the translation entries in the flat mapping. This will add
 * the last entry to the page directory entry reserved for flat mapping.
 *
 * @return The physical address of the begining of the last entry table for flat
 * mapping is returned.
 */
static uintptr_t _MapTranslationTable(void);

/**
 * @brief Creates a flat mapping of the physical addresses in virtual address
 * space.
 *
 * @details Creates a flat mapping of the physical addresses in virtual address
 * space. A page directory entry is reserved for this purpose and the subsequent
 * entries are defined by the capability of the cpu.
 */
static void _CreateFlatMap(void);

/**
 * @brief Creates the frame metadata table.
 *
 * @brief Creates the frame metadata table. The memory for the metadata will
 * be allocated from the memory block themselves.
 */
static void _CreateFramesMetadata(void);

/**
 * @brief Setups the memory tables used in the kernel.
 *
 * @details Setups the memory tables used in the kernel. The memory map is
 * generated, the pages and frames lists are also created.
 */
static void _InitKernelFreePages(void);

/**
 * @brief Maps a kernel section to a page directory mapped in virtual memory.
 *
 * @details Maps a kernel section to a page directory mapped in virtual memory.
 * Frames can be allocated if needed. The function checks if the previous kernel
 * section is not overlapping the current one.
 *
 * @param[out] pLastSectionStart Start virtual address of the previous kernel
 * section.
 * @param[out] pLastSectionEnd End virtual address of the previous kernel
 * section.
 * @param[in] kRegionStartAddr Start virtual address of the section to map.
 * @param[in] kRegionEndAddr End virtual address of the section to map.
 * @param[in] kFlags Mapping flags.
 */
static void _MapKernelRegion(uintptr_t*      pLastSectionStart,
                             uintptr_t*      pLastSectionEnd,
                             const uintptr_t kRegionStartAddr,
                             const uintptr_t kRegionEndAddr,
                             const uint32_t  kFlags);

/**
 * @brief Initializes paging structures for the kernel.
 *
 * @details Initializes paging structures for the kernel. This function will
 * select an available memory region to allocate the memory required for the
 * kernel. Then the kernel will be mapped to memory and paging is enabled for
 * the kernel.
 */
static void _MapKernel(void);

/**
 * @brief Releases the memory used by a process.
 *
 * @details Releases the memory used by a process. This function only releases
 * the frames that are used un user space and the frames that compose the user
 * land of the page directory.
 *
 * @param[in] kPhysTable The page directory physical address to release.
 * @param[in] kBaseVirtAddr The start virtual address at which the page dir
 * shall be released.
 * @param[in] kLevel The page directory level to release.
 */
static void _ReleasePageDir(const uintptr_t kPhysTable,
                            const uintptr_t kBaseVirtAddr,
                            const uint8_t   kLevel);

/**
 * @brief User memory pages allocation.
 *
 * @details User memory pages allocation. This method gets the desired number
 * of contiguous pages from the user pages pool and allocate them.
 *
 * @param[in] kPageCount The number of desired pages to allocate.
 * @param[in] kpProcess The process from which the pages should be allocated.
 * @param[in] kFromTop Tells if the pages must be allocated from the top or
 * the bottom of the memory space.
 *
 * @return The bottom address of the first page of the contiguous block is
 * returned.
 */
static uintptr_t _AllocateUserPages(const size_t           kPageCount,
                                    const S_KernelProcess* kpProcess,
                                    const bool             kFromTop);

/**
 * @brief User memory page release.
 *
 * @details User memory page release. This method releases the user pages
 * to the user pages pool. Releasing already free or out of bound pages will
 * generate a user panic.
 *
 * @param[in] kBaseAddress The base address of the contiguous pages pool to
 * release.
 * @param[in] kPageCount The number of desired pages to release.
 * @param[in] kpProcess The process to which the pages should be released.
 *
 */
static void _ReleaseUserPages(const uintptr_t        kBaseAddress,
                              const size_t           kPageCount,
                              const S_KernelProcess* kpProcess);

/**
 * @brief Translates memory mapping flags from the memory manager interface to
 * the processor mapping flags.
 *
 * @details Translates memory mapping flags from the memory manager interface to
 * the processor mapping flags. This takes into account merges flags and will
 * prevent incompatible flags to be set.
 *
 * @param kFlags The memory manager flags to translate to the CPU mapping flags.
 *
 * @return The function returns the CPU mapping flag fileds.
 */
static inline uintptr_t _TranslateFlags(const uint32_t kFlags);

/**
 * @brief Maps a physical address in the user space of a given process.
 *
 * @details Maps a physical address in the user space of a given process.
 * This function will check the memmory bounds before applying the mapping.
 * An will set the new mapping in the provided process address space. This
 * function is recursive for ease of implementation.
 *
 * @param pTableLevel The mapped page table level where the memory will be
 * mapped.
 * @param pVirtAddress The virtual address to map to the physical address.
 * @param pPhysicalAddress The physical address to map.
 * @param pPageCount The number of pages to map.
 * @param kLevel The current level in the page directory (used for recusivity).
 * The level in the first call must be the page directory.
 * @param kPageFlags The flags to use for the mapping.
 *
 * @return The function returns the success or error status.
 */
static E_Return _MemoryMapUser(uintptr_t*     pTableLevel,
                               uintptr_t*     pVirtAddress,
                               uintptr_t*     pPhysicalAddress,
                               size_t*        pPageCount,
                               const uint8_t  kLevel,
                               const uint64_t kPageFlags);

/**
 * @brief Unmaps a virtual address in the process user space.
 *
 * @details Unmaps a virtual address in the process user space. The function
 * will check the memory bounds and remove the mapping in the provided
 * process address space.
 *
 * @param pTableLevel The mapped page table level where the memory will be
 * mapped.
 * @param pVirtAddress The virtual address to unmap.
 * @param pPageCount The number of pages to unmap.
 * @param kLevel The current level in the page directory (used for recusivity).
 * The level in the first call must be the page directory.
 *
 * @return The function returns the success or error status.
 */
static E_Return _MemoryUnmapUser(uintptr_t*    pTableLevel,
                                 uintptr_t*    pVirtAddress,
                                 size_t*       pPageCount,
                                 const uint8_t kLevel);

                                 /**
 * @brief Get the reference table and intex in the frames metadata tables.
 *
 * @details Get the reference table and intex in the frames metadata tables.
 * The function fils the ppTable and pEntryIdx buffers with the corresponding
 * values.
 *
 * @param[in] kPhysAddr The physical address for which the reference shall be
 * retrieved.
 * @param[out] ppTable The buffer the receives the pointer to the frame metadata
 * table.
 * @param[out] pEntryIdx The buffer that is filled with the entry index in the
 * frame metadata table that corresponds to the physical address.
 */
static inline void _GetReferenceIndexTable(const uintptr_t        kPhysAddr,
                                           S_FrameMetadataTable** ppTable,
                                           size_t*                pEntryIdx);

/**
 * @brief Get the reference counter pointer for a given physical address and
 * locks the corresponding frame metadata table.
 *
 * @details Get the reference counter pointer for a given physical address and
 * locks the corresponding frame metadata table.
 *
 * @param[in] kPhysAddr The physical address for which the reference count
 * pointer shall be retrieved.
 *
 * @return The function returns the pointer to the reference counter for the
 * provided physical address.
 */
static uint16_t* _GetAndLockReferenceCount(const uintptr_t kPhysAddr);

/**
 * @brief Unlocks the corresponding frame metadata table for a given physical
 * address.
 *
 * @details Unlocks the corresponding frame metadata table for a given physical
 * address.
 *
 * @param[in] kPhysAddr The physical address for which the frame metadata table
 * shall be unlocked.
 */
static void _UnlockReferenceCount(const uintptr_t kPhysAddr);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel symbols mapping: Low AP startup address start. */
extern uint8_t _START_LOW_AP_STARTUP_ADDR;
/** @brief Kernel symbols mapping: High AP startup address start. */
extern uint8_t _END_LOW_AP_STARTUP_ADDR;
/** @brief Kernel symbols mapping: Bios call region address start. */
extern uint8_t _START_BIOS_CALL_ADDR;
/** @brief Kernel symbols mapping: Bios call region address end. */
extern uint8_t _END_BIOS_CALL_ADDR;
/** @brief Kernel symbols mapping: Low startup address start. */
extern uint8_t _START_LOW_STARTUP_ADDR;
/** @brief Kernel symbols mapping: Low startup address end. */
extern uint8_t _END_LOW_STARTUP_ADDR;
/** @brief Kernel symbols mapping: Code address start. */
extern uint8_t _START_TEXT_ADDR;
/** @brief Kernel symbols mapping: Code address end. */
extern uint8_t _END_TEXT_ADDR;
/** @brief Kernel symbols mapping: RO data address start. */
extern uint8_t _START_RO_DATA_ADDR;
/** @brief Kernel symbols mapping: RO data address end. */
extern uint8_t _END_RO_DATA_ADDR;
/** @brief Kernel symbols mapping: RW Data address start. */
extern uint8_t _START_RW_DATA_ADDR;
/** @brief Kernel symbols mapping: RW Data address end. */
extern uint8_t _END_RW_DATA_ADDR;
/** @brief Kernel symbols mapping: Stacks address start. */
extern uint8_t _KERNEL_STACKS_BASE;
/** @brief Kernel symbols mapping: Stacks address end. */
extern uint8_t _KERNEL_STACKS_SIZE;
/** @brief Kernel symbols mapping: Heap address start. */
extern uint8_t _KERNEL_HEAP_BASE;
/** @brief Kernel symbols mapping: Heap address end. */
extern uint8_t _KERNEL_HEAP_SIZE;
/** @brief Kernel symbols mapping: Heap no free address start. */
extern uint8_t _KERNEL_NON_FREE_HEAP_BASE;
/** @brief Kernel symbols mapping: Heap no free address end. */
extern uint8_t _KERNEL_NON_FREE_HEAP_SIZE;
/** @brief Kernel symbols mapping: Kernel total memory start. */
extern uint8_t _KERNEL_MEMORY_START;
/** @brief Kernel symbols mapping: Kernel total memory end. */
extern uint8_t _KERNEL_MEMORY_END;

#ifdef _TESTING_FRAMEWORK_ENABLED
/** @brief Kernel symbols mapping: Test buffer start. */
extern uint8_t _KERNEL_TEST_BUFFER_BASE;
/** @brief Kernel symbols mapping: Test buffer size. */
extern uint8_t _KERNEL_TEST_BUFFER_SIZE;
#endif

/** @brief Kernel page directory intialized at boot */
extern uintptr_t _kernelPGDir[KERNEL_PGDIR_ENTRY_COUNT];

/** @brief Kernel frame-to-page entries */
extern uintptr_t _physicalMapDir[KERNEL_PGDIR_ENTRY_COUNT];

/** @brief Kernel frame-to-page translation table */
extern uintptr_t _physicalMapTranslationPage[KERNEL_PGDIR_ENTRY_COUNT];

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Physical memory chunks list */
static S_MemoryList sPhysMemList;

/** @brief Kernel free page list list */
static S_MemoryList sKernelFreePagesList;

/** @brief Kernel virtual memory bounds */
static S_MemoryRange sKernelVirtualMemBounds;

/** @brief CPU physical addressing width mask */
static uintptr_t sPhysAddressWidthMask = 0;

/** @brief CPU virtual addressing canonical bound */
static uintptr_t sCanonicalBound = 0;

/** @brief Kernel page directory virtual pointer */
static uintptr_t* spKernelPageDir = (uintptr_t*)&_kernelPGDir;

/** @brief Memory manager main lock */
static S_KernelSpinlock sLock = KERNEL_SPINLOCK_INIT_VALUE;

/** @brief Frames metadata tables */
static S_FrameMetadataTable* spFramesMeta = NULL;

/** @brief CPU physical addressing width */
static uint8_t sPhysAddressWidth = 0;

/** @brief CPU virtual addressing width */
static uint8_t sVirtAddressWidth = 0;

/** @brief CPU virtual 1GB page support */
static bool sCpu1GBPageSupport = 0;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static bool _PageFaultHandler(void)
{
  uintptr_t                faultAddress;
  uintptr_t                physAddr;
  uint32_t                 errorCode;
  uint32_t                 flags;
  bool                     staleEntry;
  S_KernelThread*          pCurrentThread;
  S_ProcessMemoryMetadata* pProcData;

  pCurrentThread = SchedulerGetCurrentThread();
  pProcData = (S_ProcessMemoryMetadata*)pCurrentThread->pProcess->pMemoryData;

  /* Get the fault address and error code */
  __asm__ __volatile__ ("mov %%cr2, %0" : "=r"(faultAddress));
  errorCode = ((S_VirtualCPU*)pCurrentThread->pVCpu)->intContext.errorCode;

  /* Check if the fault occured because we hit a stale TLB entry */
  physAddr   = _GetPhysAddr(faultAddress, pProcData->PDPhysAddress, &flags);
  staleEntry = false;
  if (physAddr != MEMMGR_PHYS_ADDR_ERROR)
  {
    staleEntry = true;
    if ((errorCode & PAGE_FAULT_ERROR_PROT_VIOLATION) ==
        PAGE_FAULT_ERROR_PROT_VIOLATION)
    {
      /* Check the privilege level */
      if ((errorCode & PAGE_FAULT_ERROR_USER) == PAGE_FAULT_ERROR_USER &&
          (flags & MEMMGR_MAP_USER) != MEMMGR_MAP_USER)
      {
        staleEntry = false;
      }

      /* Check if execution is allowed */
      if ((errorCode & PAGE_FAULT_ERROR_EXEC) == PAGE_FAULT_ERROR_EXEC &&
          (flags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
      {
        staleEntry = false;
      }

      /* Check the access rights */
      if ((errorCode & PAGE_FAULT_ERROR_WRITE) == PAGE_FAULT_ERROR_WRITE)
      {
        /* Check if the entry is set as COW */
        if ((flags & MEMMGR_MAP_COW) == MEMMGR_MAP_COW)
        {
          /* TODO */
          staleEntry = false;
        }
        /* Check if the error is due to a stale entry */
        else if ((flags & MEMMGR_MAP_RW) != MEMMGR_MAP_RW)
        {
          staleEntry = false;
        }
      }
    }
    else if (errorCode != 0)
    {
      staleEntry = false;
    }
  }

  if (staleEntry != true)
  {
    /* Set reason page fault and reason data the address,
     * also get the reason code in the interrupt info.
     */
    pCurrentThread->errorTable.exceptionId  = PAGE_FAULT_EXC_LINE;
    pCurrentThread->errorTable.segfaultAddr = faultAddress;
    pCurrentThread->errorTable.instAddr     = CPUGetContextIP(pCurrentThread);
    pCurrentThread->errorTable.pExecVCpu    = pCurrentThread->pVCpu;

    SchedulerSetThreadErrored(pCurrentThread);
  }
  else
  {
    CPUInvalidateTLBEntry(faultAddress);
  }

  return !staleEntry;
}

static inline uintptr_t _MakeCanonical(const uintptr_t kAddress,
                                       const bool      kIsPhysical)
{
  if (kIsPhysical == false)
  {
    if ((kAddress & (1ULL << (sVirtAddressWidth - 1))) != 0)
    {
      return kAddress | ~sCanonicalBound;
    }
    else
    {
      return kAddress & sCanonicalBound;
    }
  }
  else
  {
    return kAddress & sPhysAddressWidthMask;
  }
}

static void _AddBlock(S_MemoryList* pList,
                      uintptr_t     baseAddress,
                      const size_t  kLength)
{
  S_KernelQueueNode* pCursor;
  S_KernelQueueNode* pNewNode;
  S_KernelQueueNode* pSaveCursor;
  S_MemoryRange*     pRange;
  S_MemoryRange*     pNextRange;
  uintptr_t          limit;
  bool               merged;

  limit = baseAddress + kLength;

  MEM_ASSERT(pList != NULL,
             "Tried to add a memory block to a NULL list",
             ERR_INVALID_PARAMETER);

  MEM_ASSERT((baseAddress & PAGE_SIZE_MASK) == 0 &&
              (kLength & PAGE_SIZE_MASK) == 0 &&
              kLength != 0,
             "Tried to add a non aligned block",
             ERR_UNAUTHORIZED_ACTION);

  /* Manage rollover */
  MEM_ASSERT(limit > baseAddress,
             "Tried to add a rollover memory block",
             ERR_INVALID_PARAMETER);

  KERNEL_LOCK(pList->lock);

  /* Try to merge the new block, the list is ordered by base address asc */
  pCursor = pList->pQueue->pHead;
  merged = false;
  while (pCursor != NULL)
  {
    pRange = (S_MemoryRange*)pCursor->pData;

    /* If the base address is lower than the new base and the limit is also
      * greather than the new limit, we are adding an already free block.
      */
    MEM_ASSERT((baseAddress < pRange->base &&
                limit <= pRange->base)||
                (baseAddress >= pRange->limit),
                "Adding an already free block",
                ERR_UNAUTHORIZED_ACTION);

    /* If the new block is before but needs merging */
    if (baseAddress < pRange->base && limit == pRange->base)
    {
        /* Extend left */
        pRange->base  = baseAddress;
        pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX - baseAddress;
        merged = true;
    }
    /* If the new block is after but needs merging */
    else if (baseAddress == pRange->limit)
    {
      /* Check if next if not overlapping */
      if (pCursor->pNext != NULL)
      {
        pNextRange = (S_MemoryRange*)pCursor->pNext->pData;

        MEM_ASSERT(pNextRange->base >= limit,
                   "Adding an already free block",
                   ERR_UNAUTHORIZED_ACTION);


        /* See if we can merge this one too */
        if (pNextRange->base == limit)
        {
          pSaveCursor = pCursor;
          pCursor = pCursor->pNext;

          pNextRange->base = pRange->base;
          pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX -
                              pNextRange->base;
          merged = true;


          /* Remove node */
          KFree(pSaveCursor->pData);
          KQueueRemove(pList->pQueue, pSaveCursor);
          KQueueDestroyNode(&pSaveCursor);
        }
      }

      if (merged == false)
      {
        /* Extend up */
        pRange->limit = limit;
        merged        = true;
      }
    }
    else if (baseAddress < pRange->base)
    {
      /* We are before this block, we already checked if not overlapping
        * just stop iterating
        */
      break;
    }

    pCursor = pCursor->pNext;
  }

  /* If not merged, create a new block in the list */
  if (merged == false)
  {
    pRange   = KMalloc(sizeof(S_MemoryRange), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
    pNewNode = KQueueCreateNode(pRange);

    pRange->base  = baseAddress;
    pRange->limit = limit;

    KQueuePushPrio(pNewNode,
                   pList->pQueue,
                   KERNEL_VIRTUAL_ADDR_MAX - baseAddress);

  }

  KERNEL_UNLOCK(pList->lock);
}

static void _RemoveBlock(S_MemoryList*  pList,
                         uintptr_t    baseAddress,
                         const size_t kLength)
{
  S_KernelQueueNode* pCursor;
  S_KernelQueueNode* pNewNode;
  S_KernelQueueNode* pSaveCursor;
  S_MemoryRange*     pRange;
  uintptr_t          limit;
  uintptr_t          saveLimit;

  MEM_ASSERT(pList != NULL,
              "Tried to remove a memory block from a NULL list",
              ERR_INVALID_PARAMETER);

  MEM_ASSERT((baseAddress & PAGE_SIZE_MASK) == 0 &&
              (kLength & PAGE_SIZE_MASK) == 0,
              "Tried to remove a non aligned block",
              ERR_UNAUTHORIZED_ACTION);

  limit = baseAddress + kLength;


  KERNEL_LOCK(pList->lock);

  /* Try to find all the regions that might be impacted */
  pCursor = pList->pQueue->pHead;
  while (pCursor != NULL && limit != 0)
  {
    pRange = (S_MemoryRange*)pCursor->pData;

    /* Check if fully contained */
    if (pRange->base >= baseAddress && pRange->limit <= limit)
    {
      pSaveCursor = pCursor;
      pCursor = pCursor->pNext;

      /* Update the rest to remove */
      baseAddress = pRange->limit;
      if (limit == pRange->limit)
      {
          limit = 0;
      }

      KFree(pSaveCursor->pData);
      KQueueRemove(pList->pQueue, pSaveCursor);
      KQueueDestroyNode(&pSaveCursor);
    }
    /* If up containted */
    else if (pRange->base < baseAddress && pRange->limit <= limit)
    {
      pRange->limit = baseAddress;

      /* Update the rest to remove */
      if (limit == pRange->limit)
      {
          limit = 0;
      }
      else
      {
          baseAddress = pRange->limit;
      }
      pCursor = pCursor->pNext;
    }
    /* If down containted */
    else if (pRange->base >= baseAddress && pRange->limit > limit)
    {
      /* Update the rest to remove */
      pRange->base = limit;

      /* We are done*/
      limit = 0;
    }
    /* If inside */
    else if (pRange->base < baseAddress && pRange->limit > limit)
    {
      /* Save next limit */
      saveLimit = pRange->limit;

      /* Update the current region */
      pRange->limit = baseAddress;

      /* Get new base address */
      baseAddress = limit;

      /* Create new node */
      pRange = KMalloc(sizeof(S_MemoryRange), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
      MEM_ASSERT(pRange != NULL,
                  "Failed to allocate new memory range",
                  ERR_NO_MEMORY);

      pNewNode = KQueueCreateNode(pRange);

      pRange->base  = baseAddress;
      pRange->limit = saveLimit;

      KQueuePushPrio(pNewNode,
                      pList->pQueue,
                      KERNEL_VIRTUAL_ADDR_MAX - baseAddress);

      /* We are done*/
      limit = 0;
    }
    else
    {
      pCursor = pCursor->pNext;
    }
  }

  KERNEL_UNLOCK(pList->lock);
}

static uintptr_t _GetBlock(S_MemoryList* pList, const size_t kLength)
{
  uintptr_t          retBlock;
  S_KernelQueueNode* pCursor;
  S_MemoryRange*     pRange;

  MEM_ASSERT((kLength & PAGE_SIZE_MASK) == 0,
              "Tried to get a non aligned block",
              ERR_UNAUTHORIZED_ACTION);

  retBlock = 0;

  KERNEL_LOCK(pList->lock);

  /* Walk the list until we find a valid block */
  pCursor = pList->pQueue->pHead;
  while (pCursor != NULL)
  {
    pRange = (S_MemoryRange*)pCursor->pData;

    if (pRange->base + kLength <= pRange->limit ||
        ((pRange->base + kLength > pRange->base) && pRange->limit == 0))
    {
      retBlock = pRange->base;

      /* Reduce the node or remove it */
      if (pRange->base + kLength == pRange->limit)
      {
        KFree(pCursor->pData);
        KQueueRemove(pList->pQueue, pCursor);
        KQueueDestroyNode(&pCursor);
      }
      else
      {
        pRange->base += kLength;
        pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX - pRange->base;
      }
      break;
    }

    pCursor = pCursor->pNext;
  }

  KERNEL_UNLOCK(pList->lock);

  return retBlock;
}

static uintptr_t _GetBlockFromEnd(S_MemoryList* pList, const size_t kLength)
{
  uintptr_t          retBlock;
  S_KernelQueueNode* pCursor;
  S_MemoryRange*     pRange;

  MEM_ASSERT((kLength & PAGE_SIZE_MASK) == 0,
              "Tried to get a non aligned block",
              ERR_UNAUTHORIZED_ACTION);

  retBlock = 0;

  KERNEL_LOCK(pList->lock);

  /* Walk the list until we find a valid block */
  pCursor = pList->pQueue->pTail;
  while (pCursor != NULL)
  {
    pRange = (S_MemoryRange*)pCursor->pData;

    if (pRange->base + kLength <= pRange->limit ||
        ((pRange->base + kLength > pRange->base) && pRange->limit == 0))
    {
      retBlock = pRange->limit - kLength;

      /* Reduce the node or remove it */
      if (pRange->base + kLength == pRange->limit)
      {
        KFree(pCursor->pData);
        KQueueRemove(pList->pQueue, pCursor);
        KQueueDestroyNode(&pCursor);
      }
      else
      {
        pRange->limit -= kLength;
      }
      break;
    }

    pCursor = pCursor->pPrev;
}

  KERNEL_UNLOCK(pList->lock);

  return retBlock;
}

static uintptr_t _AllocateFrames(const size_t kFrameCount)
{
  uintptr_t physAddr;
  uintptr_t baseAddr;
  uintptr_t i;
  uint16_t* refCount;

  physAddr = _GetBlock(&sPhysMemList, KERNEL_PAGE_SIZE * kFrameCount);
  MEM_ASSERT((physAddr & PAGE_SIZE_MASK) == 0,
              "Non aligned frame allocated.",
              ERR_INVALID_PARAMETER);
  baseAddr = physAddr;
  if (physAddr != (uintptr_t)NULL)
  {
    /* Increment the reference count */
    for (i = 0; i < kFrameCount; ++i)
    {
      refCount = _GetAndLockReferenceCount(physAddr);
      if (refCount != NULL)
      {
        MEM_ASSERT(*refCount == 0,
                   "Invalid reference count non zero",
                   ERR_UNAUTHORIZED_ACTION);
        *refCount = 1;
        _UnlockReferenceCount(physAddr);
      }
      physAddr += KERNEL_PAGE_SIZE;
    }
  }

  return baseAddr;
}

static void _ReleaseFrames(const uintptr_t kBaseAddress,
                           const size_t    kFrameCount)
{
  uintptr_t physAddr;
  uintptr_t i;
  uint16_t* refCount;

  physAddr = kBaseAddress;

  /* Increment the reference count */
  for (i = 0; i < kFrameCount; ++i)
  {
    refCount = _GetAndLockReferenceCount(physAddr);
    if (refCount != NULL)
    {
      MEM_ASSERT(*refCount == 1, "Release used frame", ERR_UNAUTHORIZED_ACTION);
      *refCount = 0;
    }
    _UnlockReferenceCount(physAddr);
    physAddr += KERNEL_PAGE_SIZE;
  }

  _AddBlock(&sPhysMemList, kBaseAddress, kFrameCount * KERNEL_PAGE_SIZE);
}

static uintptr_t _AllocateKernelPages(const size_t kPageCount)
{
  uintptr_t page;

  page =  _GetBlock(&sKernelFreePagesList, kPageCount * KERNEL_PAGE_SIZE);
  MEM_ASSERT((page & PAGE_SIZE_MASK) == 0,
             "Non aligned page allocated.",
             ERR_UNAUTHORIZED_ACTION);

  return page;
}

static void _ReleaseKernelPages(const uintptr_t kBaseAddress,
                                const size_t    kPageCount)
{
  _AddBlock(&sKernelFreePagesList, kBaseAddress, kPageCount * KERNEL_PAGE_SIZE);
}

static bool _IsMapped(const uintptr_t kVirtualAddress,
                               size_t          pageCount,
                               const uintptr_t kPageDir,
                               const bool      kCheckFull)
{
  uintptr_t  currVirtAddr;
  uintptr_t  nextPtable;
  uintptr_t* pPageTable[4];
  uint16_t   pmlEntry[4];
  int8_t     j;
  size_t     stride;
  bool       mapped;

  MEM_ASSERT((kVirtualAddress & PAGE_SIZE_MASK) == 0,
             "Checking mapping for non aligned address",
             ERR_INVALID_PARAMETER);

  mapped = false;
  if (pageCount != 0)
  {

    currVirtAddr = kVirtualAddress;
    do
    {
      pmlEntry[3] = (currVirtAddr >> PML4_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
      pmlEntry[2] = (currVirtAddr >> PML3_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
      pmlEntry[1] = (currVirtAddr >> PML2_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
      pmlEntry[0] = (currVirtAddr >> PML1_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;

      for (j = 3; j >= 0; --j)
      {
        if (j != 3)
        {
          nextPtable = _MakeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                      ~PAGE_SIZE_MASK,
                                      true);
          pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
        }
        else
        {
          pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
        }


        if (j != 0 && (pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) == 0)
        {
            /* If the check is a full check and we have an unmapped
             * region, return false
             */
            if (kCheckFull == true)
            {
              mapped    = false;
              pageCount = 0;
              break;
            }

            /* Check next level entry and either zeroise or set the
             * page
             */
            if (j == 3)
            {
              stride = ((KERNEL_PGDIR_ENTRY_COUNT -
                        (pmlEntry[1] + 1)) *
                        KERNEL_PGDIR_ENTRY_COUNT *
                        KERNEL_PGDIR_ENTRY_COUNT) +
                        ((KERNEL_PGDIR_ENTRY_COUNT -
                          (pmlEntry[2] + 1)) *
                        KERNEL_PGDIR_ENTRY_COUNT) +
                        KERNEL_PGDIR_ENTRY_COUNT - pmlEntry[1];

              currVirtAddr += KERNEL_PAGE_SIZE * stride;
              pageCount -= MIN(pageCount, stride);

              /* We are done with the rest of the hierarchy */
              j = -1;
            }
            else if (j == 2)
            {
              stride = ((KERNEL_PGDIR_ENTRY_COUNT -
                        (pmlEntry[2] + 1)) * KERNEL_PGDIR_ENTRY_COUNT) +
                      KERNEL_PGDIR_ENTRY_COUNT - pmlEntry[1];

              currVirtAddr += KERNEL_PAGE_SIZE * stride;
              pageCount -= MIN(pageCount, stride);

              /* We are dones with the rest of the hierarchy */
              j = -1;
            }
            else if (j == 1)
            {
              stride = KERNEL_PGDIR_ENTRY_COUNT - pmlEntry[1];
              currVirtAddr += KERNEL_PAGE_SIZE * stride;
              pageCount -= MIN(pageCount, stride);

              /* We are dones with the rest of the hierarchy */
              j = -1;
            }
        }
        else if (j == 0)
        {
          do
          {
            if ((pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) == 0)
            {
              /* If the check is a full check and we have an unmapped
                * region, return false
                */
              if (kCheckFull == true)
              {
                mapped    = false;
                pageCount = 0;
                break;
              }
            }
            else if (kCheckFull == false || pageCount == 1)
            {
              /* If the check is not a full check and we have a mapp
              * partial region, return true
              */
              mapped    = true;
              pageCount = 0;
              break;
            }

            currVirtAddr += KERNEL_PAGE_SIZE;
            --pageCount;
            ++pmlEntry[0];
          } while (pageCount > 0 && pmlEntry[0] < KERNEL_PGDIR_ENTRY_COUNT);
        }
      }
    } while (pageCount > 0);
  }

  return mapped;
}

static E_Return _MemoryMap(const uintptr_t kVirtualAddress,
                           const uintptr_t kPhysicalAddress,
                           const size_t    kPageCount,
                           const uint32_t  kFlags,
                           const uintptr_t kPageDir)
{
  size_t       toMap;
  int8_t       j;
  uint64_t     mapFlags;
  uint64_t     mapPgdirFlags;
  bool         isMapped;
  uintptr_t    currVirtAddr;
  uintptr_t    currPhysAdd;
  uintptr_t    newPgTableFrame;
  uintptr_t*   pPageTable[4];
  uint16_t     pmlEntry[4];
  uintptr_t    nextPtable;

  /* Check the alignements */
  if ((kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
      (kPhysicalAddress & PAGE_SIZE_MASK) != 0 ||
      kPageCount == 0)
  {
    return ERR_INVALID_PARAMETER;
  }

  /* Check the canonical address */
  if ((kVirtualAddress & ~sCanonicalBound) != 0)
  {
    if ((kVirtualAddress & ~sCanonicalBound) != ~sCanonicalBound)
    {
      return ERR_INVALID_PARAMETER;
    }
  }

  if ((kPhysicalAddress & ~sPhysAddressWidthMask) != 0)
  {
    return ERR_INVALID_PARAMETER;
  }

  /* Check if the mapping already exists, check if we need to update one or
   * more page directory entries
   */
  isMapped = _IsMapped(kVirtualAddress, kPageCount, kPageDir, false);
  if (isMapped == true)
  {
    return ERR_UNAUTHORIZED_ACTION;
  }

  /* Get the flags */
  mapFlags = PAGE_FLAG_PRESENT | _TranslateFlags(kFlags);

  mapPgdirFlags = PAGE_FLAG_SUPER_ACCESS  |
                  PAGE_FLAG_USER_ACCESS   |
                  PAGE_FLAG_READ_WRITE    |
                  PAGE_FLAG_CACHE_WB      |
                  PAGE_FLAG_XD            |
                  PAGE_FLAG_PRESENT;

  /* Apply the mapping */
  toMap        = kPageCount;
  currVirtAddr = kVirtualAddress;
  currPhysAdd  = kPhysicalAddress;

  while (toMap != 0)
  {
    pmlEntry[3] = (currVirtAddr >> PML4_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
    pmlEntry[2] = (currVirtAddr >> PML3_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
    pmlEntry[1] = (currVirtAddr >> PML2_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
    pmlEntry[0] = (currVirtAddr >> PML1_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;

    /* Setup entry in the four levels is needed  */
    for (j = 3; j >= 0; --j)
    {
      if (j != 3)
      {
        nextPtable = _MakeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                    ~PAGE_SIZE_MASK,
                                    true);
        pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
      }
      else
      {
        pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
      }

      if (j != 0 && (pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) == 0)
      {
        /* Allocate a new frame and map to temporary boot entry */
        newPgTableFrame = _AllocateFrames(1);
        MEM_ASSERT(newPgTableFrame != 0,
                   "Allocated a NULL frame",
                   ERR_INVALID_PARAMETER);

        pPageTable[j][pmlEntry[j]] = (newPgTableFrame & sPhysAddressWidthMask) |
                                     mapPgdirFlags;

        /* Zeroise the page */
        memset((void*)GET_VIRT_MEM_ADDR(newPgTableFrame), 0, KERNEL_PAGE_SIZE);
      }
      else if (j == 0)
      {
        /* Map as much as we can in this page table */
        do
        {
          /* Set mapping and invalidate */
          pPageTable[j][pmlEntry[0]] = (currPhysAdd & sPhysAddressWidthMask) |
                                       mapFlags;
          CPUInvalidateTLBEntry(currVirtAddr);

          currVirtAddr += KERNEL_PAGE_SIZE;
          currPhysAdd  += KERNEL_PAGE_SIZE;
          --toMap;
          ++pmlEntry[0];
        } while (toMap > 0 && pmlEntry[0] != KERNEL_PGDIR_ENTRY_COUNT);
      }
    }
  }

  return NO_ERROR;
}

static E_Return _MemoryUnmap(const uintptr_t kVirtualAddress,
                             const size_t    kPageCount,
                             const uintptr_t kPageDir)
{
  size_t          toUnmap;
  bool            hasMapping;
  uint16_t        i;
  int8_t          j;
  uintptr_t       currVirtAddr;
  uintptr_t       nextPtable;
  uintptr_t*      pPageTable[4];
  uint16_t        pmlEntry[4];
  uintptr_t       physAddr;
  S_IPIParameters ipiParams;

  /* Check the alignements */
  if ((kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
      kPageCount == 0)
  {
    return ERR_INVALID_PARAMETER;
  }

  /* Check the canonical address */
  if ((kVirtualAddress & ~sCanonicalBound) != 0)
  {
    if ((kVirtualAddress & ~sCanonicalBound) !=
        ~sCanonicalBound)
    {
      return ERR_INVALID_PARAMETER;
    }
  }

  /* Check if the mapping already exists, check if we need to update one or
    * more page directory entries
    */
  hasMapping = _IsMapped(kVirtualAddress, kPageCount, kPageDir, true);
  if (hasMapping == false)
  {
    return ERR_UNAUTHORIZED_ACTION;
  }

  /* Apply the unmapping */
  toUnmap = kPageCount;
  currVirtAddr = kVirtualAddress;

  ipiParams.function = IPI_FUNC_TLB_INVAL;

  while (toUnmap != 0)
  {
    /* Skip unmapped regions */
    hasMapping = false;

    pmlEntry[3] = (currVirtAddr >> PML4_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[2] = (currVirtAddr >> PML3_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[1] = (currVirtAddr >> PML2_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[0] = (currVirtAddr >> PML1_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;

    /* Get the memory mapping */
    for (j = 3; j >= 0; --j)
    {
      if (j != 3)
      {
        nextPtable = _MakeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                     ~PAGE_SIZE_MASK,
                                    true);
        pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
      }
      else
      {
        pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
      }
    }

    /* Remove entry in the four levels is needed  */
    for (j = 0; j < 3 && toUnmap > 0; ++j)
    {
      /* If first level, unmap */
      if (j == 0)
      {
        do
        {
          pPageTable[j][pmlEntry[0]] = 0;

          CPUInvalidateTLBEntry(currVirtAddr);

          /* Update other cores TLB */
          ipiParams.pData = (void*)currVirtAddr;
          CPUSendIPI(CPU_IPI_BROADCAST_TO_OTHER, &ipiParams);

          currVirtAddr += KERNEL_PAGE_SIZE;
          --toUnmap;
          ++pmlEntry[0];
        } while (toUnmap > 0 && pmlEntry[0] != KERNEL_PGDIR_ENTRY_COUNT);

        /* Check if we can clean this directory entries */
        hasMapping = false;
        for (i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
        {
          if ((pPageTable[j][i] & PAGE_FLAG_PRESENT) != 0)
          {
            hasMapping = true;
            break;
          }
        }

        if (hasMapping == false)
        {
          /* Release the frames */
          physAddr = _MakeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                     ~PAGE_SIZE_MASK,
                                    true);
          _ReleaseFrames(physAddr, 1);

          /* Set the entry as unmapped in the previous level */
          pPageTable[j + 1][pmlEntry[j + 1]] = 0;
        }
      }
      else
      {
        /* Check if we can clean this directory entries */
        for (i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
        {
          if ((pPageTable[j + 1][i] & PAGE_FLAG_PRESENT) != 0)
          {
            hasMapping = true;
            break;
          }
        }

        if (hasMapping == false)
        {
          /* Release the frames */
          physAddr = _MakeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                     ~PAGE_SIZE_MASK,
                                    true);
          _ReleaseFrames(physAddr, 1);

          /* Set the entry as unmapped in the previous level */
          pPageTable[j + 1][pmlEntry[j + 1]] = 0;
        }
      }
    }
  }

  return NO_ERROR;
}

static void _ReleasePageDir(const uintptr_t kPhysTable,
                            const uintptr_t kBaseVirtAddr,
                            const uint8_t   kLevel)
{
  uintptr_t   frameAddr;
  uintptr_t*  currentLevelPage;
  uintptr_t   virtAddr;
  uintptr_t   levelAddrCount;
  uint32_t    i;
  uint16_t*   refCount;

  MEM_ASSERT(kLevel > 0,
              "Invalid page directory level in release",
              ERR_INVALID_PARAMETER);

  /* Allocate frames for mapping */
  currentLevelPage = (uintptr_t*)GET_VIRT_MEM_ADDR(kPhysTable);

  /* Get the address increase based on the level */
  switch(kLevel)
  {
    /* PML4 */
    case 4:
      levelAddrCount = 1ULL << PML4_ENTRY_OFFSET;
      break;
    /* PML3 */
    case 3:
      levelAddrCount = 1ULL << PML3_ENTRY_OFFSET;
      break;
    /* PML2 */
    case 2:
      levelAddrCount = 1ULL << PML2_ENTRY_OFFSET;
      break;
    /* PML 1 */
    case 1:
      levelAddrCount = 1ULL << PML1_ENTRY_OFFSET;
      break;
    default:
      levelAddrCount = 0;
      MEM_ASSERT(false,
                 "Invalid page directory level in release",
                 ERR_INVALID_PARAMETER);
  }

  /* Check all entries of the current table */
  if (kLevel == 1)
  {
    for (i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
    {
      virtAddr = kBaseVirtAddr + i * levelAddrCount;

      /* Check if we still are in the low kernel
       * space
       */
      if (virtAddr < USER_MEMORY_START)
      {
        /* We do not release low-memory kernel frames */
        continue;
      }
      /* Check if we are in the high kernel space*/
      else if (virtAddr >= USER_MEMORY_END)
      {
        /* The next address will not need any release */
        break;
      }
      else
      {
        /* If present and not hardware, release the frame */
        if ((currentLevelPage[i] &
            (PAGE_FLAG_PRESENT | PAGE_FLAG_IS_HW)) == PAGE_FLAG_PRESENT)
        {
            frameAddr = _MakeCanonical(currentLevelPage[i] &  ~PAGE_SIZE_MASK,
                                       true);

            /* Decrease the reference count */
            refCount = _GetAndLockReferenceCount(frameAddr);
            MEM_ASSERT(*refCount > 0,
                       "Invalid reference count zero",
                        ERR_UNAUTHORIZED_ACTION);
            *refCount = *refCount - 1;

            /* If not one uses the frame, release the frame */
            if (*refCount == 0)
            {
              _ReleaseFrames(frameAddr, 1);
            }
            _UnlockReferenceCount(frameAddr);
        }
      }
    }
  }
  else
  {
    for (i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
    {
      virtAddr = kBaseVirtAddr + i * levelAddrCount;

      /* Check if we are in the high kernel space*/
      if (virtAddr >= USER_MEMORY_END)
      {
        /* The next address will not need any release */
        break;
      }

      /* If present, got to next level */
      if ((currentLevelPage[i] & PAGE_FLAG_PRESENT) == PAGE_FLAG_PRESENT)
      {
        virtAddr = kBaseVirtAddr + (uintptr_t)i * levelAddrCount;
        frameAddr = _MakeCanonical(currentLevelPage[i] & ~PAGE_SIZE_MASK,
                                   true);

        /* Release the next level if not in kernel zone */
        _ReleasePageDir(frameAddr, virtAddr, kLevel - 1);
      }
    }
  }

  /* Release the page table */
  _ReleaseFrames(kPhysTable, 1);
}

static uintptr_t _GetPhysAddr(const uintptr_t kVirtualAddress,
                              const uintptr_t kPageDir,
                              uint32_t*       pFlags)
{
  uintptr_t  retPhysAddr;
  uintptr_t  nextPtable;
  uintptr_t* pPageTable[4];
  uint16_t   pmlEntry[4];
  int8_t     j;

  retPhysAddr = MEMMGR_PHYS_ADDR_ERROR;

  pmlEntry[3] = (kVirtualAddress >> PML4_ENTRY_OFFSET) &
                  PG_ENTRY_OFFSET_MASK;
  pmlEntry[2] = (kVirtualAddress >> PML3_ENTRY_OFFSET) &
                  PG_ENTRY_OFFSET_MASK;
  pmlEntry[1] = (kVirtualAddress >> PML2_ENTRY_OFFSET) &
                  PG_ENTRY_OFFSET_MASK;
  pmlEntry[0] = (kVirtualAddress >> PML1_ENTRY_OFFSET) &
                  PG_ENTRY_OFFSET_MASK;

  for (j = 3; j >= 0; --j)
  {
    if (j != 3)
    {
      nextPtable = _MakeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                   ~PAGE_SIZE_MASK,
                                  true);
      pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
    }
    else
    {
      pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
    }

    if ((pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) != 0)
    {
      if (j == 0)
      {
        if (pFlags != NULL)
        {
          retPhysAddr = pPageTable[j][pmlEntry[j]];
          *pFlags     = MEMMGR_MAP_KERNEL;

          if ((retPhysAddr & PAGE_FLAG_READ_WRITE) == PAGE_FLAG_READ_WRITE)
          {
            *pFlags |= MEMMGR_MAP_RW;
          }
          else
          {
            *pFlags |= MEMMGR_MAP_RO;
          }

          if ((retPhysAddr & PAGE_FLAG_XD) != PAGE_FLAG_XD)
          {
            *pFlags |= MEMMGR_MAP_EXEC;
          }

          if ((retPhysAddr & PAGE_FLAG_USER_ACCESS) == PAGE_FLAG_USER_ACCESS)
          {
            *pFlags |= MEMMGR_MAP_USER;
          }

          if ((retPhysAddr & PAGE_FLAG_CACHE_DISABLED) ==
              PAGE_FLAG_CACHE_DISABLED)
          {
            *pFlags |= MEMMGR_MAP_CACHE_DISABLED;
          }
          if ((retPhysAddr & PAGE_FLAG_IS_HW) == PAGE_FLAG_IS_HW)
          {
            *pFlags |= MEMMGR_MAP_HARDWARE;
          }
          if ((retPhysAddr & PAGE_FLAG_COW) == PAGE_FLAG_COW)
          {
            *pFlags |= MEMMGR_MAP_COW;
          }

          retPhysAddr = (retPhysAddr & sPhysAddressWidthMask) & ~PAGE_SIZE_MASK;
        }
        else
        {
          retPhysAddr = (pPageTable[j][pmlEntry[j]] & sPhysAddressWidthMask) &
                        ~PAGE_SIZE_MASK;
        }
      }
    }
    else
    {
      break;
    }
  }

  if (retPhysAddr != MEMMGR_PHYS_ADDR_ERROR)
  {
    retPhysAddr |= kVirtualAddress & PAGE_SIZE_MASK;
  }

  return retPhysAddr;
}

static void _DetectMemory(void)
{
  uintptr_t              baseAddress;
  size_t                size;
  size_t                 initSize;
  uintptr_t              kernelPhysStart;
  uintptr_t              kernelPhysEnd;
  const S_FDTMemoryNode* kpPhysMemNode;
  const S_FDTMemoryNode* kpResMemNode;
  const S_FDTMemoryNode* kpCursor;

  kpPhysMemNode = FDTGetMemory();
  MEM_ASSERT(kpPhysMemNode != NULL,
              "No physical memory detected in FDT",
              ERR_NO_MEMORY);

  /* Now iterate on all memory nodes and add the regions */
  while (kpPhysMemNode != NULL)
  {
    /* Align the base address and size */
    baseAddress = ALIGN_UP(FDTTOCPU64(kpPhysMemNode->baseAddress),
                           KERNEL_PAGE_SIZE);
    initSize = baseAddress - FDTTOCPU64(kpPhysMemNode->baseAddress);
    initSize = ALIGN_DOWN(FDTTOCPU64(kpPhysMemNode->size) - initSize,
                          KERNEL_PAGE_SIZE);

    MEM_ASSERT(baseAddress + initSize <= KERNEL_MAX_MEM_PHYS,
               "Kernel does not support physical memory over 511GB",
               ERR_NOT_SUPPORTED);

    /* Add block to the free frames */
    _AddBlock(&sPhysMemList, baseAddress, initSize);

    /* Go to next node */
    kpPhysMemNode = kpPhysMemNode->pNextNode;
  }

  /* Remove reserved memory */
  kpResMemNode = FDTGetReservedMemory();
  kpCursor = kpResMemNode;
  while (kpCursor != NULL)
  {
    baseAddress = ALIGN_DOWN(FDTTOCPU64(kpCursor->baseAddress),
                             KERNEL_PAGE_SIZE);
    size = ALIGN_UP(FDTTOCPU64(kpCursor->size), KERNEL_PAGE_SIZE);

    _RemoveBlock(&sPhysMemList, baseAddress, size);

    kpCursor = kpCursor->pNextNode;
  }

  /* Get kernel bounds */
  kernelPhysStart = (uintptr_t)&_KERNEL_MEMORY_START;
#ifdef _TESTING_FRAMEWORK_ENABLED
  /* If testing is enabled, the end is after its buffer */
  kernelPhysEnd = (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                  (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE;
#else
  kernelPhysEnd   = (uintptr_t)&_KERNEL_MEMORY_END;
#endif

  /* Get actual physical address */
  kernelPhysStart = ALIGN_DOWN(kernelPhysStart - KERNEL_MEM_OFFSET,
                               KERNEL_PAGE_SIZE);
  kernelPhysEnd   = ALIGN_UP(kernelPhysEnd - KERNEL_MEM_OFFSET,
                             KERNEL_PAGE_SIZE);

  /* Remove the kernel physical memory */
  _RemoveBlock(&sPhysMemList, kernelPhysStart, kernelPhysEnd - kernelPhysStart);
}

static uintptr_t _MapTranslationTable(void)
{
  uintptr_t              baseAddress;
  size_t                 size;
  size_t                 initSize;
  size_t                 frameCount;
  size_t                 nbEntries;
  uintptr_t              frameTable;
  uintptr_t              physFrameTable;
  const S_FDTMemoryNode* kpPhysMemNode;

  kpPhysMemNode = FDTGetMemory();
  MEM_ASSERT(kpPhysMemNode != NULL,
              "No physical memory detected in FDT",
              ERR_NO_MEMORY);

  /* Compute the memory needed for the flat map */
  size = 0;
  while (kpPhysMemNode != NULL)
  {
    /* Align the base address and size */
    baseAddress = ALIGN_DOWN(FDTTOCPU64(kpPhysMemNode->baseAddress),
                             KERNEL_MEM_2MB);
    initSize    = FDTTOCPU64(kpPhysMemNode->baseAddress) - baseAddress;
    initSize    = ALIGN_UP(FDTTOCPU64(kpPhysMemNode->size) + initSize,
                           KERNEL_MEM_2MB);

    MEM_ASSERT(baseAddress + initSize <= KERNEL_MAX_MEM_PHYS,
               "Kernel does not support physical memory over 511GB",
               ERR_NOT_SUPPORTED);

    size += initSize;

    /* Go to next node */
    kpPhysMemNode = kpPhysMemNode->pNextNode;
  }

  /* Get the number of frames needed for the mapping */
  nbEntries = (size / KERNEL_MEM_2MB);
  if (size % KERNEL_MEM_2MB != 0)
  {
    ++nbEntries;
  }

  /* Get the frames count */
  frameCount = nbEntries / KERNEL_PGDIR_ENTRY_COUNT;
  if (nbEntries % KERNEL_PGDIR_ENTRY_COUNT != 0)
  {
    ++frameCount;
  }
  MEM_ASSERT(frameCount < 512,
             "Kernel does not support physical memory over 511GB",
             ERR_NOT_SUPPORTED);

  /* Get a memory block aligned on 2MB */
  physFrameTable = _GetBlock(&sPhysMemList, KERNEL_PAGE_SIZE);
  frameTable = physFrameTable;
  while ((frameTable & (KERNEL_MEM_2MB - 1)) != 0)
  {
    frameTable = _GetBlock(&sPhysMemList, KERNEL_PAGE_SIZE);
  }
  /* Get the rest of the frames */
  if (frameCount > 1)
  {
    (void)_GetBlock(&sPhysMemList, KERNEL_PAGE_SIZE * (frameCount - 1));
  }
  /* Relinquishes the rest of the unused frames */
  if (frameTable != physFrameTable)
  {
    _AddBlock(&sPhysMemList, physFrameTable, frameTable - physFrameTable);
  }
  MEM_ASSERT(frameTable != (uintptr_t)NULL,
             "Not enough memory to support the physical memory.",
             ERR_NO_MEMORY);

  /* Map the table */
  physFrameTable = (uintptr_t)_physicalMapTranslationPage - KERNEL_MEM_OFFSET;
  _physicalMapDir[KERNEL_PGDIR_ENTRY_COUNT - 1] = physFrameTable          |
                                                  PAGE_FLAG_SUPER_ACCESS  |
                                                  PAGE_FLAG_CACHE_WB      |
                                                  PAGE_FLAG_READ_WRITE    |
                                                  PAGE_FLAG_XD            |
                                                  PAGE_FLAG_PRESENT;
  _physicalMapTranslationPage[0] = frameTable              |
                                   PAGE_FLAG_PAGE_SIZE_2MB |
                                   PAGE_FLAG_SUPER_ACCESS  |
                                   PAGE_FLAG_CACHE_WB      |
                                   PAGE_FLAG_READ_WRITE    |
                                   PAGE_FLAG_GLOBAL        |
                                   PAGE_FLAG_XD            |
                                   PAGE_FLAG_PRESENT;
  physFrameTable = KERNEL_MEM_PML4_ENTRY * 512ULL * KERNEL_MEM_1GB +
                    511ULL * KERNEL_MEM_2MB;
  CPUInvalidateTLBEntry(physFrameTable);

  /* Return the first usable address */
  return frameTable;
}

static void _CreateFlatMap(void)
{
  uintptr_t              baseAddress;
  size_t                 size;
  size_t                 initSize;
  size_t                 usedFrames;
  uintptr_t              frameEntry;
  uintptr_t              frameTable;
  uintptr_t              nextPtable;
  uintptr_t*             pTable;
  uintptr_t              startTranslationAddr;
  const S_FDTMemoryNode* kpPhysMemNode;

  /* Get the physical memory */
  kpPhysMemNode = FDTGetMemory();
  MEM_ASSERT(kpPhysMemNode != NULL,
             "No physical memory detected in FDT",
             ERR_NO_MEMORY);

  if (sCpu1GBPageSupport == false)
  {
    /* Map the translation pages */
    startTranslationAddr = _MapTranslationTable();
    usedFrames           = 0;

    /* Now iterate on all memory nodes and add the regions */
    while (kpPhysMemNode != NULL)
    {
      /* Align the base address and size */
      baseAddress = ALIGN_DOWN(FDTTOCPU64(kpPhysMemNode->baseAddress),
                               KERNEL_MEM_2MB);
      initSize    = FDTTOCPU64(kpPhysMemNode->baseAddress) - baseAddress;
      initSize    = ALIGN_UP(FDTTOCPU64(kpPhysMemNode->size) + initSize,
                             KERNEL_MEM_2MB);

      /* Add to the page-to-frame directory */
      size = 0;
      /* Map the physical by split of 2MB */
      while (size < initSize)
      {
        /* Get the frame table and entry */
        frameTable = (baseAddress + size) / KERNEL_MEM_1GB;
        frameEntry = ((baseAddress + size) % KERNEL_MEM_1GB) / KERNEL_MEM_2MB;

        /* If not present, add a new frame */
        if ((_physicalMapDir[frameTable] & PAGE_FLAG_PRESENT) == 0)
        {
          /* Get the frame and increment */
          nextPtable = startTranslationAddr + usedFrames * KERNEL_PAGE_SIZE;
          ++usedFrames;
          nextPtable = _MakeCanonical(nextPtable, true);

          _physicalMapDir[frameTable] = nextPtable             |
                                        PAGE_FLAG_SUPER_ACCESS |
                                        PAGE_FLAG_CACHE_WB     |
                                        PAGE_FLAG_READ_WRITE   |
                                        PAGE_FLAG_XD           |
                                        PAGE_FLAG_PRESENT;
        }
        /* Get the virtual address of the table to update */
        nextPtable = _MakeCanonical((_physicalMapDir[frameTable] &
                                      ~PAGE_SIZE_MASK) -
                                    startTranslationAddr,
                                    true);
        nextPtable = KERNEL_MEM_PML4_ENTRY * 512ULL *
                      KERNEL_MEM_1GB +
                      511ULL * KERNEL_MEM_2MB * 512ULL +
                      nextPtable;
        pTable = (uintptr_t*)_MakeCanonical(nextPtable, false);
        pTable[frameEntry] = ((frameTable * KERNEL_MEM_1GB) +
                              (frameEntry * KERNEL_MEM_2MB)) |
                              PAGE_FLAG_PAGE_SIZE_2MB         |
                              PAGE_FLAG_SUPER_ACCESS          |
                              PAGE_FLAG_CACHE_WB              |
                              PAGE_FLAG_READ_WRITE            |
                              PAGE_FLAG_GLOBAL                |
                              PAGE_FLAG_XD                    |
                              PAGE_FLAG_PRESENT;

        size += KERNEL_MEM_2MB;
      }

      /* Go to next node */
      kpPhysMemNode = kpPhysMemNode->pNextNode;
    }
  }
  else
  {
    /* Now iterate on all memory nodes and add the regions */
    while (kpPhysMemNode != NULL)
    {
      /* Align the base address and size */
      baseAddress = ALIGN_DOWN(FDTTOCPU64(kpPhysMemNode->baseAddress),
                               KERNEL_MEM_1GB);
      initSize    = FDTTOCPU64(kpPhysMemNode->baseAddress) - baseAddress;
      initSize    = ALIGN_UP(FDTTOCPU64(kpPhysMemNode->size) + initSize,
                             KERNEL_MEM_1GB);

      MEM_ASSERT(baseAddress + initSize <= KERNEL_MAX_MEM_PHYS,
              "Kernel does not support physical memory over 511GB",
              ERR_NOT_SUPPORTED);

      /* Add to the page-to-frame directory */
      size = 0;
      /* Map by pages of 1GB */
      while (size < initSize)
      {
        frameEntry = (baseAddress + size) / KERNEL_MEM_1GB;
        if ((_physicalMapDir[frameEntry] & PAGE_FLAG_PRESENT) == 0)
        {
          _physicalMapDir[frameEntry] = (frameEntry * KERNEL_MEM_1GB) |
                                        PAGE_FLAG_PAGE_SIZE_1GB       |
                                        PAGE_FLAG_SUPER_ACCESS        |
                                        PAGE_FLAG_CACHE_WB            |
                                        PAGE_FLAG_READ_WRITE          |
                                        PAGE_FLAG_GLOBAL              |
                                        PAGE_FLAG_XD                  |
                                        PAGE_FLAG_PRESENT;
        }
        size += KERNEL_MEM_1GB;
      }
      /* Go to next node */
      kpPhysMemNode = kpPhysMemNode->pNextNode;
    }
  }
}

static void _CreateFramesMetadata(void)
{
  size_t                size;
  uintptr_t             refCountPages;
  uintptr_t             refCountFrames;
  uintptr_t             base;
  uintptr_t             limit;
  uintptr_t             blockSize;
  E_Return              error;
  S_KernelQueueNode*    pNode;
  S_MemoryRange*        pRange;
  S_FrameMetadataTable* pMetaTable;
  S_FrameMetadataTable* pCursor;
  S_FrameMetadataTable* pLastCursor;

  /* Create the frame meta table */
  pNode = sPhysMemList.pQueue->pHead;
  while (pNode != NULL)
  {
    pRange = pNode->pData;

    /* Allocate a new node in the frame meta table */
    pMetaTable = KMalloc(sizeof(S_FrameMetadataTable),
                         ALIGN_ADDRESS,
                         KMALLOC_NO_FREE_POOL);

    /* Allocate the reference count table from this block by iteration */
    base      = pRange->base;
    limit     = pRange->limit;
    blockSize = limit - base;
    while (true)
    {
      /* Get the size in bytes of the reference count table */
      size = (limit - base) / KERNEL_PAGE_SIZE * sizeof(S_FrameMetadata);
      size = ALIGN_UP(size, KERNEL_PAGE_SIZE);

      if (size + (limit - base) <= blockSize || base >= limit)
      {
        break;
      }
      base += KERNEL_PAGE_SIZE;
    }

    MEM_ASSERT(base < limit,
                "Failed to allocate frame meta reference count table, the "
                "block is too small.",
                ERR_NO_MEMORY);

    /* Get the frames */
    refCountFrames = pRange->base;

    /* Update the range */
    pRange->base = base;
    pNode->priority = KERNEL_VIRTUAL_ADDR_MAX - base;

    /* Setup the meta table info */
    pMetaTable->firstFrame = base;
    pMetaTable->lastFrame  = limit;

    size /= KERNEL_PAGE_SIZE;
    refCountPages = _AllocateKernelPages(size);
    MEM_ASSERT(refCountPages != (uintptr_t)NULL,
               "Failed to allocate frame meta reference count table",
               ERR_NO_MEMORY);

    /* Map and initialize the table */
    error = _MemoryMap(refCountPages,
                       refCountFrames,
                       size,
                       MEMMGR_MAP_RW | MEMMGR_MAP_KERNEL,
                       (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
    MEM_ASSERT(error == NO_ERROR,
                "Failed to map frame meta reference count table",
                ERR_NO_MEMORY);

    pMetaTable->pRefCountTable = (S_FrameMetadata*)refCountPages;
    memset(pMetaTable->pRefCountTable, 0, size * KERNEL_PAGE_SIZE);

    /* Link the table */
    pLastCursor = NULL;
    pCursor = spFramesMeta;
    while (pCursor != NULL)
    {
      if (pCursor->firstFrame > pMetaTable->firstFrame)
      {
        break;
      }
      pLastCursor = pCursor;
      pCursor = pCursor->pNext;
    }

    if (pLastCursor == NULL)
    {
      pMetaTable->pNext = spFramesMeta;
      spFramesMeta = pMetaTable;
    }
    else
    {
      pLastCursor->pNext = pMetaTable;
      pMetaTable->pNext = pCursor;
    }
    pNode = pNode->pNext;
  }
}

static void _InitKernelFreePages(void)
{
  uintptr_t kernelVirtEnd;

#ifdef _TESTING_FRAMEWORK_ENABLED
  /* If testing is enabled, the end is after its buffer */
  kernelVirtEnd = (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                  (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE;
#else
  /* Initialize kernel pages */
  kernelVirtEnd  = (uintptr_t)&_KERNEL_MEMORY_END;
#endif

  /* Get actual physical address */
  kernelVirtEnd = ALIGN_UP(kernelVirtEnd, KERNEL_PAGE_SIZE);

  sKernelVirtualMemBounds.base  = kernelVirtEnd;
  sKernelVirtualMemBounds.limit = KERNEL_VIRTUAL_ADDR_MAX;

  /* Add free pages */
  _AddBlock(&sKernelFreePagesList,
            kernelVirtEnd,
            KERNEL_VIRTUAL_ADDR_MAX - kernelVirtEnd + 1);
}

static void _MapKernelRegion(uintptr_t*      pLastSectionStart,
                             uintptr_t*      pLastSectionEnd,
                             const uintptr_t kRegionStartAddr,
                             const uintptr_t kRegionEndAddr,
                             const uint32_t  kFlags)
{
  int8_t     i;
  uintptr_t  tmpPageTablePhysAddr;
  uintptr_t  kernelSectionStart;
  uintptr_t  kernelSectionEnd;
  uint16_t   pmlEntry[4];
  uintptr_t* pPageTable[4];
  uintptr_t  nextPtable;

  /* Align and check */
  kernelSectionStart = ALIGN_DOWN(kRegionStartAddr, KERNEL_PAGE_SIZE);
  kernelSectionEnd   = ALIGN_UP(kRegionEndAddr, KERNEL_PAGE_SIZE);

  MEM_ASSERT(*pLastSectionEnd <= kernelSectionStart,
              "Overlapping kernel memory sections",
              ERR_NO_MEMORY);

  *pLastSectionStart = kernelSectionStart;
  *pLastSectionEnd   = kernelSectionEnd;

  /* Map per 4K pages in the temporary boot entry */
  while (kernelSectionStart < kernelSectionEnd)
  {
    /* Get entry indexes */
    pmlEntry[0] = (kernelSectionStart >> PML1_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[1] = (kernelSectionStart >> PML2_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[2] = (kernelSectionStart >> PML3_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    if (kernelSectionStart < KERNEL_MEM_OFFSET)
    {
        pmlEntry[3] = (kernelSectionStart >> PML4_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
    }
    else
    {
        pmlEntry[3] = KERNEL_PML4_BOOT_TMP_ENTRY;
    }

    /* Setup entry in the four levels is needed  */
    for (i = 3; i >= 0; --i)
    {
      if (i != 3)
      {
        nextPtable = _MakeCanonical(pPageTable[i + 1][pmlEntry[i + 1]] &
                                     ~PAGE_SIZE_MASK,
                                    true);
        pPageTable[i] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
      }
      else
      {
        pPageTable[i] = spKernelPageDir;
      }

      if ((pPageTable[i][pmlEntry[i]] & PAGE_FLAG_PRESENT) == 0)
      {
        /* Allocate a new frame and map to temporary boot entry */
        tmpPageTablePhysAddr = _AllocateFrames(1);
        MEM_ASSERT(tmpPageTablePhysAddr != 0,
                   "Allocated a NULL frame",
                   ERR_INVALID_PARAMETER);

        pPageTable[i][pmlEntry[i]] = tmpPageTablePhysAddr    |
                                     PAGE_FLAG_SUPER_ACCESS  |
                                     PAGE_FLAG_USER_ACCESS   |
                                     PAGE_FLAG_READ_WRITE    |
                                     PAGE_FLAG_CACHE_WB      |
                                     PAGE_FLAG_PRESENT;

        /* Zeroize the table */
        if (i == 0)
        {
          /* Last level, set the entry */
          if (kernelSectionStart >= KERNEL_MEM_OFFSET)
          {
            pPageTable[i][pmlEntry[i]] = (kernelSectionStart -
                                          KERNEL_MEM_OFFSET)       |
                                          PAGE_FLAG_PAGE_SIZE_4KB  |
                                          PAGE_FLAG_SUPER_ACCESS   |
                                          PAGE_FLAG_CACHE_WB       |
                                          PAGE_FLAG_GLOBAL         |
                                          PAGE_FLAG_PRESENT;
          }
          else
          {
            pPageTable[i][pmlEntry[i]] =  kernelSectionStart      |
                                          PAGE_FLAG_PAGE_SIZE_4KB |
                                          PAGE_FLAG_SUPER_ACCESS  |
                                          PAGE_FLAG_CACHE_WB      |
                                          PAGE_FLAG_GLOBAL        |
                                          PAGE_FLAG_PRESENT;
          }

          /* Set the flags */
          if ((kFlags & MEMMGR_MAP_RW) == MEMMGR_MAP_RW)
          {
            pPageTable[i][pmlEntry[i]] |= PAGE_FLAG_READ_WRITE;
          }
          if ((kFlags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
          {
            pPageTable[i][pmlEntry[i]] |= PAGE_FLAG_XD;
          }
        }
        else
        {
          memset((void*)GET_VIRT_MEM_ADDR(tmpPageTablePhysAddr),
                  0,
                  KERNEL_PAGE_SIZE);
        }
      }
    }

    kernelSectionStart += KERNEL_PAGE_SIZE;
  }
}

static void _MapKernel(void)
{
  uintptr_t  kernelSectionStart;
  uintptr_t  kernelSectionEnd;

  kernelSectionStart = 0;
  kernelSectionEnd   = 0;

  /* Map kernel code */
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_START_LOW_AP_STARTUP_ADDR,
                    (uintptr_t)&_END_LOW_AP_STARTUP_ADDR,
                    MEMMGR_MAP_RO | MEMMGR_MAP_EXEC);
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_START_BIOS_CALL_ADDR,
                    (uintptr_t)&_END_BIOS_CALL_ADDR,
                    MEMMGR_MAP_RW | MEMMGR_MAP_EXEC);
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_START_TEXT_ADDR,
                    (uintptr_t)&_END_TEXT_ADDR,
                    MEMMGR_MAP_RO | MEMMGR_MAP_EXEC);

  /* Map kernel RO data */
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_START_RO_DATA_ADDR,
                    (uintptr_t)&_END_RO_DATA_ADDR,
                    MEMMGR_MAP_RO);

  /* Map kernel RW data, stack and heap */
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_START_RW_DATA_ADDR,
                    (uintptr_t)&_END_RW_DATA_ADDR,
                    MEMMGR_MAP_RW);
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_KERNEL_STACKS_BASE,
                    (uintptr_t)&_KERNEL_STACKS_BASE +
                    (uintptr_t)&_KERNEL_STACKS_SIZE,
                    MEMMGR_MAP_RW);
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_KERNEL_NON_FREE_HEAP_BASE,
                    (uintptr_t)&_KERNEL_NON_FREE_HEAP_BASE +
                    (uintptr_t)&_KERNEL_NON_FREE_HEAP_SIZE,
                    MEMMGR_MAP_RW);
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_KERNEL_HEAP_BASE,
                    (uintptr_t)&_KERNEL_HEAP_BASE +
                    (uintptr_t)&_KERNEL_HEAP_SIZE,
                    MEMMGR_MAP_RW);
#ifdef _TESTING_FRAMEWORK_ENABLED
  _MapKernelRegion(&kernelSectionStart,
                    &kernelSectionEnd,
                    (uintptr_t)&_KERNEL_TEST_BUFFER_BASE,
                    (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE,
                    MEMMGR_MAP_RW);
#endif

  /* Copy temporary entry to kernel entry and clear temp  */
  spKernelPageDir[KERNEL_PML4_KERNEL_ENTRY] =
      spKernelPageDir[KERNEL_PML4_BOOT_TMP_ENTRY];
  spKernelPageDir[KERNEL_PML4_BOOT_TMP_ENTRY] = 0;

  /* Update the whole page table */
  CPUSetPageDirectory((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
}

static uintptr_t _AllocateUserPages(const size_t           kPageCount,
                                    const S_KernelProcess* kpProcess,
                                    const bool             kFromTop)
{
    S_ProcessMemoryMetadata* pMemProcInfo;
    uintptr_t                page;

    pMemProcInfo = kpProcess->pMemoryData;

    if (kFromTop == true)
    {
      page =  _GetBlockFromEnd(&pMemProcInfo->freePageTable,
                               kPageCount * KERNEL_PAGE_SIZE);
    }
    else
    {
      page = _GetBlock(&pMemProcInfo->freePageTable,
                       kPageCount * KERNEL_PAGE_SIZE);
    }

    MEM_ASSERT((page & PAGE_SIZE_MASK) == 0,
               "Non aligned page allocated.",
               ERR_UNAUTHORIZED_ACTION);

    return page;
}

static void _ReleaseUserPages(const uintptr_t        kBaseAddress,
                              const size_t           kPageCount,
                              const S_KernelProcess* kpProcess)
{
    S_ProcessMemoryMetadata* pMemProcInfo;

    pMemProcInfo = kpProcess->pMemoryData;

    _AddBlock(&pMemProcInfo->freePageTable,
              kBaseAddress,
              kPageCount * KERNEL_PAGE_SIZE);
}

static E_Return _MemoryMapUser(uintptr_t*     pTableLevel,
                               uintptr_t*     pVirtAddress,
                               uintptr_t*     pPhysicalAddress,
                               size_t*        pPageCount,
                               const uint8_t  kLevel,
                               const uint64_t kPageFlags)
{
  uint32_t  addrEntryIdx;
  uintptr_t nextDirLevelFrame;
  uintptr_t nextDirLevelPage;
  uintptr_t startPageCount;
  size_t    initPageCount;
  uintptr_t initVirtAddr;
  E_Return  error;
  E_Return  internalError;

  if (*pPageCount == 0)
  {
    return NO_ERROR;
  }

  /* Get the entry index */
  switch(kLevel)
  {
    /* PML4 */
    case 4:
      addrEntryIdx = (*pVirtAddress >> PML4_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      initPageCount = *pPageCount;
      initVirtAddr  = *pVirtAddress;
      break;
    /* PML3 */
    case 3:
      addrEntryIdx = (*pVirtAddress >> PML3_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    /* PML2 */
    case 2:
      addrEntryIdx = (*pVirtAddress >> PML2_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    /* PML 1 */
    case 1:
      addrEntryIdx = (*pVirtAddress >> PML1_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    default:
      return ERR_INVALID_PARAMETER;
  }

  error = NO_ERROR;
  /* Check all entries of the current table */
  while (*pPageCount > 0 && addrEntryIdx < KERNEL_PGDIR_ENTRY_COUNT)
  {
    startPageCount = *pPageCount;

    /* If not already mapped, create a new table and init */
    if ((pTableLevel[addrEntryIdx] & PAGE_FLAG_PRESENT) == 0)
    {
      /* If not last level, we are mapping a physical frame that is part
       * of the page directory.
       */
      if (kLevel != 1)
      {
        /* Allocate the new entry for the table */
        nextDirLevelFrame = _AllocateFrames(1);
        if (nextDirLevelFrame == (uintptr_t)NULL)
        {
          error = ERR_NO_MEMORY;
          break;
        }
        nextDirLevelPage = GET_VIRT_MEM_ADDR(nextDirLevelFrame);

        /* Clear the new page table */
        memset((uintptr_t*)nextDirLevelPage,
               0,
               KERNEL_PGDIR_ENTRY_COUNT * sizeof(uintptr_t));

        /* Set the mapping flags */
        pTableLevel[addrEntryIdx] = nextDirLevelFrame       |
                                    PAGE_FLAG_SUPER_ACCESS  |
                                    PAGE_FLAG_USER_ACCESS   |
                                    PAGE_FLAG_READ_WRITE    |
                                    PAGE_FLAG_CACHE_WB      |
                                    PAGE_FLAG_XD            |
                                    PAGE_FLAG_PRESENT;


        /* Map next level, pVirtAddress will be updated there */
        error = _MemoryMapUser((uintptr_t*)nextDirLevelPage,
                                pVirtAddress,
                                pPhysicalAddress,
                                pPageCount,
                                kLevel - 1,
                                kPageFlags);

        /* Stop on error */
        if (error != NO_ERROR)
        {
          /* The recursive partially errored mapping was released*/
          pTableLevel[addrEntryIdx] = 0;
          _ReleaseFrames(nextDirLevelFrame, 1);
          *pPageCount = startPageCount;
          break;
        }
      }
      else
      {
        /* Set the mapping flags */
        pTableLevel[addrEntryIdx] = *pPhysicalAddress | kPageFlags;

        /* Update position */
        *pVirtAddress += KERNEL_PAGE_SIZE;
        *pPhysicalAddress += KERNEL_PAGE_SIZE;
        *pPageCount -= 1;
      }
    }
    else
    {
      /* If not in the last level, just get the mapping and pursue */
      if (kLevel != 1)
      {
        /* Get the entry and map it */
        nextDirLevelFrame = _MakeCanonical(pTableLevel[addrEntryIdx] &
                                            ~PAGE_SIZE_MASK,
                                            true);
        nextDirLevelPage = GET_VIRT_MEM_ADDR(nextDirLevelFrame);

        /* Pursue */
        error = _MemoryMapUser((uintptr_t*)nextDirLevelPage,
                                  pVirtAddress,
                                  pPhysicalAddress,
                                  pPageCount,
                                  kLevel - 1,
                                  kPageFlags);

        /* Stop on error */
        if (error != NO_ERROR)
        {
          /* The recursive partially errored mapping was released*/
          *pPageCount = startPageCount;
          break;
        }
      }
      else
      {
        /* This page is already mapped, error */
        error = ERR_UNAUTHORIZED_ACTION;
        break;
      }
    }

    /* Go to next entry */
    ++addrEntryIdx;
  }

  /* On error, release the mapped memory in the last level */
  if (kLevel == 4 && error != NO_ERROR && initPageCount - *pPageCount != 0)
  {
    initPageCount = initPageCount - *pPageCount;
    internalError = _MemoryUnmapUser(pTableLevel,
                                     &initVirtAddr,
                                     &initPageCount,
                                     4);
    MEM_ASSERT(internalError == NO_ERROR,
               "Failed to unmap already mapped memory",
               internalError);
  }

  return error;
}

static E_Return _MemoryUnmapUser(uintptr_t*    pTableLevel,
                                 uintptr_t*    pVirtAddress,
                                 size_t*       pPageCount,
                                 const uint8_t kLevel)
{
  uint32_t    i;
  uint32_t    addrEntryIdx;
  uintptr_t   nextDirLevelFrame;
  uintptr_t*  nextDirLevelPage;
  E_Return error;

  if (*pPageCount == 0)
  {
    return ERR_INVALID_PARAMETER;
  }

  /* Get the entry index */
  switch(kLevel)
  {
    /* PML4 */
    case 4:
      addrEntryIdx = (*pVirtAddress >> PML4_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    /* PML3 */
    case 3:
      addrEntryIdx = (*pVirtAddress >> PML3_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    /* PML2 */
    case 2:
      addrEntryIdx = (*pVirtAddress >> PML2_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    /* PML 1 */
    case 1:
      addrEntryIdx = (*pVirtAddress >> PML1_ENTRY_OFFSET) &
                      PG_ENTRY_OFFSET_MASK;
      break;
    default:
      return ERR_INVALID_PARAMETER;
  }

  error = NO_ERROR;
  /* Check all entries of the current table */
  while (*pPageCount > 0 && addrEntryIdx < KERNEL_PGDIR_ENTRY_COUNT)
  {
    /* If mapped unmap what needs to be mapped */
    if ((pTableLevel[addrEntryIdx] & PAGE_FLAG_PRESENT) != 0)
    {
      /* If not last level, we are mapping a physical frame that is part
       * of the page directory.
       */
      if (kLevel != 1)
      {
        /* Get the entry and map it */
        nextDirLevelFrame = _MakeCanonical(pTableLevel[addrEntryIdx] &
                                             ~PAGE_SIZE_MASK,
                                           true);
        nextDirLevelPage = (uintptr_t*)GET_VIRT_MEM_ADDR(nextDirLevelFrame);


        /* Unmap next level, pVirtAddress will be updated there */
        error = _MemoryUnmapUser(nextDirLevelPage,
                                 pVirtAddress,
                                 pPageCount,
                                 kLevel - 1);

        if (error == NO_ERROR)
        {
          /* Check if we can release the frame */
          for (i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
          {
            if ((nextDirLevelPage[i] & PAGE_FLAG_PRESENT) != 0)
            {
              break;
            }
            if (i == KERNEL_PGDIR_ENTRY_COUNT)
            {
              _ReleaseFrames(nextDirLevelFrame, 1);
              pTableLevel[addrEntryIdx] = 0;
            }
          }
        }
        else
        {
          /* Stop on error */
          break;
        }
      }
      else
      {
        /* Unset the mapping */
        pTableLevel[addrEntryIdx] = 0;

        /* Update position */
        *pVirtAddress += KERNEL_PAGE_SIZE;
        *pPageCount -= 1;
      }
    }
    else
    {
      return ERR_UNAUTHORIZED_ACTION;
    }

    /* Go to next entry */
    ++addrEntryIdx;
  }

  return error;
}

static inline void _GetReferenceIndexTable(const uintptr_t        kPhysAddr,
                                           S_FrameMetadataTable** ppTable,
                                           size_t*                pEntryIdx)
{
  /* Search for the entry */
  *ppTable = spFramesMeta;
  while (*ppTable != NULL)
  {
    if (kPhysAddr >= (*ppTable)->firstFrame &&
        kPhysAddr <= (*ppTable)->lastFrame)
    {
      break;
    }
    *ppTable = (*ppTable)->pNext;
  }

  MEM_ASSERT(*ppTable != NULL,
              "Failed to find physical address in frames meta table",
              ERR_NO_MEMORY);

  /* Calculate the id in the index */
  *pEntryIdx = (kPhysAddr - (*ppTable)->firstFrame) >> PML1_ENTRY_OFFSET;
}

static uint16_t* _GetAndLockReferenceCount(const uintptr_t kPhysAddr)
{
  size_t                entryIdx;
  S_FrameMetadataTable* pTable;

  if (spFramesMeta == NULL)
  {
    return 0;
  }

  _GetReferenceIndexTable(kPhysAddr, &pTable, &entryIdx);
  KERNEL_LOCK(pTable->pRefCountTable[entryIdx].lock);

  return &pTable->pRefCountTable[entryIdx].refCount;
}

static void _UnlockReferenceCount(const uintptr_t kPhysAddr)
{
  size_t                entryIdx;
  S_FrameMetadataTable* pTable;

  if (spFramesMeta == NULL)
  {
    return;
  }

  _GetReferenceIndexTable(kPhysAddr, &pTable, &entryIdx);
  KERNEL_UNLOCK(pTable->pRefCountTable[entryIdx].lock);
}

static uint64_t _TranslateFlags(const uint32_t kFlags)
{
  uint64_t mapFlags;

  mapFlags = 0;

  if ((kFlags & MEMMGR_MAP_KERNEL) == MEMMGR_MAP_KERNEL)
  {
    mapFlags |= PAGE_FLAG_SUPER_ACCESS | PAGE_FLAG_GLOBAL;
  }
  if ((kFlags & MEMMGR_MAP_USER) == MEMMGR_MAP_USER)
  {
    mapFlags |= PAGE_FLAG_USER_ACCESS;
  }
  if ((kFlags & MEMMGR_MAP_RW) == MEMMGR_MAP_RW)
  {
    mapFlags |= PAGE_FLAG_READ_WRITE;
  }
  else
  {
    mapFlags |= PAGE_FLAG_READ_ONLY;
  }
  if ((kFlags & MEMMGR_MAP_CACHE_DISABLED) == MEMMGR_MAP_CACHE_DISABLED)
  {
    mapFlags |= PAGE_FLAG_CACHE_DISABLED;
  }
  else
  {
    mapFlags |= PAGE_FLAG_CACHE_WB;
  }
  if ((kFlags & MEMMGR_MAP_WRITE_COMBINING) == MEMMGR_MAP_WRITE_COMBINING)
  {
    mapFlags |= PAGE_FLAG_CACHE_WC;
  }
  if ((kFlags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
  {
    mapFlags |= PAGE_FLAG_XD;
  }
  if ((kFlags & MEMMGR_MAP_HARDWARE) == MEMMGR_MAP_HARDWARE)
  {
    mapFlags |= PAGE_FLAG_CACHE_DISABLED | PAGE_FLAG_IS_HW;
  }
  if ((kFlags & MEMMGR_MAP_COW) == MEMMGR_MAP_COW)
  {
    mapFlags |= PAGE_FLAG_COW;
  }

  return mapFlags;
}

void MemoryInit(void)
{
  E_Return error;

  /* Get the CPU capabilities */
  sPhysAddressWidth  = CPUGetPhysicalAddressWidth();
  sVirtAddressWidth  = CPUGetVirtualAddressWidth();
  sCpu1GBPageSupport = CPUGet1GBPageSupport();

  /* Initialize structures */
  sPhysMemList.pQueue = KQueueCreate();
  KERNEL_SPINLOCK_INIT(sPhysMemList.lock);

  sKernelFreePagesList.pQueue = KQueueCreate();
  KERNEL_SPINLOCK_INIT(sKernelFreePagesList.lock);

  sPhysAddressWidthMask = ((1ULL << sPhysAddressWidth) - 1);
  sCanonicalBound       = ((1ULL << (sVirtAddressWidth - 1)) - 1);

  /* Setup the PAT as follows:
   * WC UC- WT WB UC UC- WT WB
   */
  __asm__ __volatile__ ("mov $0x277, %%rcx\n\t"
                        "rdmsr\n\t"
                        "and $0xFFFFFFFFF8FFFFFF, %%rdx\n\t"
                        "or  $0x01000000, %%rdx\n\t"
                        "wrmsr\n\t"
                        :
                        :
                        : "rax", "rcx", "rdx");

  /* Setup the memory frames mapping */
  spKernelPageDir[KERNEL_MEM_PML4_ENTRY] =
    ((uintptr_t)_physicalMapDir - KERNEL_MEM_OFFSET) |
    PAGE_FLAG_SUPER_ACCESS       |
    PAGE_FLAG_CACHE_WB           |
    PAGE_FLAG_READ_WRITE         |
    PAGE_FLAG_PRESENT;

  /* Setup the kernel free pages */
  _InitKernelFreePages();

  /* Detect the memory */
  _DetectMemory();

  /* Create the flat physical memory translation */
  _CreateFlatMap();

  /* Update the whole page table */
  CPUSetPageDirectory((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);

  /* Map the kernel */
  _MapKernel();

  /* Creates the frames metadata */
  _CreateFramesMetadata();

  /* Registers the page fault handler */
  error = InterruptRegister(PAGE_FAULT_EXC_LINE, _PageFaultHandler, false);
  MEM_ASSERT(error == NO_ERROR,
             "Failed to register the page fault handler",
             error);
}

void* MemoryKernelMap(const void*    kPhysicalAddress,
                      const size_t   kSize,
                      const uint32_t kFlags,
                      E_Return*      pError)
{
  uintptr_t kernelPages;
  size_t    pageCount;

  /* Check size */
  if ((kSize & PAGE_SIZE_MASK) == 0 && kSize >= KERNEL_PAGE_SIZE)
  {
    pageCount = kSize / KERNEL_PAGE_SIZE;

    KERNEL_LOCK(sLock);

    /* Allocate pages */
    kernelPages = _AllocateKernelPages(pageCount);
    if (kernelPages != (uintptr_t)NULL)
    {
      /* Apply mapping */
      *pError = _MemoryMap(kernelPages,
                           (uintptr_t)kPhysicalAddress,
                           pageCount,
                           kFlags | MEMMGR_MAP_KERNEL,
                           (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
      if (*pError != NO_ERROR)
      {
        _ReleaseKernelPages(kernelPages, pageCount);
        kernelPages = (uintptr_t)NULL;
      }

      KERNEL_UNLOCK(sLock);
    }
    else
    {
      KERNEL_UNLOCK(sLock);

      *pError = ERR_NO_MEMORY;
      kernelPages = (uintptr_t)NULL;
    }
  }
  else
  {
    *pError = ERR_INVALID_PARAMETER;
    kernelPages = (uintptr_t)NULL;
  }

  return (void*)kernelPages;
}

E_Return MemoryKernelUnmap(const void* kVirtualAddress, const size_t kSize)
{
  size_t   pageCount;
  E_Return error;

  /* Check size */
  if ((kSize & PAGE_SIZE_MASK) == 0 && kSize >= KERNEL_PAGE_SIZE)
  {
    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Check if actually kernel addresses */
    if ((uintptr_t)kVirtualAddress >= sKernelVirtualMemBounds.base         &&
        (uintptr_t)kVirtualAddress + kSize < sKernelVirtualMemBounds.limit &&
        (uintptr_t)kVirtualAddress + kSize >= (uintptr_t)kVirtualAddress)
    {
      KERNEL_LOCK(sLock);

      /* Unmap */
      error = _MemoryUnmap((uintptr_t)kVirtualAddress,
                           pageCount,
                           (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);

      /* Release the kernel page if correctly unmaped */
      if (error == NO_ERROR)
      {
        _ReleaseKernelPages((uintptr_t)kVirtualAddress, pageCount);
      }

      KERNEL_UNLOCK(sLock);
    }
    else
    {
      error = ERR_INVALID_PARAMETER;
    }
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

uintptr_t MemoryMapStack(const size_t     kSize,
                         const bool       kIsKernel,
                         S_KernelProcess* pProcess)
{
  size_t                   pageCount;
  size_t                   mappedCount;
  size_t                   i;
  E_Return                 error;
  uintptr_t                pageBaseAddress;
  uintptr_t                newFrame;
  uintptr_t                mapFlags;
  S_ProcessMemoryMetadata* pProcMem;
  uintptr_t                pgDir;
  S_KernelSpinlock*        pLock;

  /* Get the page count */
  pageCount = ALIGN_UP(kSize, KERNEL_PAGE_SIZE) / KERNEL_PAGE_SIZE;

  /* Get the correct page directory */
  if (kIsKernel == true)
  {
    pLock = &sLock;
    pgDir = (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET;
  }
  else
  {
    pProcMem = pProcess->pMemoryData;
    pLock    = &pProcMem->lock;
    pgDir    = pProcMem->PDPhysAddress;
  }

  /* Request the pages + 1 to catch overflow (not mapping the last page)*/
  if (kIsKernel == true)
  {
    mapFlags = MEMMGR_MAP_RW | MEMMGR_MAP_KERNEL;
    pageBaseAddress = _AllocateKernelPages(pageCount + 1);

  }
  else
  {
    mapFlags = MEMMGR_MAP_RW | MEMMGR_MAP_USER;
    pageBaseAddress = _AllocateUserPages(pageCount + 1, pProcess, true);
  }

  if (pageBaseAddress != 0)
  {
    KERNEL_LOCK(*pLock);
    /* Now map, we do not need contiguous frames */
    for (i = 0; i < pageCount; ++i)
    {
      newFrame = _AllocateFrames(1);
      if (newFrame == 0)
      {
        break;
      }

      error = _MemoryMap(pageBaseAddress + (i + 1) * KERNEL_PAGE_SIZE,
                         newFrame,
                         1,
                         mapFlags,
                         pgDir);
      if (error != NO_ERROR)
      {
        /* On error, release the frame */
        _ReleaseFrames(newFrame, 1);
        break;
      }
    }

    /* Check if everything is mapped, if not unmap and return */
    if (i < pageCount)
    {
      if (i != 0)
      {

        mappedCount = i;
        /* Release frames */
        for (i = 0; i < mappedCount; ++i)
        {
          newFrame = _GetPhysAddr(pageBaseAddress + KERNEL_PAGE_SIZE * (i + 1),
                                  pgDir,
                                  NULL);
          MEM_ASSERT(newFrame != MEMMGR_PHYS_ADDR_ERROR,
                     "Invalid physical frame",
                     ERR_UNAUTHORIZED_ACTION);
          _ReleaseFrames(newFrame, 1);
        }

        _MemoryUnmap(pageBaseAddress + KERNEL_PAGE_SIZE, mappedCount, pgDir);

      }
      if (kIsKernel == true)
      {
        _ReleaseKernelPages(pageBaseAddress, pageCount + 1);
      }
      else
      {
        _ReleaseUserPages(pageBaseAddress, pageCount + 1, pProcess);
      }

      pageBaseAddress = (uintptr_t)NULL;
    }
    KERNEL_UNLOCK(*pLock);

    if (pageBaseAddress != (uintptr_t)NULL)
    {
      pageBaseAddress += ((pageCount + 1) * KERNEL_PAGE_SIZE);
    }
  }

  return pageBaseAddress;
}

void MemoryUnmapStack(const uintptr_t  kEndAddress,
                      const size_t     kSize,
                      const bool       kIsKernel,
                      S_KernelProcess* pProcess)
{
  size_t                   pageCount;
  size_t                   i;
  uintptr_t                frameAddr;
  uintptr_t                baseAddress;
  S_ProcessMemoryMetadata* pProcMem;
  uintptr_t                pgDir;
  S_KernelSpinlock*        pLock;

  MEM_ASSERT((kEndAddress & PAGE_SIZE_MASK) == 0 &&
              (kSize & PAGE_SIZE_MASK) == 0 &&
              kSize != 0,
              "Unmaped kernel stack with invalid parameters",
              ERR_INVALID_PARAMETER);

  /* Get the page count */
  pageCount   = kSize / KERNEL_PAGE_SIZE;
  baseAddress = kEndAddress - kSize;

  if (kIsKernel == true)
  {
    /* Check if actually kernel addresses */
    MEM_ASSERT((uintptr_t)kEndAddress <= sKernelVirtualMemBounds.limit &&
                (uintptr_t)kEndAddress - kSize >=
               sKernelVirtualMemBounds.base,
               "Trying to release kernel stack our of kernel space",
               ERR_INVALID_PARAMETER);
    pLock = &sLock;
    pgDir = (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET;
  }
  else
  {
    /* Check if actually user addresses */
    MEM_ASSERT((uintptr_t)kEndAddress <= USER_MEMORY_END &&
                (uintptr_t)kEndAddress - kSize >= USER_MEMORY_START,
               "Trying to release kernel stack our of kernel space",
               ERR_INVALID_PARAMETER);
    pProcMem = pProcess->pMemoryData;
    pLock = &pProcMem->lock;
    pgDir = pProcMem->PDPhysAddress;
  }

  KERNEL_LOCK(*pLock);
  /* Free the frames and memory */
  for (i = 0; i < pageCount; ++i)
  {
    frameAddr = _GetPhysAddr(baseAddress + KERNEL_PAGE_SIZE * i, pgDir, NULL);
    MEM_ASSERT(frameAddr != MEMMGR_PHYS_ADDR_ERROR,
               "Invalid physical frame",
               ERR_UNAUTHORIZED_ACTION);
    _ReleaseFrames(frameAddr, 1);
  }

  /* Unmap the memory */
  _MemoryUnmap(baseAddress, pageCount, pgDir);


  if (kIsKernel == true)
  {
    _ReleaseKernelPages(baseAddress - KERNEL_PAGE_SIZE, pageCount + 1);
  }
  else
  {
    _ReleaseUserPages(baseAddress - KERNEL_PAGE_SIZE, pageCount + 1, pProcess);
  }

  KERNEL_UNLOCK(*pLock);
}

uintptr_t MemoryMgrGetPhysAddr(const uintptr_t        kVirtualAddress,
                               const S_KernelProcess* kpProcess,
                               uint32_t*              pFlags)
{
  uintptr_t                retPhysAddr;
  S_ProcessMemoryMetadata* pMemInfo;

  pMemInfo = kpProcess->pMemoryData;

  KERNEL_LOCK(sLock);
  KERNEL_LOCK(pMemInfo->lock);

  retPhysAddr = _GetPhysAddr(kVirtualAddress, pMemInfo->PDPhysAddress, pFlags);

  KERNEL_UNLOCK(pMemInfo->lock);
  KERNEL_UNLOCK(sLock);

  return retPhysAddr;
}

void* MemoryKernelAllocate(const size_t   kSize,
                           const uint32_t kFlags,
                           E_Return*      pError)
{
  size_t    pageCount;
  size_t    mappedCount;
  size_t    i;
  E_Return  error;
  E_Return  internalError;
  uintptr_t pageBaseAddress;
  uintptr_t newFrame;

  /* Check size */
  if ((kSize & PAGE_SIZE_MASK) == 0 && kSize >= KERNEL_PAGE_SIZE &&
      (kFlags & MEMMGR_MAP_HARDWARE) != MEMMGR_MAP_HARDWARE)
  {
    /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Request the pages */
    pageBaseAddress = _AllocateKernelPages(pageCount);
    if (pageBaseAddress != 0)
    {
      KERNEL_LOCK(sLock);
      /* Now map, we do not need contiguous frames */
      for (i = 0; i < pageCount; ++i)
      {
        newFrame = _AllocateFrames(1);
        if (newFrame == 0)
        {
          break;
        }

        error = _MemoryMap(pageBaseAddress + i * KERNEL_PAGE_SIZE,
                            newFrame,
                            1,
                            kFlags,
                            (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
        if (error != NO_ERROR)
        {
          /* On error, release the frame */
          _ReleaseFrames(newFrame, 1);
          break;
        }
      }

      /* Check if everything is mapped, if not unmap and return */
      if (i < pageCount)
      {
        if (i != 0)
        {
          mappedCount = i;
          /* Release frames */
          for (i = 0; i < mappedCount; ++i)
          {
            newFrame = _GetPhysAddr(pageBaseAddress +
                                             KERNEL_PAGE_SIZE * i,
                                             (uintptr_t)spKernelPageDir -
                                             KERNEL_MEM_OFFSET,
                                             NULL);
            MEM_ASSERT(newFrame != MEMMGR_PHYS_ADDR_ERROR,
                       "Invalid physical frame",
                       ERR_UNAUTHORIZED_ACTION);
            _ReleaseFrames(newFrame, 1);
          }

          internalError = _MemoryUnmap(pageBaseAddress,
                                       mappedCount,
                                       (uintptr_t)spKernelPageDir -
                                       KERNEL_MEM_OFFSET);
          MEM_ASSERT(internalError == NO_ERROR,
                     "Failed to unmapp mapped memory",
                     internalError);
        }
        _ReleaseKernelPages(pageBaseAddress, pageCount);
        pageBaseAddress = (uintptr_t)NULL;
      }

      KERNEL_UNLOCK(sLock);

      *pError = error;
    }
    else
    {
      *pError = ERR_NO_MEMORY;
      pageBaseAddress = (uintptr_t)NULL;
    }
  }
  else
  {
    *pError = ERR_INVALID_PARAMETER;
    pageBaseAddress = (uintptr_t)NULL;
  }

  return (void*)pageBaseAddress;
}

E_Return MemoryKernelFree(const void* kVirtualAddress, const size_t kSize)
{
  size_t    pageCount;
  size_t    i;
  uintptr_t frameAddr;
  E_Return  error;

  if (((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) == 0 &&
      (kSize & PAGE_SIZE_MASK) == 0 &&
      kSize >= KERNEL_PAGE_SIZE &&
      (uintptr_t)kVirtualAddress >= sKernelVirtualMemBounds.base &&
      (uintptr_t)kVirtualAddress + kSize <= sKernelVirtualMemBounds.limit &&
      (uintptr_t)kVirtualAddress + kSize >= (uintptr_t)kVirtualAddress)
  {
     /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;

    KERNEL_LOCK(sLock);

    /* Free the frames and memory */
    for (i = 0; i < pageCount; ++i)
    {
      frameAddr = _GetPhysAddr((uintptr_t)kVirtualAddress +
                               KERNEL_PAGE_SIZE * i,
                               (uintptr_t)spKernelPageDir -
                               KERNEL_MEM_OFFSET,
                               NULL);
      MEM_ASSERT(frameAddr != MEMMGR_PHYS_ADDR_ERROR,
                  "Invalid physical frame",
                  ERR_UNAUTHORIZED_ACTION);
      _ReleaseFrames(frameAddr, 1);
    }

    /* Unmap the memory */
    error = _MemoryUnmap((uintptr_t)kVirtualAddress,
                         pageCount,
                         (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
    MEM_ASSERT(error == NO_ERROR,
               "Invalid unmapping frame",
               ERR_UNAUTHORIZED_ACTION);

    KERNEL_UNLOCK(sLock);

    /* Release pages */
    _ReleaseKernelPages((uintptr_t)kVirtualAddress, pageCount);
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

void* MemoryCreateProcessData(void)
{
  S_ProcessMemoryMetadata* pMemProcInfo;

  /* Create the memory structure */
  pMemProcInfo = KMalloc(sizeof(S_ProcessMemoryMetadata),
                         ALIGN_ADDRESS,
                         KMALLOC_FREE_POOL);

  /* Create the page directory */
  if (SchedulerIsInitialized() == true)
  {
    /* Allocate a frame for the page directory */
    pMemProcInfo->PDPhysAddress = (uintptr_t)NULL;

    /* Create the free page table */
    pMemProcInfo->freePageTable.pQueue = KQueueCreate();
    KERNEL_SPINLOCK_INIT(pMemProcInfo->freePageTable.lock);

    /* Add free pages */
    _AddBlock(&pMemProcInfo->freePageTable,
              USER_MEMORY_START,
              USER_MEMORY_END - USER_MEMORY_START);

    KERNEL_SPINLOCK_INIT(pMemProcInfo->lock);
  }
  else
  {
    /* When the scheduler is not initialized, use the kernel page dir */
    pMemProcInfo->PDPhysAddress = (uintptr_t)spKernelPageDir -
                                  KERNEL_MEM_OFFSET;
  }

  return pMemProcInfo;
}

void MemoryDestroyProcessData(void* pMemoryData)
{
  S_ProcessMemoryMetadata* pMemProcInfo;

  pMemProcInfo = pMemoryData;

  MEM_ASSERT(pMemProcInfo->PDPhysAddress !=
             (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET,
             "Tried to release kernel page directory",
             ERR_UNAUTHORIZED_ACTION);

  KERNEL_LOCK(pMemProcInfo->lock);

  /* Destroy the page directory */
  _ReleasePageDir(pMemProcInfo->PDPhysAddress, 0, 4);

  /* Destroy the free page table */
  KQueueClean(pMemProcInfo->freePageTable.pQueue);
  KQueueDestroy(&pMemProcInfo->freePageTable.pQueue);

  KERNEL_UNLOCK(pMemProcInfo->lock);

  /* Release the memory structure */
  KFree(pMemProcInfo);
}

uintptr_t MemoryGetUserStartAddr(void)
{
  return (uintptr_t)USER_MEMORY_START;
}

uintptr_t MemoryGetUserEndAddr(void)
{
  return (uintptr_t)USER_MEMORY_END;
}

uintptr_t MemoryAllocFrames(const size_t kFrameCount)
{
  return _AllocateFrames(kFrameCount);
}

void MemoryReleaseFrame(const uintptr_t kBaseAddress,
                        const size_t    kFrameCount)
{
  _ReleaseFrames(kBaseAddress, kFrameCount);
}

E_Return MemoryUserMapDirect(const void*      kPhysicalAddress,
                             const void*      kVirtualAddress,
                             const size_t     kSize,
                             const uint32_t   kFlags,
                             const bool       kRemoveFromPagePool,
                             S_KernelProcess* pProcess)
{
  S_ProcessMemoryMetadata* pMemProcInfo;
  uint64_t                 flags;
  size_t                   pageCount;
  uintptr_t                startVirt;
  uintptr_t                startPhys;
  uintptr_t*               pPageDir;
  E_Return                 error;
  bool                     isMapped;

  pMemProcInfo = pProcess->pMemoryData;

  if (((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) == 0 &&
      ((uintptr_t)kPhysicalAddress & PAGE_SIZE_MASK) == 0 &&
      (kSize & PAGE_SIZE_MASK) == 0 &&
      kSize >= KERNEL_PAGE_SIZE &&
      (uintptr_t)kVirtualAddress >= USER_MEMORY_START &&
      (uintptr_t)kVirtualAddress + kSize <= USER_MEMORY_END &&
      (uintptr_t)kVirtualAddress + kSize >= (uintptr_t)kVirtualAddress)
  {
    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Get flags */
    flags = PAGE_FLAG_PRESENT | _TranslateFlags(kFlags);

    KERNEL_LOCK(pMemProcInfo->lock);

    /* Check if the mapping already exists */
    isMapped = _IsMapped((uintptr_t)kVirtualAddress,
                         pageCount,
                         pMemProcInfo->PDPhysAddress,
                         false);
    if (isMapped == false)
    {
      /* Temporary map the process page directory */
      pPageDir = (uintptr_t*)GET_VIRT_MEM_ADDR(pMemProcInfo->PDPhysAddress);

      /* Map the data */
      startVirt = (uintptr_t)kVirtualAddress;
      startPhys = (uintptr_t)kPhysicalAddress;
      error = _MemoryMapUser(pPageDir,
                                &startVirt,
                                &startPhys,
                                &pageCount,
                                4,
                                flags);

      if (error == NO_ERROR && kRemoveFromPagePool == true)
      {
        /* Remove from user free pages */
        _RemoveBlock(&pMemProcInfo->freePageTable,
                      (uintptr_t)kVirtualAddress,
                      kSize);
      }
    }
    else
    {
      error = ERR_UNAUTHORIZED_ACTION;
    }

    KERNEL_UNLOCK(pMemProcInfo->lock);
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

E_Return MemoryUserUnmap(const void*       kVirtualAddress,
                         const size_t      kSize,
                         const bool        kAddToPagePool,
                         S_KernelProcess* pProcess)
{
  S_ProcessMemoryMetadata* pMemProcInfo;
  size_t                   pageCount;
  uintptr_t                startVirt;
  uintptr_t*               pPageDir;
  E_Return                 error;
  bool                     isMapped;

  /* Aligne memory */
  if (((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) == 0 &&
    (kSize & PAGE_SIZE_MASK) == 0 &&
    kSize >= KERNEL_PAGE_SIZE &&
    (uintptr_t)kVirtualAddress >= USER_MEMORY_START &&
    (uintptr_t)kVirtualAddress + kSize <= USER_MEMORY_END &&
    (uintptr_t)kVirtualAddress + kSize >= (uintptr_t)kVirtualAddress)
  {
    pageCount = kSize / KERNEL_PAGE_SIZE;
    pMemProcInfo = pProcess->pMemoryData;

    KERNEL_LOCK(pMemProcInfo->lock);

    /* Check if the mapping already exists */
    isMapped = _IsMapped((uintptr_t)kVirtualAddress,
                         pageCount,
                         pMemProcInfo->PDPhysAddress,
                         true);
    if (isMapped == true)
    {
      /* Temporary map the process page directory */
      pPageDir = (uintptr_t*)GET_VIRT_MEM_ADDR(pMemProcInfo->PDPhysAddress);

      /* Unmap the data */
      startVirt = (uintptr_t)kVirtualAddress;
      error = _MemoryUnmapUser(pPageDir, &startVirt, &pageCount, 4);
      MEM_ASSERT(error == NO_ERROR, "Failed to unmap mapped memory", error);

      KERNEL_UNLOCK(pMemProcInfo->lock);

      if (error == NO_ERROR && kAddToPagePool == true)
      {
        pageCount = kSize / KERNEL_PAGE_SIZE;
        _ReleaseUserPages((uintptr_t)kVirtualAddress, pageCount, pProcess);
      }
    }
    else
    {
      KERNEL_UNLOCK(pMemProcInfo->lock);
      error = ERR_UNAUTHORIZED_ACTION;
    }
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

void* MemoryUserAllocate(const size_t     kSize,
                         const uint32_t   kFlags,
                         S_KernelProcess* pProcess,
                         E_Return*        pError)
{
  size_t                   pageCount;
  size_t                   mappedCount;
  size_t                   i;
  E_Return                 internalError;
  uintptr_t                pageBaseAddress;
  uintptr_t                newFrame;
  S_ProcessMemoryMetadata* pMemInfo;

  /* Check size and flags */
  if ((kSize & PAGE_SIZE_MASK) == 0 &&
      kSize >= KERNEL_PAGE_SIZE &&
      (kFlags & MEMMGR_MAP_HARDWARE) != MEMMGR_MAP_HARDWARE)
  {
    /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Request the pages */
    pageBaseAddress = _AllocateUserPages(pageCount, pProcess, false);
    if (pageBaseAddress != 0)
    {
      pMemInfo = pProcess->pMemoryData;

      KERNEL_LOCK(pMemInfo->lock);

      /* Now map, we do not need contiguous frames */
      for (i = 0; i < pageCount; ++i)
      {
        newFrame = _AllocateFrames(1);
        if (newFrame == 0)
        {
            break;
        }

        *pError = _MemoryMap(pageBaseAddress + i * KERNEL_PAGE_SIZE,
                             newFrame,
                             1,
                             kFlags,
                             pMemInfo->PDPhysAddress);
        if (*pError != NO_ERROR)
        {
          /* On error, release the frame */
          _ReleaseFrames(newFrame, 1);
          break;
        }
      }

      /* Check if everything is mapped, if not unmap and return */
      if (i < pageCount)
      {
        if (i != 0)
        {
          mappedCount = i;
          /* Release frames */
          for (i = 0; i < mappedCount; ++i)
          {
            newFrame = _GetPhysAddr(pageBaseAddress + KERNEL_PAGE_SIZE * i,
                                    pMemInfo->PDPhysAddress,
                                    NULL);
            MEM_ASSERT(newFrame != MEMMGR_PHYS_ADDR_ERROR,
                       "Invalid physical frame",
                       ERR_UNAUTHORIZED_ACTION);
            _ReleaseFrames(newFrame, 1);
          }

          internalError = _MemoryUnmap(pageBaseAddress,
                                       mappedCount,
                                       pMemInfo->PDPhysAddress);
          MEM_ASSERT(internalError == NO_ERROR,
                     "Failed to unmapp mapped memory",
                     internalError);
        }
        _ReleaseUserPages(pageBaseAddress, pageCount, pProcess);
        pageBaseAddress = (uintptr_t)NULL;
      }

      KERNEL_UNLOCK(pMemInfo->lock);
    }
    else
    {
      *pError = ERR_NO_MEMORY;
    }
  }
  else
  {
    pageBaseAddress = (uintptr_t)NULL;
    *pError         = ERR_INVALID_PARAMETER;
  }

  return (void*)pageBaseAddress;
}

E_Return MemoryUserFree(const void*      kVirtualAddress,
                        const size_t     kSize,
                        S_KernelProcess* pProcess)
{
    size_t                   pageCount;
    size_t                   i;
    uintptr_t                frameAddr;
    E_Return                 error;
    S_ProcessMemoryMetadata* pMemInfo;

    if (((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) == 0 &&
        (kSize & PAGE_SIZE_MASK) == 0 &&
        kSize >= KERNEL_PAGE_SIZE &&
        (uintptr_t)kVirtualAddress >= USER_MEMORY_START &&
        (uintptr_t)kVirtualAddress + kSize <= USER_MEMORY_END &&
        (uintptr_t)kVirtualAddress + kSize >= (uintptr_t)kVirtualAddress)
    {
      /* Get the page count */
      pageCount = kSize / KERNEL_PAGE_SIZE;

      pMemInfo = pProcess->pMemoryData;

      KERNEL_LOCK(pMemInfo->lock);

      /* Free the frames and memory */
      for (i = 0; i < pageCount; ++i)
      {
        frameAddr = _GetPhysAddr((uintptr_t)kVirtualAddress +
                                  KERNEL_PAGE_SIZE * i,
                                 pMemInfo->PDPhysAddress,
                                 NULL);
        MEM_ASSERT(frameAddr != MEMMGR_PHYS_ADDR_ERROR,
                   "Invalid physical frame",
                   ERR_UNAUTHORIZED_ACTION);
        _ReleaseFrames(frameAddr, 1);
      }

      /* Unmap the memory */
      error = _MemoryUnmap((uintptr_t)kVirtualAddress,
                           pageCount,
                           pMemInfo->PDPhysAddress);
      MEM_ASSERT(error == NO_ERROR,
                 "Invalid unmapping frame",
                 ERR_UNAUTHORIZED_ACTION);

      KERNEL_UNLOCK(pMemInfo->lock);

      /* Release pages */
      _ReleaseUserPages((uintptr_t)kVirtualAddress, pageCount, pProcess);
    }
    else
    {
      error = ERR_INVALID_PARAMETER;
    }

    return error;
}

bool MemoryIsMapped(const uintptr_t  kVirtualAddress,
                    const size_t     size,
                    S_KernelProcess* pProcess,
                    const bool       kCheckFull)
{
  uintptr_t pgdir;
  uintptr_t address;
  size_t    pageCount;
  size_t    alignedSize;

  address = ALIGN_DOWN(kVirtualAddress, KERNEL_PAGE_SIZE);
  alignedSize = size + (kVirtualAddress - address);
  pageCount = alignedSize / KERNEL_PAGE_SIZE;
  if (alignedSize % KERNEL_PAGE_SIZE != 0)
  {
    pageCount += 1;
  }

  pgdir = ((S_ProcessMemoryMetadata*)pProcess->pMemoryData)->PDPhysAddress;
  return _IsMapped(address, pageCount, pgdir, kCheckFull);
}

/************************************ EOF *************************************/