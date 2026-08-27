/*******************************************************************************
 * @file CPU.c
 *
 * @see X64CPU.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief x64 CPU management functions
 *
 * @details x64 CPU manipulation functions. Wraps inline assembly calls for
 * ease of development.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <LAPIC.h>
#include <CPUID.h>
#include <Panic.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <Memory.h>
#include <ProcFS.h>
#include <FastQueue.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>
#include <InterruptHandlers.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <X64Cpu.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "CPU_X64"

/** @brief Stores the procfs cpu entry name */
#define CPUS_PROCFS_DIR_PATH "cpuinfo"
/** @brief CPU info buffer size */
#define CPUINFO_BUFFER_SIZE 4096

/***************************
 * GDT Flags
 **************************/
/** @brief Kernel's 64 bits code segment descriptor. */
#define KERNEL_CS_64 0x08
/** @brief Kernel's 64 bits data segment descriptor. */
#define KERNEL_DS_64 0x10
/** @brief User's 64 bits code segment descriptor. */
#define USER_CS_64 0x18
/** @brief User's 64 bits data segment descriptor. */
#define USER_DS_64 0x20
/** @brief Kernel's TSS segment descriptor. */
#define TSS_SEGMENT 0x30

/** @brief Kernel's 64 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_64  0x00000000
/** @brief Kernel's 64 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_64 0x000FFFFF
/** @brief Kernel's 64 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_64  0x00000000
/** @brief Kernel's 64 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_64 0x000FFFFF

/** @brief User's 64 bits code segment base address. */
#define USER_CODE_SEGMENT_BASE_64  0x00000000
/** @brief User's 64 bits code segment limit address. */
#define USER_CODE_SEGMENT_LIMIT_64 0x000FFFFF
/** @brief User's 64 bits data segment base address. */
#define USER_DATA_SEGMENT_BASE_64  0x00000000
/** @brief User's 64 bits data segment limit address. */
#define USER_DATA_SEGMENT_LIMIT_64 0x000FFFFF

/** @brief GDT Accessed Bit */
#define GDT_ACCESS_BYTE_ACCESSED 0x01
/** @brief GDT Readable / Writeable Bit */
#define GDT_ACCESS_BYTE_WR 0x02
/** @brief GDT Direction Grow Up Bit */
#define GDT_ACCESS_BYTE_GROW_UP 0x00
/** @brief GDT Direction Grow Down Bit */
#define GDT_ACCESS_BYTE_GROW_DOWN 0x04
/** @brief GDT Conforming Clear Bit */
#define GDT_ACCESS_BYTE_NON_CONFORMING 0x00
/** @brief GDT Conforming Set Bit */
#define GDT_ACCESS_BYTE_CONFORMING 0x04
/** @brief GDT Executable Bit */
#define GDT_ACCESS_BYTE_EXEC 0x08
/** @brief GDT System Segment Type 16B TSS Available Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_16B_TSS_AVAIL 0x01
/** @brief GDT System Segment Type LDT Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_LDT 0x02
/** @brief GDT System Segment Type 16B TSS Busy Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_16B_TSS_BUSY 0x03
/** @brief GDT System Segment Type 32B TSS Available Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_32B_TSS_AVAIL 0x09
/** @brief GDT System Segment Type 64B TSS Available Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_64B_TSS_AVAIL 0x09
/** @brief GDT System Segment Type 32B TSS Busy Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_32B_TSS_BUSY 0x0B
/** @brief GDT System Segment Type 64B TSS Busy Bit */
#define GDT_ACCESS_BYTE_SYS_TYPE_64B_TSS_BUSY 0x0B

/** @brief GDT Type System Bit */
#define GDT_ACCESS_BYTE_SYSTEM 0x00
/** @brief GDT Type Code or Data Bit */
#define GDT_ACCESS_BYTE_CODE_DATA 0x10
/** @brief GDT Descriptor Level Ring 0 Bit */
#define GDT_ACCESS_BYTE_RING0 0x00
/** @brief GDT Descriptor Level Ring 1 Bit */
#define GDT_ACCESS_BYTE_RING1 0x20
/** @brief GDT Descriptor Level Ring 2 Bit */
#define GDT_ACCESS_BYTE_RING2 0x40
/** @brief GDT Descriptor Level Ring 3 Bit */
#define GDT_ACCESS_BYTE_RING3 0x60
/** @brief GDT Present Bit */
#define GDT_ACCESS_BYTE_PRESENT 0x80

/** @brief GDT Long Mode Flag */
#define GDT_FLAG_LONGMODE_CODE 0x2
/** @brief GDT DB 16 Bits Flag */
#define GDT_FLAG_DB_16B 0x0
/** @brief GDT DB 32 Bits Flag */
#define GDT_FLAG_DB_32B 0x4
/** @brief GDT Granularity 1B Flag */
#define GDT_FLAG_GRANULARITY_1B 0x0
/** @brief GDT Granularity 4K Flag */
#define GDT_FLAG_GRANULARITY_4K 0x8

/***************************
 * IDT Flags
 **************************/
/** @brief IDT flag: storage segment. */
#define IDT_FLAG_STORAGE_SEG 0x10
/** @brief IDT flag: privilege level, ring 0. */
#define IDT_FLAG_PL0 0x00
/** @brief IDT flag: privilege level, ring 1. */
#define IDT_FLAG_PL1 0x20
/** @brief IDT flag: privilege level, ring 2. */
#define IDT_FLAG_PL2 0x40
/** @brief IDT flag: privilege level, ring 3. */
#define IDT_FLAG_PL3 0x60
/** @brief IDT flag: interrupt present. */
#define IDT_FLAG_PRESENT 0x80

/** @brief IDT flag: interrupt type task gate. */
#define IDT_TYPE_TASK_GATE 0x05
/** @brief IDT flag: interrupt type interrupt gate. */
#define IDT_TYPE_INT_GATE  0x0E
/** @brief IDT flag: interrupt type trap gate. */
#define IDT_TYPE_TRAP_GATE 0x0F

/***************************
 * CPU Interrupt Lines
 **************************/
/** @brief Minimal customizable accepted interrupt line. */
#define MIN_INTERRUPT_LINE 0x00
/** @brief Maximal customizable accepted interrupt line. */
#define MAX_INTERRUPT_LINE (IDT_ENTRY_COUNT - 1)
/** @brief Defines the spurious interrupt line */
#define SPURIOUS_INT_LINE MAX_INTERRUPT_LINE
/** @brief Defines the software interrupt number for scheduling. */
#define SCHEDULER_SW_INT_LINE 0x21
/** @brief Defines the interrupt line used to handle IPIs */
#define CPU_IPI_INT_LINE 0x22

/***************************
 * Misc CPU Definitions
 **************************/
/** @brief CPU MXCSR Precision Interrupt Mask */
#define MXCSR_PRECISION_EXC_MASK 0x00001000
/** @brief CPU WP bit in CR0 */
#define CPU_WP_BIT_CR0 0x10000

/** @brief Thread's initial EFLAGS register value. */
#define KERNEL_THREAD_INIT_RFLAGS 0x202 /* INT | PARITY */
/** @brief Thread's initial EFLAGS register value. */
#define USER_THREAD_INIT_RFLAGS 0x202 /* INT | PARITY */

/** @brief Double fault special stack size */
#define DOUBLE_FAULT_STACK_SIZE 128

/** @brief IPI send flag CPU mask */
#define CPU_IPI_SEND_TO_CPU_MASK (CPU_IPI_SEND_TO(0xFFFFFFFF));

/** @brief Size in number of elements of the IPI queues */
#define IPI_QUEUE_SIZE 50

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief CPU IDT entry. Describes an entry in the IDT.
 */
typedef struct
{
  /** @brief ISR low address. */
  uint16_t offLow;
  /** @brief Code segment selector. */
  uint16_t cSel;
  /** @brief Entry IST number. */
  uint8_t ist;
  /** @brief Entry flags. */
  uint8_t flags;
  /** @brief ISR middle address. */
  uint16_t offMid;
  /** @brief ISR high address. */
  uint32_t offHi;
  /** @brief Must be zero. */
  uint32_t reserved;
} S_CPUIDTEntry;

/**
 * @brief Define the GDT pointer, contains the  address and limit of the GDT.
 */
typedef struct
{
  /** @brief The GDT size. */
  uint16_t size;
  /** @brief The GDT address. */
  uintptr_t base;

  /** @brief Alignement padding. */
  uint8_t padding[6];
}__attribute__((packed)) S_GDTPtr;

/**
 * @brief Define the IDT pointer, contains the  address and limit of the IDT.
 */
typedef struct
{
  /** @brief The IDT size. */
  uint16_t size;
  /** @brief The IDT address. */
  uintptr_t base;
}__attribute__((packed)) S_IDTPtr;

/**
 * @brief CPU TSS abstraction structure. This is the representation the kernel
 * has of an intel's TSS entry.
 */
typedef struct
{
  /** @brief Reserved entry */
  uint32_t reserved0;
  /** @brief RSP for RING0 value. */
  uint64_t rsp0;
  /** @brief RSP for RING1 value. */
  uint64_t rsp1;
  /** @brief RSP for RING2 value */
  uint64_t rsp2;
  /** @brief Reserved entry */
  uint64_t reserved1;
  /** @brief Interrupt ST 1 */
  uint64_t ist1;
  /** @brief Interrupt ST 2 */
  uint64_t ist2;
  /** @brief Interrupt ST 3 */
  uint64_t ist3;
  /** @brief Interrupt ST 4 */
  uint64_t ist4;
  /** @brief Interrupt ST 5 */
  uint64_t ist5;
  /** @brief Interrupt ST 6 */
  uint64_t ist6;
  /** @brief Interrupt ST 7 */
  uint64_t ist7;
  /** @brief Reserved entry */
  uint64_t reserved2;
  /** @brief IO privileges map */
  uint16_t ioMapBase;
  /** @brief Reserved entry */
  uint16_t reserved3;
} __attribute__((__packed__)) S_CPUTSSEntry;

/** @brief CPU config structure */
typedef struct
{
  /** @brief CPU GDT space in memory. */
  uint64_t gdt[CPU_GDT_SIZE];
  /** @brief Kernel GDT structure pointer */
  S_GDTPtr gdtPtr;
  /** @brief CPU TSS space in memory. */
  S_CPUTSSEntry tss;
  /** @brief Pointer to the end of the kernel stack for the CPU. */
  uintptr_t kernelStackEnd;
  /** @brief CPU physical addressing width */
  uint8_t physAddressWidth;
  /** @brief CPU virtual addressing width */
  uint8_t virtAddressWidth;
  /** @brief CPU virtual 1GB page support */
  bool cpu1GBPageSupport;
  /** @brief Store the advances CPU information */
  S_CPUInformation cpuInfo;
  /** @brief Stores the kernel-allocated CPU Id */
  uint32_t cpuId;
} S_CPUConfig;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define CPU_ASSERT(COND, MSG, ERROR) {            \
  if ((COND) == false)                            \
  {                                               \
    PANIC(ERROR, MODULE_NAME, MSG, false, false); \
  }                                               \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Setups the generic kernel's IDT in memory and loads it in the IDT
 * register.
 *
 * @details Setups a simple IDT for the kernel. Fills the entries in the IDT
 * table by adding basic support to the x86 exception (interrutps 0 to 32).
 * The rest of the interrupts are not set.
 */
static void _SetupIDT(void);

/**
 * @brief Setups the kernel's GDT in memory and loads it in the GDT register.
 *
 * @param[out] pCPUConfig CPU configuration for which the GDT is initialized.
 *
 * @details Setups a GDT for the kernel. Fills the entries in the GDT table and
 * load the new GDT in the CPU's GDT register.
 * Once done, the function sets the segment registers (CS, DS, ES, FS, GS, SS)
 * of the CPU according to the kernel's settings.
 */
static void _SetupGDT(S_CPUConfig* pCPUConfig);


/**
 *  @brief Setups the main CPU TSS for the kernel.
 *
 * @param[out] pCPUConfig CPU configuration for which the TSS is initialized.
 *
 * @details Initializes the main CPU's TSS with kernel settings in memory and
 * loads it in the TSS register.
 */
static void _SetupTSS(S_CPUConfig* pCPUConfig);

/**
 * @brief Formats a GDT entry.
 *
 * @details Formats data given as parameter into a standard GDT entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] pEntry The pointer to the entry structure to format.
 * @param[in] kBase  The base address of the segment for the GDT entry.
 * @param[in] kLimit The limit address of the segment for the GDT entry.
 * @param[in] kAccess The access bits of segment for the GDT entry.
 * @param[in] kFlags The flags to be set for the GDT entry.
 */
static void _FormatGDTEntry(uint64_t*      pEntry,
                            const uint32_t kBase,
                            const uint32_t kLimit,
                            const uint8_t  kAccess,
                            const uint8_t  kFlags);

/**
 * @brief Formats a TSS entry.
 *
 * @details Formats data given as parameter into a standard TSS entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] pEntry The pointer to the entry structure to format.
 * @param[in] kBase  The base address of the segment for the TSS entry.
 * @param[in] kSize The size of the segment for the TSS entry.
 * @param[in] kAccess  The access byte of segment for the TSS entry.
 * @param[in] kFlags The flags to be set for the TSS entry.
 */
static void _FormatTSSEntry(uint64_t*      pEntry,
                            const uint64_t kBase,
                            const uint32_t kSize,
                            const uint8_t  kAccess,
                            const uint8_t  kFlags);

/**
 * @brief Formats an IDT entry.
 *
 * @details Formats data given as parameter into a standard IDT entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] pEntry The pointer to the entry structure to format.
 * @param[in] kandler The handler function for the IDT entry.
 * @param[in] kType  The type of segment for the IDT entry.
 * @param[in] kFlags The flags to be set for the IDT entry.
 * @param[in] kIst The IST to be set for the IDT entry.
 */
static void _FormatIDTEntry(S_CPUIDTEntry*  pEntry,
                            const uintptr_t kHandler,
                            const uint8_t   kType,
                            const uint32_t  kFlags,
                            const uint8_t   kIst);

/**
 * @brief Attaches the CPU Manager driver to the system.
 *
 * @details Attaches the CPU Manager driver to the system. This function will
 * use the FDT to initialize the CPU Manager hardware and retreive the CPU
 * Manager parameters.
 *
 * @param[in] kpNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _CPUAttach(const S_FDTNode* kpNode);

/**
 * @brief Checks the architecture's feature and requirements for roOs.
 *
 * @details Checks the architecture's feature and requirements for roOs. If a
 * requirement is not met, a kernel panic is raised.
 */
static void _ValidateArchitecture(void);

/**
 * @brief IPI interrupt handler.
 *
 * @details IPI interrupt handler. Based on the IPI parameters, the handler
 * dispatches the IPI request.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _IPIInterruptHandler(void);

/**
 * @brief Initializes the IPI mechanism.
 *
 * @details Initializes the IPI mechanism. This will create the IPI queues and
 * register the IPI interrupt line.
 */
static void _InitializeIPI(void);

/**
 * @brief Opens the ProcFS main entry.
 *
 * @details Opens the ProcFS main entry. This function will open the
 * ProcFS main entry and return a pointer to the file handle. The file handle
 * will be used to read the information from the kernel.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] kpPath The path to the ProcFS entry.
 * @param[in] flags The flags for opening the file.
 * @param[in] mode The mode for opening the file.
 *
 * @return The pointer to the file handle or -1 in case of error.
 */
static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode);

/**
 * @brief Closes the ProcFS main entry.
 *
 * @details Closes the ProcFS main entry. This function will close the
 * ProcFS main entry and free the resources used by the entry.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle);

/**
 * @brief Read the ProcFS main entry.
 *
 * @details Read the ProcFS main entry. This function will read the
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count);

/**
 * @brief Creates the ProcFS entry for the driver.
 *
 * @details Creates the ProcFS entry for the driver. This function will
 * create the ProcFS entry for the driver and register it in the ProcFS.
 * The entry will be used to read the information from the kernel. The
 * entry will be created in the /proc/cpuinfo directory.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateProcFSEntry(void);

/**
 * @brief Creates the CPU information string.
 * @details Creates the CPU information string. This function will fill the
 * buffer as much as possible and return the generated size in pInfoSize.
 *
 * @param[in] kpInfo The CPU information for which the string should be
 * generated.
 * @param[out] pInfoBuffer The buffer used to store the generated string.
 * @param[in, out] pInfoSize The buffer size. This is updated oncethe generation
 * is finished, with the actual size of the generated string.
 */
static void _GenerateCPUInfoString(const S_CPUInformation* kpInfo,
                                   char*                   pInfoBuffer,
                                   size_t*                 pInfoSize);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel stacks base symbol. */
extern int8_t _KERNEL_STACKS_BASE;
/** @brief Stores the number of CPU that booted. */
extern volatile uint32_t _bootedCPUCount;
/** @brief Stores the support for xsaveopt. */
extern uint32_t _xSaveoptSupported;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief CPU configuration table. */
static S_CPUConfig* spCPUConfiguration[SOC_MAX_CPU_COUNT];

/**@brief Stores the number of detected CPUs */
static uint32_t sNumberOfCPUs;

/** @brief CPU IDT space in memory. */
static S_CPUIDTEntry sIDT[IDT_ENTRY_COUNT] __attribute__((aligned(8)));

/** @brief Kernel IDT structure */
static S_IDTPtr sIDTPtr __attribute__((aligned(8)));

/** @brief Stores the Double Fault Exception Special Stack */
static uint8_t sDFStack[DOUBLE_FAULT_STACK_SIZE * SOC_MAX_CPU_COUNT] __attribute__((aligned(8)));

/** @brief Queues used to communicate with IPIs. */
static S_FastQueue*** spIPIRequestQueue;

/** @brief Stores the LAPIC driver instance */
static const S_LAPICDriver* kspLAPICDriver = NULL;

/** @brief Stores the LAPIC timer driver instance */
static const S_LAPICTimerDriver* kspLAPICTimerDriver = NULL;

/** @brief Stores the CPUs LAPIS identifiers. */
static uint32_t spCPUIds[SOC_MAX_CPU_COUNT];

/** @brief Stores the booted CPU state. */
static volatile bool sAllCPUBooted;

/** @brief Stores the CPU interrupt handlers entry point */
static uintptr_t sIntHandlerTable[IDT_ENTRY_COUNT] =
{
  (uintptr_t)IntHandler0,
  (uintptr_t)IntHandler1,
  (uintptr_t)IntHandler2,
  (uintptr_t)IntHandler3,
  (uintptr_t)IntHandler4,
  (uintptr_t)IntHandler5,
  (uintptr_t)IntHandler6,
  (uintptr_t)IntHandler7,
  (uintptr_t)IntHandler8,
  (uintptr_t)IntHandler9,
  (uintptr_t)IntHandler10,
  (uintptr_t)IntHandler11,
  (uintptr_t)IntHandler12,
  (uintptr_t)IntHandler13,
  (uintptr_t)IntHandler14,
  (uintptr_t)IntHandler15,
  (uintptr_t)IntHandler16,
  (uintptr_t)IntHandler17,
  (uintptr_t)IntHandler18,
  (uintptr_t)IntHandler19,
  (uintptr_t)IntHandler20,
  (uintptr_t)IntHandler21,
  (uintptr_t)IntHandler22,
  (uintptr_t)IntHandler23,
  (uintptr_t)IntHandler24,
  (uintptr_t)IntHandler25,
  (uintptr_t)IntHandler26,
  (uintptr_t)IntHandler27,
  (uintptr_t)IntHandler28,
  (uintptr_t)IntHandler29,
  (uintptr_t)IntHandler30,
  (uintptr_t)IntHandler31,
  (uintptr_t)IntHandler32,
  (uintptr_t)IntHandler33,
  (uintptr_t)IntHandler34,
  (uintptr_t)IntHandler35,
  (uintptr_t)IntHandler36,
  (uintptr_t)IntHandler37,
  (uintptr_t)IntHandler38,
  (uintptr_t)IntHandler39,
  (uintptr_t)IntHandler40,
  (uintptr_t)IntHandler41,
  (uintptr_t)IntHandler42,
  (uintptr_t)IntHandler43,
  (uintptr_t)IntHandler44,
  (uintptr_t)IntHandler45,
  (uintptr_t)IntHandler46,
  (uintptr_t)IntHandler47,
  (uintptr_t)IntHandler48,
  (uintptr_t)IntHandler49,
  (uintptr_t)IntHandler50,
  (uintptr_t)IntHandler51,
  (uintptr_t)IntHandler52,
  (uintptr_t)IntHandler53,
  (uintptr_t)IntHandler54,
  (uintptr_t)IntHandler55,
  (uintptr_t)IntHandler56,
  (uintptr_t)IntHandler57,
  (uintptr_t)IntHandler58,
  (uintptr_t)IntHandler59,
  (uintptr_t)IntHandler60,
  (uintptr_t)IntHandler61,
  (uintptr_t)IntHandler62,
  (uintptr_t)IntHandler63,
  (uintptr_t)IntHandler64,
  (uintptr_t)IntHandler65,
  (uintptr_t)IntHandler66,
  (uintptr_t)IntHandler67,
  (uintptr_t)IntHandler68,
  (uintptr_t)IntHandler69,
  (uintptr_t)IntHandler70,
  (uintptr_t)IntHandler71,
  (uintptr_t)IntHandler72,
  (uintptr_t)IntHandler73,
  (uintptr_t)IntHandler74,
  (uintptr_t)IntHandler75,
  (uintptr_t)IntHandler76,
  (uintptr_t)IntHandler77,
  (uintptr_t)IntHandler78,
  (uintptr_t)IntHandler79,
  (uintptr_t)IntHandler80,
  (uintptr_t)IntHandler81,
  (uintptr_t)IntHandler82,
  (uintptr_t)IntHandler83,
  (uintptr_t)IntHandler84,
  (uintptr_t)IntHandler85,
  (uintptr_t)IntHandler86,
  (uintptr_t)IntHandler87,
  (uintptr_t)IntHandler88,
  (uintptr_t)IntHandler89,
  (uintptr_t)IntHandler90,
  (uintptr_t)IntHandler91,
  (uintptr_t)IntHandler92,
  (uintptr_t)IntHandler93,
  (uintptr_t)IntHandler94,
  (uintptr_t)IntHandler95,
  (uintptr_t)IntHandler96,
  (uintptr_t)IntHandler97,
  (uintptr_t)IntHandler98,
  (uintptr_t)IntHandler99,
  (uintptr_t)IntHandler100,
  (uintptr_t)IntHandler101,
  (uintptr_t)IntHandler102,
  (uintptr_t)IntHandler103,
  (uintptr_t)IntHandler104,
  (uintptr_t)IntHandler105,
  (uintptr_t)IntHandler106,
  (uintptr_t)IntHandler107,
  (uintptr_t)IntHandler108,
  (uintptr_t)IntHandler109,
  (uintptr_t)IntHandler110,
  (uintptr_t)IntHandler111,
  (uintptr_t)IntHandler112,
  (uintptr_t)IntHandler113,
  (uintptr_t)IntHandler114,
  (uintptr_t)IntHandler115,
  (uintptr_t)IntHandler116,
  (uintptr_t)IntHandler117,
  (uintptr_t)IntHandler118,
  (uintptr_t)IntHandler119,
  (uintptr_t)IntHandler120,
  (uintptr_t)IntHandler121,
  (uintptr_t)IntHandler122,
  (uintptr_t)IntHandler123,
  (uintptr_t)IntHandler124,
  (uintptr_t)IntHandler125,
  (uintptr_t)IntHandler126,
  (uintptr_t)IntHandler127,
  (uintptr_t)IntHandler128,
  (uintptr_t)IntHandler129,
  (uintptr_t)IntHandler130,
  (uintptr_t)IntHandler131,
  (uintptr_t)IntHandler132,
  (uintptr_t)IntHandler133,
  (uintptr_t)IntHandler134,
  (uintptr_t)IntHandler135,
  (uintptr_t)IntHandler136,
  (uintptr_t)IntHandler137,
  (uintptr_t)IntHandler138,
  (uintptr_t)IntHandler139,
  (uintptr_t)IntHandler140,
  (uintptr_t)IntHandler141,
  (uintptr_t)IntHandler142,
  (uintptr_t)IntHandler143,
  (uintptr_t)IntHandler144,
  (uintptr_t)IntHandler145,
  (uintptr_t)IntHandler146,
  (uintptr_t)IntHandler147,
  (uintptr_t)IntHandler148,
  (uintptr_t)IntHandler149,
  (uintptr_t)IntHandler150,
  (uintptr_t)IntHandler151,
  (uintptr_t)IntHandler152,
  (uintptr_t)IntHandler153,
  (uintptr_t)IntHandler154,
  (uintptr_t)IntHandler155,
  (uintptr_t)IntHandler156,
  (uintptr_t)IntHandler157,
  (uintptr_t)IntHandler158,
  (uintptr_t)IntHandler159,
  (uintptr_t)IntHandler160,
  (uintptr_t)IntHandler161,
  (uintptr_t)IntHandler162,
  (uintptr_t)IntHandler163,
  (uintptr_t)IntHandler164,
  (uintptr_t)IntHandler165,
  (uintptr_t)IntHandler166,
  (uintptr_t)IntHandler167,
  (uintptr_t)IntHandler168,
  (uintptr_t)IntHandler169,
  (uintptr_t)IntHandler170,
  (uintptr_t)IntHandler171,
  (uintptr_t)IntHandler172,
  (uintptr_t)IntHandler173,
  (uintptr_t)IntHandler174,
  (uintptr_t)IntHandler175,
  (uintptr_t)IntHandler176,
  (uintptr_t)IntHandler177,
  (uintptr_t)IntHandler178,
  (uintptr_t)IntHandler179,
  (uintptr_t)IntHandler180,
  (uintptr_t)IntHandler181,
  (uintptr_t)IntHandler182,
  (uintptr_t)IntHandler183,
  (uintptr_t)IntHandler184,
  (uintptr_t)IntHandler185,
  (uintptr_t)IntHandler186,
  (uintptr_t)IntHandler187,
  (uintptr_t)IntHandler188,
  (uintptr_t)IntHandler189,
  (uintptr_t)IntHandler190,
  (uintptr_t)IntHandler191,
  (uintptr_t)IntHandler192,
  (uintptr_t)IntHandler193,
  (uintptr_t)IntHandler194,
  (uintptr_t)IntHandler195,
  (uintptr_t)IntHandler196,
  (uintptr_t)IntHandler197,
  (uintptr_t)IntHandler198,
  (uintptr_t)IntHandler199,
  (uintptr_t)IntHandler200,
  (uintptr_t)IntHandler201,
  (uintptr_t)IntHandler202,
  (uintptr_t)IntHandler203,
  (uintptr_t)IntHandler204,
  (uintptr_t)IntHandler205,
  (uintptr_t)IntHandler206,
  (uintptr_t)IntHandler207,
  (uintptr_t)IntHandler208,
  (uintptr_t)IntHandler209,
  (uintptr_t)IntHandler210,
  (uintptr_t)IntHandler211,
  (uintptr_t)IntHandler212,
  (uintptr_t)IntHandler213,
  (uintptr_t)IntHandler214,
  (uintptr_t)IntHandler215,
  (uintptr_t)IntHandler216,
  (uintptr_t)IntHandler217,
  (uintptr_t)IntHandler218,
  (uintptr_t)IntHandler219,
  (uintptr_t)IntHandler220,
  (uintptr_t)IntHandler221,
  (uintptr_t)IntHandler222,
  (uintptr_t)IntHandler223,
  (uintptr_t)IntHandler224,
  (uintptr_t)IntHandler225,
  (uintptr_t)IntHandler226,
  (uintptr_t)IntHandler227,
  (uintptr_t)IntHandler228,
  (uintptr_t)IntHandler229,
  (uintptr_t)IntHandler230,
  (uintptr_t)IntHandler231,
  (uintptr_t)IntHandler232,
  (uintptr_t)IntHandler233,
  (uintptr_t)IntHandler234,
  (uintptr_t)IntHandler235,
  (uintptr_t)IntHandler236,
  (uintptr_t)IntHandler237,
  (uintptr_t)IntHandler238,
  (uintptr_t)IntHandler239,
  (uintptr_t)IntHandler240,
  (uintptr_t)IntHandler241,
  (uintptr_t)IntHandler242,
  (uintptr_t)IntHandler243,
  (uintptr_t)IntHandler244,
  (uintptr_t)IntHandler245,
  (uintptr_t)IntHandler246,
  (uintptr_t)IntHandler247,
  (uintptr_t)IntHandler248,
  (uintptr_t)IntHandler249,
  (uintptr_t)IntHandler250,
  (uintptr_t)IntHandler251,
  (uintptr_t)IntHandler252,
  (uintptr_t)IntHandler253,
  (uintptr_t)IntHandler254,
  (uintptr_t)IntHandler255
};

/** @brief Defines the CPU interrupt configuration */
const S_CPUInterruptConfiguration ksInterruptConfig =
{
  .minInterruptLine       = MIN_INTERRUPT_LINE,
  .maxInterruptLine       = MAX_INTERRUPT_LINE,
  .schedulerInterruptLine = SCHEDULER_SW_INT_LINE,
  .spuriousInterruptLine  = SPURIOUS_INT_LINE,
  .ipiInterruptLine       = CPU_IPI_INT_LINE
};

/** @brief CPU driver instance. */
static S_Driver sX86CPUDriver =
{
  .pName         = "X86 CPU Driver",
  .pDescription  = "X86 CPU Driver for roOs",
  .pCompatible   = "generic,x86_64",
  .pVersion      = "1.0",
  .pDriverAttach = _CPUAttach
};

/** @brief ProcFS entry for the CPU driver  */
static S_ProcFSDirEntry* spProcFSEntry = NULL;

/** @brief PROCFS operations */
static S_ProcFSFileOperations sProcFSOps =
{
  .pOpen    = _ProcFSOpen,
  .pClose   = _ProcFSClose,
  .pRead    = _ProcFSRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _SetupIDT(void)
{
  uint32_t i;

  /* Blank the IDT */
  memset(sIDT, 0, sizeof(S_CPUIDTEntry) * IDT_ENTRY_COUNT);

  /* Set interrupt handlers for each interrupt */
  for (i = 0; i < IDT_ENTRY_COUNT; ++i)
  {
    if (i == DOUBLE_FAULT_EXC_LINE)
    {
      _FormatIDTEntry(&sIDT[i],
                      sIntHandlerTable[i],
                      IDT_TYPE_INT_GATE,
                      IDT_FLAG_PRESENT | IDT_FLAG_PL0,
                      1);
    }
    else
    {
      _FormatIDTEntry(&sIDT[i],
                      sIntHandlerTable[i],
                      IDT_TYPE_INT_GATE,
                      IDT_FLAG_PRESENT | IDT_FLAG_PL0,
                      0);
    }
  }

  /* Set the IDT descriptor */
  sIDTPtr.size = ((sizeof(S_CPUIDTEntry) * IDT_ENTRY_COUNT) - 1);
  sIDTPtr.base = (uintptr_t)&sIDT;

  /* Load the IDT */
  __asm__ __volatile__("lidt %0"
                       :
                       : "m" (sIDTPtr.size), "m" (sIDTPtr.base));

}

static void _SetupGDT(S_CPUConfig* pCPUConfig)
{
  /************************************
   * KERNEL GDT ENTRIES
   ***********************************/
  /* Set the kernel 64 bits code descriptor */
  const uint32_t kKernelCode64SegFlags  = GDT_FLAG_LONGMODE_CODE |
                                          GDT_FLAG_GRANULARITY_4K;
  const uint32_t kKernelCode64SegAccess = GDT_ACCESS_BYTE_EXEC           |
                                          GDT_ACCESS_BYTE_WR             |
                                          GDT_ACCESS_BYTE_CODE_DATA      |
                                          GDT_ACCESS_BYTE_PRESENT        |
                                          GDT_ACCESS_BYTE_NON_CONFORMING |
                                          GDT_ACCESS_BYTE_RING0;

  /* Set the kernel 64 bits data descriptor */
  const uint32_t kKernelData64SegFlags  = GDT_FLAG_DB_32B |
                                          GDT_FLAG_GRANULARITY_4K;
  const uint32_t kKernelData64SegAccess = GDT_ACCESS_BYTE_WR        |
                                          GDT_ACCESS_BYTE_CODE_DATA |
                                          GDT_ACCESS_BYTE_PRESENT   |
                                          GDT_ACCESS_BYTE_GROW_UP   |
                                          GDT_ACCESS_BYTE_RING0;

  /* Set the user 64 bits code descriptor */
  const uint32_t kUserCode64SegFlags  = GDT_FLAG_LONGMODE_CODE |
                                        GDT_FLAG_GRANULARITY_4K;
  const uint32_t kUserCode64SegAccess = GDT_ACCESS_BYTE_EXEC           |
                                        GDT_ACCESS_BYTE_WR             |
                                        GDT_ACCESS_BYTE_CODE_DATA      |
                                        GDT_ACCESS_BYTE_PRESENT        |
                                        GDT_ACCESS_BYTE_NON_CONFORMING |
                                        GDT_ACCESS_BYTE_RING3;

  /* Set the user 64 bits data descriptor */
  const uint32_t kUserData64SegFlags = GDT_FLAG_DB_32B |
                                       GDT_FLAG_GRANULARITY_4K;

  const uint32_t kUserData64SegAccess = GDT_ACCESS_BYTE_WR        |
                                        GDT_ACCESS_BYTE_CODE_DATA |
                                        GDT_ACCESS_BYTE_PRESENT   |
                                        GDT_ACCESS_BYTE_GROW_UP   |
                                        GDT_ACCESS_BYTE_RING3;

  /************************************
   * TSS ENTRY
   ***********************************/
  const uint32_t kTssSegFlags  = 0;
  const uint32_t kTssSegAccess = GDT_ACCESS_BYTE_EXEC     |
                                 GDT_ACCESS_BYTE_ACCESSED |
                                 GDT_ACCESS_BYTE_PRESENT  |
                                 GDT_ACCESS_BYTE_SYSTEM;

  /* Blank the GDT, set the NULL descriptor */
  memset(&pCPUConfig->gdt, 0, CPU_GDT_SIZE);

  /* Load the segments */
  _FormatGDTEntry(&pCPUConfig->gdt[KERNEL_CS_64 / 8],
                  KERNEL_CODE_SEGMENT_BASE_64,
                  KERNEL_CODE_SEGMENT_LIMIT_64,
                  kKernelCode64SegAccess,
                  kKernelCode64SegFlags);

  _FormatGDTEntry(&pCPUConfig->gdt[KERNEL_DS_64 / 8],
                  KERNEL_DATA_SEGMENT_BASE_64,
                  KERNEL_DATA_SEGMENT_LIMIT_64,
                  kKernelData64SegAccess,
                  kKernelData64SegFlags);

  _FormatGDTEntry(&pCPUConfig->gdt[USER_CS_64 / 8],
                  USER_CODE_SEGMENT_BASE_64,
                  USER_CODE_SEGMENT_LIMIT_64,
                  kUserCode64SegAccess,
                  kUserCode64SegFlags);

  _FormatGDTEntry(&pCPUConfig->gdt[USER_DS_64 / 8],
                  USER_DATA_SEGMENT_BASE_64,
                  USER_DATA_SEGMENT_LIMIT_64,
                  kUserData64SegAccess,
                  kUserData64SegFlags);

  _FormatTSSEntry(&pCPUConfig->gdt[TSS_SEGMENT / 8],
                  (uintptr_t)&pCPUConfig->tss,
                  sizeof(S_CPUTSSEntry) - 1,
                  kTssSegAccess,
                  kTssSegFlags);

  /* Set the GDT descriptor */
  pCPUConfig->gdtPtr.size = (CPU_GDT_SIZE - 1);
  pCPUConfig->gdtPtr.base = (uintptr_t)&pCPUConfig->gdt;

  /* Load the GDT */
  __asm__ __volatile__("lgdt %0"
                       :
                       : "m" (pCPUConfig->gdtPtr.size),
                         "m" (pCPUConfig->gdtPtr.base));

  /* Load segment selectors with a far jump for CS*/
  __asm__ __volatile__("movw %w0,%%ds\n\t"
                       "movw %w0,%%es\n\t"
                       "movw %w0,%%fs\n\t"
                       "movw %w0,%%gs\n\t"
                       "movw %w0,%%ss\n\t"
                       :
                       : "r" (KERNEL_DS_64));
  __asm__ __volatile__("mov %0, %%rax\n\t"
                       "push %%rax\n\t"
                       "movabs $new_gdt_seg_, %%rax\n\t"
                       "push %%rax\n\t"
                       "lretq\n\t"
                       "new_gdt_seg_: \n\t"
                       :
                       : "i" (KERNEL_CS_64)
                       : "rax");

  /* Load the TSS */
  __asm__ __volatile__("ltr %0"
                       :
                       : "rm" ((uint16_t)(TSS_SEGMENT)));
}

static void _SetupTSS(S_CPUConfig* pCPUConfig)
{
  /* Blank the TSS */
  memset(&pCPUConfig->tss, 0, sizeof(S_CPUTSSEntry));

  /* Setup the ISTs */
  pCPUConfig->tss.ist1      = (uintptr_t)sDFStack +
                              (DOUBLE_FAULT_STACK_SIZE *
                               (pCPUConfig->cpuId + 1)) - ALIGN_16_BYTES;
  pCPUConfig->tss.rsp0      = ALIGN_DOWN(pCPUConfig->kernelStackEnd -
                                         ALIGN_16_BYTES,
                                         ALIGN_16_BYTES);
  pCPUConfig->tss.ioMapBase = sizeof(S_CPUTSSEntry);
}

static void _FormatGDTEntry(uint64_t*      pEntry,
                            const uint32_t kBase,
                            const uint32_t kLimit,
                            const uint8_t  kAccess,
                            const uint8_t  kFlags)
{
  *((uint32_t*)pEntry) = ((kBase & 0xFFFF) << 16) | (kLimit & 0xFFFF);
  *(((uint32_t*)pEntry) + 1) = ((kBase >> 16) & 0xFF) |
                               (kAccess << 8)         |
                               (kLimit & 0x000F0000)  |
                               ((kFlags & 0xF) << 20) |
                               (kBase & 0xFF000000);
}

static void _FormatTSSEntry(uint64_t*      pEntry,
                            const uint64_t kBase,
                            const uint32_t kSize,
                            const uint8_t  kAccess,
                            const uint8_t  kFlags)
{
  *((uint32_t*)pEntry) = ((kBase & 0xFFFF) << 16) | (kSize & 0xFFFF);
  *(((uint32_t*)pEntry) + 1) = ((kBase >> 16) & 0xFF) |
                               (kAccess << 8)         |
                               (kSize & 0x000F0000)   |
                               (kFlags << 20)         |
                               (kBase & 0xFF000000);
  *(((uint32_t*)pEntry) + 2) = (kBase >> 32) & 0xFFFFFFFF;
  *(((uint32_t*)pEntry) + 3) = 0;
}

static void _FormatIDTEntry(S_CPUIDTEntry*  pEntry,
                            const uintptr_t kHandler,
                            const uint8_t   kType,
                            const uint32_t  kFlags,
                            const uint8_t   kIst)
{
  /* Set offset */
  pEntry->offLow = kHandler & 0x000000000000FFFF;
  pEntry->offMid = (kHandler >> 16) & 0x000000000000FFFF;
  pEntry->offHi  = (kHandler >> 32) & 0x00000000FFFFFFFF;

  /* Set selector and flags */
  pEntry->cSel  = KERNEL_CS_64;
  pEntry->flags = (kFlags & 0xF0) | (kType & 0x0F);

  /* Set the rest of the attributes */
  pEntry->ist      = kIst;
  pEntry->reserved = 0;
}

static E_Return _CPUAttach(const S_FDTNode* kpNode)
{
  (void)kpNode;
  return NO_ERROR;
}

static void _ValidateArchitecture(void)
{
  uint32_t          cr0Reg;
  uint32_t          cpuId;
  S_CPUInformation* pNewCpuInfo;

  cpuId = CPUGetId();

  /* Link */
  pNewCpuInfo = &spCPUConfiguration[cpuId]->cpuInfo;

  /* CPU identifier */
  pNewCpuInfo->id = cpuId;

  /* Get the informations from CPUID */
  CPUIDAnalyzeCPU(pNewCpuInfo);

  /* Validate basic features */
  CPU_ASSERT(pNewCpuInfo->flags.fpu,
             "CPU does not support FPU",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.tsc,
             "CPU does not support TSC",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.apic,
             "CPU does not support APIC",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.pat,
             "CPU does not support PAT",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.fxsr,
             "CPU does not support FX instructions",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.xsave,
             "CPU does not support XSAVE/XRSTOR instructions",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.sse,
             "CPU does not support SSE",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.sse2,
             "CPU does not support SSE2",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.lm,
             "CPU is not 64 bits",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.lahf_lm,
             "CPU is not 64 bits",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->flags.syscall,
             "CPU does not support SYSCALL",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(pNewCpuInfo->physAddressWidth != 0 &&
             pNewCpuInfo->virtAddressWidth != 0,
             "CPU addressing width unavailable",
             ERR_NOT_SUPPORTED);
  CPU_ASSERT(KERNEL_VIRTUAL_ADDR_WIDTH == pNewCpuInfo->virtAddressWidth,
             "CPU addressing width incompatible with virtual address width",
             ERR_NOT_SUPPORTED);

  spCPUConfiguration[cpuId]->cpu1GBPageSupport = pNewCpuInfo->flags.page1gb;
  spCPUConfiguration[cpuId]->physAddressWidth = pNewCpuInfo->physAddressWidth;
  spCPUConfiguration[cpuId]->virtAddressWidth = pNewCpuInfo->virtAddressWidth;

  /* Save the global support to xsaveopt */
  _xSaveoptSupported = pNewCpuInfo->flags.xsaveopt;

  if (pNewCpuInfo->id != 0)
  {
    /* Validate uniformity*/
    CPU_ASSERT(
      (pNewCpuInfo->flags.page1gb == spCPUConfiguration[0]->cpu1GBPageSupport &&
       pNewCpuInfo->physAddressWidth == spCPUConfiguration[0]->physAddressWidth &&
       pNewCpuInfo->virtAddressWidth == spCPUConfiguration[0]->virtAddressWidth),
      "Heterogenous configuration detected.",
      ERR_NOT_SUPPORTED);
  }

  /* Get the WP bit */
  __asm__ __volatile__ ("mov %%cr0, %%rax\n\t"
                        "mov %%eax, %0\n\t"
                        : "=m" (cr0Reg)
                        : /* no input */
                        : "%rax");
  pNewCpuInfo->wp = (cr0Reg & CPU_WP_BIT_CR0) == CPU_WP_BIT_CR0;
}

static bool _IPIInterruptHandler(void)
{
  S_IPIParameters params;
  bool            available;
  bool            doSchedule;
  uint32_t        i;
  uint32_t        cpuId;

  InterruptSetEOI(ksInterruptConfig.ipiInterruptLine);

  /* Get the next IPI request */
  doSchedule = false;
  cpuId = CPUGetId();
  for (i = 0; i < sNumberOfCPUs; ++i)
  {
    available = FQueuePop(spIPIRequestQueue[cpuId][i], (void*)&params);
    while (available == true)
    {
      /* Dispatch */
      switch (params.function)
      {
        case IPI_FUNC_PANIC:
          KernelPanicSecondary();
          break;
        case IPI_FUNC_TLB_INVAL:
          CPUInvalidateTLBEntry((uintptr_t)params.pData);
          break;
        case IPI_FUNC_SCHEDULE:
          doSchedule = true;
          break;
        default:
          PANIC(ERR_EXCEEDED_LIMIT,
                MODULE_NAME,
                "Unknown IPI function",
                false,
                false);
      }
      available = FQueuePop(spIPIRequestQueue[cpuId][i], (void*)&params);
    }
  }

  return doSchedule;
}

static void _InitializeIPI(void)
{
  uint32_t i;
  uint32_t j;
  E_Return error;

  /* Create the IPI queues */
  spIPIRequestQueue = KMalloc(sizeof(S_FastQueue**) * sNumberOfCPUs,
                              KMALLOC_NO_FREE_POOL);
  for (i = 0; i < sNumberOfCPUs; ++i)
  {
    spIPIRequestQueue[i] = KMalloc(sizeof(S_FastQueue*) * sNumberOfCPUs,
                                   KMALLOC_NO_FREE_POOL);
    for (j = 0; j < sNumberOfCPUs; ++j)
    {
      spIPIRequestQueue[i][j] = FQueueCreate(IPI_QUEUE_SIZE,
                                             sizeof(S_IPIParameters));
    }
  }

  /* Register the IPI interrupt */
  error = InterruptRegister(ksInterruptConfig.ipiInterruptLine,
                            _IPIInterruptHandler,
                            false);
  CPU_ASSERT(error == NO_ERROR, "Failed to register IPI interrupt", error);
}

void CPUInit(void)
{
  E_Return error;

  sNumberOfCPUs = 1;

  /* Setup the shared IDT */
  _SetupIDT();

  spCPUConfiguration[0] = KMalloc(sizeof(S_CPUConfig), KMALLOC_NO_FREE_POOL);
  /* Set the main CPU kernel stack */
  spCPUConfiguration[0]->kernelStackEnd = ((uintptr_t)&_KERNEL_STACKS_BASE) +
                                           KERNEL_STACK_SIZE - 1;
  spCPUConfiguration[0]->cpuId = 0;
  /* Setup the main CPU GDT and TSS */
  _SetupTSS(spCPUConfiguration[0]);
  _SetupGDT(spCPUConfiguration[0]);

  /* Validate architecture */
  _ValidateArchitecture();

  /* Init the procfs entries */
  error = _CreateProcFSEntry();
  CPU_ASSERT(error == NO_ERROR, "Failed to create ProcFS entry", error);

  sAllCPUBooted = false;
}

void CPUStartSMP(void)
{
  const S_LAPICNode* kpLapicNode;

  /* Check if the LAPIC driver was registered */
  CPU_ASSERT(kspLAPICDriver != NULL, "No LAPIC driver.", ERR_NOT_SUPPORTED);
  CPU_ASSERT(_bootedCPUCount == 1,
             "Multiple CPUs already started.",
             ERR_UNAUTHORIZED_ACTION);

  /* Init the current CPU information */
  spCPUIds[0] = kspLAPICDriver->pGetLAPICId();

  /* Initialize IPIs */
  _InitializeIPI();

  /* Check if we need to enable more CPUs */
  kpLapicNode = kspLAPICDriver->pGetLAPICList();
  while (kpLapicNode != NULL && _bootedCPUCount < SOC_MAX_CPU_COUNT)
  {
    /* If not self */
    if (spCPUIds[0] != kpLapicNode->lapic.lapicId)
    {
      /* Check if CPU can be started */
      if ((kpLapicNode->lapic.flags & 1) != 0)
      {
        /* Start the CPU */
        kspLAPICDriver->pStartCpu(kpLapicNode->lapic.lapicId);
      }
    }

    /* Go to next */
    kpLapicNode = kpLapicNode->pNext;
  }

  /* Wait for all CPU to have booted */
  while (_bootedCPUCount < sNumberOfCPUs)
  {
  }

  sAllCPUBooted = true;
}

void CPUAPInit(const uint8_t kCPUId)
{
  /* Register the existing IDT */
  __asm__ __volatile__("lidt %0"
                       :
                       : "m" (sIDTPtr.size), "m" (sIDTPtr.base));

  /* Create the internal structure */
  spCPUConfiguration[kCPUId] = KMalloc(sizeof(S_CPUConfig),
                                       KMALLOC_NO_FREE_POOL);
  /* Set the main CPU kernel stack */
  spCPUConfiguration[kCPUId]->kernelStackEnd =
    ((uintptr_t)&_KERNEL_STACKS_BASE) +
    (((kCPUId + 1) * KERNEL_STACK_SIZE) - 1);
  spCPUConfiguration[kCPUId]->cpuId = kCPUId;

  /* Setup the main CPU GDT and TSS */
  _SetupTSS(spCPUConfiguration[kCPUId]);
  _SetupGDT(spCPUConfiguration[kCPUId]);

  /* Validate architecture */
  _ValidateArchitecture();

  /* Initialize the CPU LAPIC */
  kspLAPICDriver->pInitApCPU();
  spCPUIds[kCPUId] = kspLAPICDriver->pGetLAPICId();
  if (kspLAPICTimerDriver != NULL)
  {
    kspLAPICTimerDriver->pInitApCPU(kCPUId);
  }

  KERNEL_INFO("Secondary CPU %d Started\n", _bootedCPUCount - 1);

  /* Wait release and schedule */
  while (sAllCPUBooted != true)
  {}
  SchedulerSchedule();

  /* Once the scheduler is started, we should never come back here. */
  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "CPU AP Init Returned",
        false,
        false);
}

const S_VirtualCPU* CPUGetVirtualCPU(const S_KernelThread* kpThread)
{
  return (S_VirtualCPU*)kpThread->pVCpu;
}

void CPUSetCount(const uint32_t kCPUCount)
{
  sNumberOfCPUs = MIN(kCPUCount, SOC_MAX_CPU_COUNT);
}

uint32_t CPUGetCount(void)
{
  return sNumberOfCPUs;
}

uintptr_t CPUGetStackEnd(const uint32_t kCPUId)
{
  return ((uintptr_t)&_KERNEL_STACKS_BASE) +
         (((kCPUId + 1) * KERNEL_STACK_SIZE) - 1);
}

size_t CPUGetStackSize(void)
{
  return KERNEL_STACK_SIZE;
}

void CPUHalt(void)
{
  __asm__ __volatile__("mfence\n\thlt\n\t"
                       :
                       :
                       : "memory");
}

void CPUSetPageDirectory(const uintptr_t kNewPgDir)
{
  __asm__ __volatile__("mov %%rax, %%cr3"::"a"(kNewPgDir));
}

void CPUInvalidateTLBEntry(const uintptr_t kVirtAddress)
{
  __asm__ __volatile__("invlpg (%0)": :"r"(kVirtAddress) : "memory");
}

void* CPUCreateVirtualCPU(S_KernelThread* pThread)
{
  S_VirtualCPU*       pVCpu;
  uintptr_t           stack;
  uint64_t            csVal;
  uint64_t            ssVal;
  uint64_t            rflagsVal;
  size_t              fxDataSize;
  S_InterruptContext* pIntContext;
  S_CPUState*         pCPUState;

  /* Allocate the new VCPU */
  pVCpu = KMallocUser(sizeof(S_VirtualCPU), pThread->pProcess->pHeap);
  if (pVCpu != NULL)
  {
    /* Setup the FX Data and align */
    fxDataSize = spCPUConfiguration[0]->cpuInfo.fxStateSize + ALIGN_64_BYTES;
    pVCpu->fxDataRegionNonAligned = (uintptr_t)KMallocUser(fxDataSize,
                                                 pThread->pProcess->pHeap);

    if (pVCpu->fxDataRegionNonAligned != (uintptr_t)NULL)
    {
      pVCpu->fxDataRegion = ALIGN_UP(pVCpu->fxDataRegionNonAligned,
                                     ALIGN_64_BYTES);

      if (pThread->type == THREAD_TYPE_KERNEL)
      {
        csVal     = KERNEL_CS_64;
        ssVal     = KERNEL_DS_64;
        rflagsVal = KERNEL_THREAD_INIT_RFLAGS;
        stack     = pThread->kernelStackEnd;
      }
      else
      {
        csVal     = USER_CS_64 | 0x3;
        ssVal     = USER_DS_64 | 0x3;
        rflagsVal = USER_THREAD_INIT_RFLAGS;
        stack     = pThread->stackEnd;
      }

      /* Setup the context */
      stack = ALIGN_DOWN(stack - ALIGN_8_BYTES, ALIGN_8_BYTES);
      pIntContext    = (S_InterruptContext*)(stack - sizeof(S_InterruptContext));
      pCPUState      = (S_CPUState*)((uintptr_t)pIntContext -
                                     sizeof(S_CPUState));
      pVCpu->context = (uintptr_t)pCPUState;

      /* Setup the interrupt context */
      pIntContext->intId     = 0;
      pIntContext->errorCode = 0;
      pIntContext->cs        = csVal;
      pIntContext->ss        = ssVal;
      pIntContext->rflags    = rflagsVal;
      pIntContext->rsp       = stack;

      /* Set the entry point */
      pIntContext->rip = (uintptr_t)pThread->pEntryPoint;
      pCPUState->rdi   = (uintptr_t)pThread->pArgs;

      /* Setup stack pointers */
      pCPUState->rsp = pVCpu->context;
      pCPUState->rbp = stack;

      /* Setup the CPU state */
      pCPUState->rax = 0;
      pCPUState->rbx = 1;
      pCPUState->rcx = 2;
      pCPUState->rdx = 3;
      pCPUState->rsi = 4;
      pCPUState->r8  = 5;
      pCPUState->r9  = 6;
      pCPUState->r10 = 7;
      pCPUState->r11 = 8;
      pCPUState->r12 = 9;
      pCPUState->r13 = 10;
      pCPUState->r14 = 11;
      pCPUState->r15 = 12;
      pCPUState->gs  = 0;
      pCPUState->fs  = 0;

      /* Initial chaining */
      pCPUState->savedContext = 0xFFFFFFFFFFFFFFFFULL;
    }
    else
    {
      KFreeUser(pVCpu, pThread->pProcess->pHeap);
      pVCpu = NULL;
    }
  }


  return pVCpu;
}

void CPUDestroyVirtualCPU(S_KernelThread* pThread)
{
  S_VirtualCPU*       pVCpu;

  pVCpu = pThread->pVCpu;

  KFreeUser((void*)pVCpu->fxDataRegionNonAligned, pThread->pProcess->pHeap);
  KFreeUser(pThread->pVCpu, pThread->pProcess->pHeap);
}

uint32_t CPUGetContextInterruptNumber(const S_KernelThread* kpThread)
{
  const S_VirtualCPU* pVCpu;
  S_InterruptContext* pIntContext;

  pVCpu = kpThread->pVCpu;

  pIntContext = (S_InterruptContext*)(pVCpu->context + sizeof(S_CPUState));

  return pIntContext->intId;
}

uintptr_t CPUGetContextIP(const S_KernelThread* kpThread)
{
  const S_VirtualCPU* pVCpu;
  S_InterruptContext* pIntContext;

  pVCpu = kpThread->pVCpu;

  pIntContext = (S_InterruptContext*)(pVCpu->context + sizeof(S_CPUState));

  return pIntContext->rip;
}

const S_CPUInterruptConfiguration* CPUGetInterruptConfig(void)
{
  return &ksInterruptConfig;
}

void CPUUpdateMemoryConfig(const S_KernelThread* kpThread)
{
  uintptr_t                cr3Value;
  uint8_t                  cpuId;
  S_ProcessMemoryMetadata* pMemProcInfo;

  cpuId = CPUGetId();

  /* The process contains the pointer to the page directory */
  pMemProcInfo = kpThread->pProcess->pMemoryData;

  /* Check if we need to change */
  __asm__ __volatile__ ("mov %%cr3, %%rax\n\t"
                        "mov %%rax, %0\n\t"
                        : "=m" (cr3Value)
                        : /* no input */
                        : "%rax");
  if (cr3Value != pMemProcInfo->PDPhysAddress)
  {
    CPUSetPageDirectory(pMemProcInfo->PDPhysAddress);
  }

  if (kpThread->type == THREAD_TYPE_USER)
  {
    /* Update the TSS */
    spCPUConfiguration[cpuId]->tss.rsp0 = ALIGN_DOWN(kpThread->kernelStackEnd -
                                                     ALIGN_8_BYTES,
                                                     ALIGN_8_BYTES);
  }

  /* Update the thread local storage */
  __asm__ __volatile__("wrmsr\n\t"
                       :
                       :"a"(kpThread->pUserThreadData),
                        "d"((uintptr_t)kpThread->pUserThreadData >> 32),
                        "c"(0xC0000100)
                       :);
}

void CPUSendIPI(const uint32_t kFlags, const S_IPIParameters* kpParams)
{
  uint8_t         i;
  uint8_t         destCpuId;
  uint8_t         srcCpuId;
  uint32_t        intState;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  srcCpuId = CPUGetId();

  /* Check if we should only send to one CPU */
  if ((kFlags & CPU_IPI_BROADCAST_TO_OTHER) == 0 &&
     (kFlags & CPU_IPI_BROADCAST_TO_ALL) == 0)
  {
    /* Get the CPU to send to */
    destCpuId = kFlags & CPU_IPI_SEND_TO_CPU_MASK;

    /* Check if in bounds */
    if (destCpuId < _bootedCPUCount)
    {
      FQueuePush(spIPIRequestQueue[destCpuId][srcCpuId], kpParams);
      kspLAPICDriver->pSendIPI(spCPUIds[destCpuId],
                               ksInterruptConfig.ipiInterruptLine);
    }
  }
  else if ((kFlags & CPU_IPI_BROADCAST_TO_ALL) == CPU_IPI_BROADCAST_TO_ALL)
  {
    /* Send to all */
    for (i = 0; i < _bootedCPUCount; ++i)
    {
      FQueuePush(spIPIRequestQueue[i][srcCpuId], kpParams);
      kspLAPICDriver->pSendIPI(spCPUIds[i], ksInterruptConfig.ipiInterruptLine);
    }
  }
  else if ((kFlags & CPU_IPI_BROADCAST_TO_OTHER) == CPU_IPI_BROADCAST_TO_OTHER)
  {
    /* Send to all excepted the caller */
    for (i = 0; i < _bootedCPUCount; ++i)
    {
      if (i != srcCpuId)
      {
        FQueuePush(spIPIRequestQueue[i][srcCpuId], kpParams);
        kspLAPICDriver->pSendIPI(spCPUIds[i],
                                 ksInterruptConfig.ipiInterruptLine);
      }
    }
  }

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

uint8_t CPUGetPhysicalAddressWidth(void)
{
  return spCPUConfiguration[0]->physAddressWidth;
}

uint8_t CPUGetVirtualAddressWidth(void)
{
  return spCPUConfiguration[0]->virtAddressWidth;
}

bool CPUGet1GBPageSupport(void)
{
  return spCPUConfiguration[0]->cpu1GBPageSupport;
}

void CPURegisterLAPICDriver(const S_LAPICDriver* kpLAPICDriver)
{
  kspLAPICDriver = kpLAPICDriver;
}

void CPURegisterLAPICTimerDriver(const S_LAPICTimerDriver* kpLAPICTimerDriver)
{
  kspLAPICTimerDriver = kpLAPICTimerDriver;
}

bool CPUValidateCPUMask(const S_CPUMask* kpMask)
{
  uint32_t cpuCount;
  uint32_t i;
  uint32_t j;
  uint32_t total;
  bool     valid;

  cpuCount = CPUGetCount();
  valid    = true;
  total    = 0;
  for (i = 0; i < CPU_MASK_TABLE_SIZE && valid == true; ++i)
  {
    for (j = 0; j < 64; ++j)
    {
      /* Check that we did not select a invalid CPU */
      if ((kpMask->mask[i] & (1ULL << j)) != 0)
      {
        ++total;
        if (j + i * 64 >= cpuCount)
        {
          valid = false;
          break;
        }
      }
    }
  }
  if (total == 0)
  {
    valid = false;
  }
  return valid;
}

static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode)
{
  size_t* pHandle;

  (void)pDriverData;
  (void)mode;
  if (flags == O_RDONLY && *kpPath == 0)
  {
    pHandle = KMallocUser(sizeof(size_t), NULL);
    if (pHandle != NULL)
    {
      *pHandle = 0;
    }
    else
    {
      pHandle = (void*)-1;
    }
  }
  else
  {
    pHandle = (void*)-1;
  }

  return pHandle;
}

static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle)
{
  int32_t retCode;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    KFreeUser(pFileHandle, NULL);
    retCode = 0;
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count)
{
  int32_t                 retCode;
  size_t*                 pOffset;
  const S_CPUInformation* kpInfo;
  char*                   pInfoBuffer;
  size_t                  infoSize;
  size_t                  currentOffset;
  size_t                  bufferOffset;
  size_t                  startPos;
  size_t                  copyStart;
  size_t                  copySize;
  size_t                  copied;
  uint32_t                i;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    pInfoBuffer = KMallocUser(CPUINFO_BUFFER_SIZE, NULL);
    if (pInfoBuffer != NULL)
    {
      pOffset = (size_t*)pFileHandle;
      /* Generate CPUID data for each processor */
      currentOffset = 0;
      bufferOffset  = 0;
      copied        = 0;
      retCode       = 0;
      startPos      = *pOffset;
      for (i = 0; i < sNumberOfCPUs; ++i)
      {
        kpInfo   = &spCPUConfiguration[i]->cpuInfo;
        infoSize = CPUINFO_BUFFER_SIZE;
        _GenerateCPUInfoString(kpInfo, pInfoBuffer, &infoSize);

        /* Skip if we already read that part */
        if (*pOffset > currentOffset + infoSize)
        {
          currentOffset += infoSize;
          continue;
        }

        /* Get the start position */
        copyStart = startPos - currentOffset;
        copySize = MIN(count, infoSize - copyStart);

        /* Copy */
        memcpy(pBuffer + bufferOffset, pInfoBuffer + copyStart, copySize);

        /* Update pointers */
        count         -= copySize;
        bufferOffset  += copySize;
        currentOffset += infoSize;
        startPos      = currentOffset;
        copied        += copySize;
      }

      *pOffset += copied;
      retCode  += copied;

      KFreeUser(pInfoBuffer, NULL);
    }
    else
    {
      retCode = -1;
    }
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static E_Return _CreateProcFSEntry(void)
{
  E_Return retCode;

  /* Create the entry */
  retCode = ProcFSCreateEntry(CPUS_PROCFS_DIR_PATH,
                              0,
                              NULL,
                              &sProcFSOps,
                              NULL,
                              &spProcFSEntry);
  return retCode;
}

static void _GenerateCPUInfoString(const S_CPUInformation* kpInfo,
                                   char*                   pInfoBuffer,
                                   size_t*                 pInfoSize)
{
  size_t          maxSize;
  size_t          offset;
  S_CPUCacheInfo* pCacheInfo;
  S_CPUTLBInfo*   pTlbInfo;
  char*           cacheStr;
  char*           tlbStr;

  maxSize = *pInfoSize;

  offset = snprintf(pInfoBuffer, maxSize, "processor: %d\n", kpInfo->id);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "vendor_id: %s\n",
                    kpInfo->pVendor);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "cpu family: %d\n",
                    kpInfo->family);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "model: %d\n",
                    kpInfo->model);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "model name: %s\n",
                    kpInfo->pName);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "stepping: %d\n",
                    kpInfo->stepping);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "microcode: %d\n",
                    kpInfo->microcode);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "cpuHz: %d\n",
                    kpInfo->frequencyHz);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }

  pCacheInfo = kpInfo->pCaches;
  while (pCacheInfo != NULL)
  {
    switch(pCacheInfo->type)
    {
      case CACHE_DATA:
        cacheStr = "Data";
        break;
      case CACHE_INSTRUCTION:
        cacheStr = "Instruction";
        break;
      case CACHE_UNIFIED:
        cacheStr = "Unified";
        break;
      default:
        cacheStr = "Unknown";
    }
    offset = snprintf(pInfoBuffer,
                      maxSize,
                      "L%d %s cache:\n"
                      "\tSize: %dKB\n"
                      "\tWays: %d\n"
                      "\tSets: %d\n"
                      "\tLine size: %dB\n",
                      pCacheInfo->level + 1,
                      cacheStr,
                      pCacheInfo->size / 1024,
                      pCacheInfo->ways,
                      pCacheInfo->sets,
                      pCacheInfo->lineSize
                    );
    pInfoBuffer += offset;
    maxSize -= offset;
    if (maxSize == 0)
    {
      return;
    }
    pCacheInfo = pCacheInfo->pNext;
  }

  pTlbInfo = kpInfo->pTLBs;
  while (pTlbInfo != NULL)
  {
    switch(pTlbInfo->type)
    {
      case TLB_DATA:
        cacheStr = "Data";
        break;
      case TLB_INSTRUCTIONS:
        cacheStr = "Instruction";
        break;
      case TLB_UNIFIED:
        cacheStr = "Unified";
        break;
      default:
        cacheStr = "Unknown";
    }
    switch(pTlbInfo->size)
    {
      case TLB_4K:
        tlbStr = "4K";
        break;
      case TLB_2MB_4MB:
        tlbStr = "2MB/4MB";
        break;
      case TLB_1G:
        tlbStr = "1GB";
        break;
      default:
        tlbStr = "Unknown";
    }
    offset = snprintf(pInfoBuffer,
                      maxSize,
                      "TLB %d %s %s:\n"
                      "\tWays: %d\n"
                      "\tSets: %d\n"
                      "\tEntries: %dB\n",
                      pTlbInfo->level + 1,
                      cacheStr,
                      tlbStr,
                      pTlbInfo->ways,
                      pTlbInfo->sets,
                      pTlbInfo->nbEntries
                    );
    pInfoBuffer += offset;
    maxSize -= offset;
    if (maxSize == 0)
    {
      return;
    }
    pTlbInfo = pTlbInfo->pNext;
  }

  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "physical id: %d\n",
                    kpInfo->physicalId);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "siblings: %d\n",
                    kpInfo->siblings);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "core id: %d\n",
                    kpInfo->coreId);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "cpu cores: %d\n",
                    kpInfo->cpuCores);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "apicid: %d\n",
                    kpInfo->apicId);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "initial apicid: %d\n",
                    kpInfo->initialApicId);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "fpu: %s\n",
                    kpInfo->fpu ? "yes": "no");
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "cpuid level: 0x%X\n",
                    kpInfo->cpuIdLevel);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "wp: %s\n",
                    kpInfo->wp ? "yes": "no");
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "flags: ");
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = CPUIDGetFlagsString(pInfoBuffer, maxSize, &kpInfo->flags);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "\n");
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "bogomips: %d\n",
                    kpInfo->bogoMips);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "clflush size: %d\n",
                    kpInfo->clFlushSize);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
    return;
  }
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "address sizes: %d bits physical, %d bits virtual\n",
                    kpInfo->physAddressWidth, kpInfo->virtAddressWidth);
  pInfoBuffer += offset;
  maxSize -= offset;
  if (maxSize == 0)
  {
      return;
}
  offset = snprintf(pInfoBuffer,
                    maxSize,
                    "\n");
  pInfoBuffer += offset;
  maxSize -= offset;
  /* Update size */
  *pInfoSize = *pInfoSize - maxSize;
}

void CPUGetMaskString(const S_CPUMask* kpMask, S_CPUMaskString maskString)
{
  uint32_t i;
  size_t   offset;
  char*    pBuffer;

  offset  = 0;
  pBuffer = maskString;
  for (i = 0; i < CPU_MASK_TABLE_SIZE; ++i)
  {
    snprintf(pBuffer + offset,
             17,
             "%016llX",
             kpMask->mask[i]);
    offset += 16;
  }
  *((char*)(pBuffer + offset)) = '\0';
}

E_Return CPUCreateTLS(S_KernelThread* pThread)
{
  size_t    align;
  size_t    size;
  E_Return  error;
  E_Return  intError;
  uintptr_t tlsPhys;
  size_t    userDataAlign;
  void*     pTmpData;

  if(pThread->type != THREAD_TYPE_KERNEL)
  {
    /* Compute the memory size with alignement */
    align = MAX(pThread->pProcess->mainTlsAlign, __alignof__(S_UserThread));
    /* We do not support more than a page alignement */
    if(align <= KERNEL_PAGE_SIZE)
    {

      userDataAlign = ALIGN_UP(pThread->pProcess->mainTlsSize, align);

      size = ALIGN_UP(userDataAlign + sizeof(S_UserThread), KERNEL_PAGE_SIZE);

      pThread->pUserThreadData = MemoryUserAllocate(size,
                                         pThread->pProcess->mainTlsMappingFlags,
                                         pThread->pProcess,
                                         &error);
      if(error == NO_ERROR)
      {
        tlsPhys = MemoryMgrGetPhysAddr((uintptr_t)pThread->pUserThreadData,
                                       pThread->pProcess,
                                       NULL);
        CPU_ASSERT(tlsPhys != MEMMGR_PHYS_ADDR_ERROR,
                   "Failed to get mapped physical address",
                   ERR_INVALID_VALUE);

        pTmpData = MemoryKernelMap((void*)tlsPhys,
                                   size,
                                   MEMMGR_MAP_KERNEL | MEMMGR_MAP_RW,
                                   &error);
        if(error == NO_ERROR)
        {
          /*
           * Copy the main TLS, the User linker defines that the data for the TLS
           * comes before the bss for the TLS.
           */
          memcpy((uint8_t*)pTmpData + userDataAlign -
                           pThread->pProcess->mainTlsSize,
                pThread->pProcess->pMainTlsData,
                pThread->pProcess->mainTlsInitDataSize);
          /* Zeroize the TLS */
          memset((uint8_t*)pTmpData +
                userDataAlign -
                pThread->pProcess->mainTlsSize +
                pThread->pProcess->mainTlsInitDataSize,
                0,
                userDataAlign - pThread->pProcess->mainTlsInitDataSize);


          intError = MemoryKernelUnmap(pTmpData, size);
          CPU_ASSERT(intError == NO_ERROR,
                     "Failed to unmap mapped memory",
                     intError);

          /* Setup the user data pointer */
          pThread->pUserThreadData = (void*)((uintptr_t)pThread->pUserThreadData +
                                    userDataAlign);
        }
        else
        {
          intError = MemoryUserFree(pThread->pUserThreadData,
                                    size,
                                    pThread->pProcess);
          CPU_ASSERT(intError == NO_ERROR,
                      "Failed to free allocated memory",
                      intError);
        }
      }
    }
    else
    {
      error = ERR_NOT_SUPPORTED;
    }
  }
  else
  {
    /* Kernel thread do not have thread local storage */
    pThread->pUserThreadData = KMallocUser(sizeof(S_UserThread),
                                           pThread->pProcess->pHeap);
    if(pThread->pUserThreadData == NULL)
    {
      error = ERR_NO_MEMORY;
    }
    else
    {
      error = NO_ERROR;
    }
  }

  return error;
}

void CPUDestroyTLS(S_KernelThread* pThread)
{
  size_t    align;
  size_t    size;
  uintptr_t tls;
  E_Return  error;

  if(pThread->type != THREAD_TYPE_KERNEL)
  {
    /* Compute the memory size with alignement */
    align = MAX(pThread->pProcess->mainTlsAlign, __alignof__(S_UserThread));
    /* We do not support more than a page alignement */
    CPU_ASSERT(align <= KERNEL_PAGE_SIZE,
               "Invalid TLS alignement",
               ERR_NOT_SUPPORTED);

    size = ALIGN_UP(pThread->pProcess->mainTlsSize, align) +
                    sizeof(S_UserThread);
    /* Align to page size */
    size = ALIGN_UP(size, KERNEL_PAGE_SIZE);

    /* Get the TLS */
    tls = (uintptr_t)pThread->pUserThreadData -
          ALIGN_UP(pThread->pProcess->mainTlsSize, align);

    error = MemoryUserFree((void*)tls, size, pThread->pProcess);
    CPU_ASSERT(error == NO_ERROR,
               "Failed to release thread local storage.",
               error);
  }
  else
  {
    /* Kernel thread do not have thread local storage */
    KFreeUser(pThread->pUserThreadData, pThread->pProcess->pHeap);
  }
}

/* Stack protection support */
#ifdef _STACK_PROT
#define STACK_CHK_GUARD 0x595e9fbd94fda766ULL
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
__attribute__((noreturn)) void __stack_chk_fail(void);
__attribute__((noreturn)) void __stack_chk_fail(void)
{
  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "Stack smashing detected",
        false,
        false);
  while (true)
  {
    CPUHalt();
  }
}
#endif
/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86CPUDriver);

/************************************ EOF *************************************/