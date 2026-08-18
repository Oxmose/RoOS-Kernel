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
#include <stddef.h>
#include <Memory.h>
#include <FastQueue.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <DriverManager.h>
#include <InterruptHandlers.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* None TODO */

/* Header file */
#include <X64Cpu.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "CPU_X64"

/** @brief Compatible property name in FDT */
#define COMPATIBLE_PROP_NAME "compatible"
/** @brief Status property name in FDT */
#define STATUS_PROP_NAME "status"

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
#define DOUBLE_FAULT_STACK_SIZE 512

/** @brief IPI send flag CPU mask */
#define CPU_IPI_SEND_TO_CPU_MASK (CPU_IPI_SEND_TO(0xFFFFFFFF));

/** @brief Size in number of elements of the IPI queues */
#define IPI_QUEUE_SIZE 10

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
    PANIC(ERROR, MODULE_NAME, MSG, false);        \
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
 * @brief Initializes the number of CPU based on the FDT.
 *
 * @param[in] kpFDTNode The current FDT node to walk.
 *
 * @details Initializes the number of CPU based on the FDT.
 */
static void _WalkCPUCount(const S_FDTNode* kpFDTNode);

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

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel stacks base symbol. */
extern int8_t _KERNEL_STACKS_BASE;
/** @brief Stores the number of CPU that booted. */
extern volatile uint32_t _bootedCPUCount;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief CPU configuration table. */
static S_CPUConfig* sCPUConfiguration;

/**@brief Stores the number of detected CPUs */
static uint32_t sNumberOfCPUs;

/** @brief Stores the number of attached CPUs */
static uint32_t sAttachedCpus;

/** @brief CPU IDT space in memory. */
static S_CPUIDTEntry sIDT[IDT_ENTRY_COUNT] __attribute__((aligned(8)));

/** @brief Kernel IDT structure */
static S_IDTPtr sIDTPtr __attribute__((aligned(8)));

/** @brief Stores the Double Fault Exception Special Stack */
static uint8_t sDFStack[DOUBLE_FAULT_STACK_SIZE] __attribute__((aligned(8)));

/** @brief Queues used to communicate with IPIs. */
static S_FastQueue*** spIPIRequestQueue;

/** @brief Stores the LAPIC driver instance */
static const S_LAPICDriver* kspLAPICDriver = NULL;

/** @brief Stores the LAPIC timer driver instance */
static const S_LAPICTimerDriver* kspLAPICTimerDriver = NULL;

/** @brief Stores the CPUs LAPIS identifiers. */
static uint32_t* spCPUIds;

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
                              DOUBLE_FAULT_STACK_SIZE -
                              ALIGN_16_BYTES;
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

  CPU_ASSERT(sAttachedCpus < sNumberOfCPUs,
             "Exceeded CPU limit.",
             ERR_EXCEEDED_LIMIT);

  ++sAttachedCpus;

  return NO_ERROR;
}

static void _WalkCPUCount(const S_FDTNode* kpFDTNode)
{
  const char* kpCompatible;
  const char* kpStatus;
  size_t      propLen;

  if (kpFDTNode != NULL)
  {
    /* Manage disabled nodes */
    kpStatus = FDTGetProp(kpFDTNode, STATUS_PROP_NAME, &propLen);
    if (kpStatus == NULL || (propLen == 5 && strcmp(kpStatus, "okay") == 0))
    {
      /* Get the node compatible */
      kpCompatible = FDTGetProp(kpFDTNode, COMPATIBLE_PROP_NAME, &propLen);
      if (kpCompatible != NULL && propLen > 0)
      {
        if (strcmp(sX86CPUDriver.pCompatible, kpCompatible) == 0)
        {
          ++sNumberOfCPUs;
        }
      }
    }

    /* Got to next nodes */
    _WalkCPUCount(FDTGetChild(kpFDTNode));
    _WalkCPUCount(FDTGetNextNode(kpFDTNode));
  }
}

static void _ValidateArchitecture(void)
{
  uint32_t          cr0Reg;
  uint32_t          cpuId;
  S_CPUInformation* pNewCpuInfo;

  cpuId = CPUGetId();

  /* Link */
  pNewCpuInfo = &sCPUConfiguration[cpuId].cpuInfo;

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

  sCPUConfiguration[cpuId].cpu1GBPageSupport = pNewCpuInfo->flags.page1gb;
  sCPUConfiguration[cpuId].physAddressWidth = pNewCpuInfo->physAddressWidth;
  sCPUConfiguration[cpuId].virtAddressWidth = pNewCpuInfo->virtAddressWidth;

  if (pNewCpuInfo->id != 0)
  {
    /* Validate uniformity*/
    CPU_ASSERT(
      (pNewCpuInfo->flags.page1gb == sCPUConfiguration[0].cpu1GBPageSupport &&
       pNewCpuInfo->physAddressWidth == sCPUConfiguration[0].physAddressWidth &&
       pNewCpuInfo->virtAddressWidth == sCPUConfiguration[0].virtAddressWidth),
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
          PANIC(ERR_EXCEEDED_LIMIT, MODULE_NAME, "Unknown IPI function", false);
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
                              ALIGN_ADDRESS,
                              KMALLOC_NO_FREE_POOL);
  for (i = 0; i < sNumberOfCPUs; ++i)
  {
    spIPIRequestQueue[i] = KMalloc(sizeof(S_FastQueue*) * sNumberOfCPUs,
                                   ALIGN_ADDRESS,
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
  uint32_t         i;
  const S_FDTNode* kpFDTRoot;

  sNumberOfCPUs = 0;
  sAttachedCpus = 0;

  /* Setup the shared IDT */
  _SetupIDT();

  /* Detect the number of CPUs and allocate the contexts */
  kpFDTRoot = FDTGetRoot();
  _WalkCPUCount(kpFDTRoot);
  sCPUConfiguration = KMalloc(sizeof(S_CPUConfig) * sNumberOfCPUs,
                              ALIGN_16_BYTES,
                              KMALLOC_NO_FREE_POOL);
  spCPUIds = KMalloc(sizeof(uint32_t) * sNumberOfCPUs,
                      ALIGN_4_BYTES,
                      KMALLOC_NO_FREE_POOL);
  /* Set the main CPU kernel stack */
  for (i = 0; i < sNumberOfCPUs; ++i)
  {
    sCPUConfiguration[i].kernelStackEnd = ((uintptr_t)&_KERNEL_STACKS_BASE) +
                                           (i + 1) * KERNEL_STACK_SIZE - 1;
  }

  /* Setup the main CPU GDT and TSS */
  _SetupTSS(&sCPUConfiguration[0]);
  _SetupGDT(&sCPUConfiguration[0]);

  /* Validate architecture */
  _ValidateArchitecture();

  /* Initialize IPI */
  _InitializeIPI();
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

  /* Check if we need to enable more CPUs */
  kpLapicNode = kspLAPICDriver->pGetLAPICList();
  while (kpLapicNode != NULL && _bootedCPUCount < sNumberOfCPUs)
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

  /* Last check */
  CPU_ASSERT(sAttachedCpus == _bootedCPUCount,
             "Attached CPUs count does not match booted CPU count.",
              ERR_INVALID_VALUE);
}

void CPUAPInit(const uint8_t kCPUId)
{
  /* Register the existing IDT */
  __asm__ __volatile__("lidt %0"
                       :
                       : "m" (sIDTPtr.size), "m" (sIDTPtr.base));

  /* Setup the main CPU GDT and TSS */
  _SetupTSS(&sCPUConfiguration[kCPUId]);
  _SetupGDT(&sCPUConfiguration[kCPUId]);

  /* Validate architecture */
  _ValidateArchitecture();

  /* Initialize the CPU LAPIC */
  kspLAPICDriver->pInitApCPU();
  if (kspLAPICTimerDriver != NULL)
  {
    kspLAPICTimerDriver->pInitApCPU(kCPUId);
  }
  spCPUIds[kCPUId] = kspLAPICDriver->pGetLAPICId();

  /* Wait release and schedule */
  while (SchedulerIsInitialized() != true)
  {}
  SchedulerSchedule();

  /* Once the scheduler is started, we should never come back here. */
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "CPU AP Init Returned", false);
}

const S_VirtualCPU* CPUGetVirtualCPU(const S_KernelThread* kpThread)
{
  return (S_VirtualCPU*)kpThread->pVCpu;
}

uint32_t CPUGetCount(void)
{
  return sNumberOfCPUs;
}

uintptr_t CPUGetStackEnd(const uint32_t kCPUId)
{
  return sCPUConfiguration[kCPUId].kernelStackEnd;
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
  S_VirtualCPU* pVCpu;
  S_FXData*     pFxData;
  uintptr_t     stack;
  uint64_t      csVal;
  uint64_t      dsVal;
  uint64_t      rflagsVal;

  if (pThread->type == THREAD_TYPE_KERNEL)
  {
    csVal     = KERNEL_CS_64;
    dsVal     = KERNEL_DS_64;
    rflagsVal = KERNEL_THREAD_INIT_RFLAGS;
    stack     = pThread->kernelStackEnd;
  }
  else
  {
    csVal     = USER_CS_64 | 0x3;
    dsVal     = USER_DS_64 | 0x3;
    rflagsVal = USER_THREAD_INIT_RFLAGS;
    stack     = pThread->stackEnd;
  }

  /* Allocate the new VCPU */
  pVCpu = KMalloc(sizeof(S_VirtualCPU), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
  memset(pVCpu, 0, sizeof(S_VirtualCPU));

  /* Setup the interrupt context */
  pVCpu->intContext.intId     = 0;
  pVCpu->intContext.errorCode = 0;
  pVCpu->intContext.cs        = csVal;
  pVCpu->intContext.rflags    = rflagsVal;

  /* Set the entry point */
  pVCpu->intContext.rip = (uintptr_t)pThread->pEntryPoint;
  pVCpu->cpuState.rdi   = (uintptr_t)pThread->pArgs;

  /* Setup stack pointers */
  pVCpu->cpuState.rsp   = ALIGN_DOWN(stack - ALIGN_8_BYTES, ALIGN_8_BYTES);
  pVCpu->cpuState.rbp   = pVCpu->cpuState.rsp;

  /* Setup the CPU state */
  pVCpu->cpuState.rsi = 0;
  pVCpu->cpuState.rdx = 0;
  pVCpu->cpuState.rcx = 0;
  pVCpu->cpuState.rbx = 0;
  pVCpu->cpuState.rax = 0;
  pVCpu->cpuState.r8  = 0;
  pVCpu->cpuState.r9  = 0;
  pVCpu->cpuState.r10 = 0;
  pVCpu->cpuState.r11 = 0;
  pVCpu->cpuState.r12 = 0;
  pVCpu->cpuState.r13 = 0;
  pVCpu->cpuState.r14 = 0;
  pVCpu->cpuState.r15 = 0;
  pVCpu->cpuState.ss  = dsVal;
  pVCpu->cpuState.gs  = dsVal;
  pVCpu->cpuState.fs  = dsVal;
  pVCpu->cpuState.es  = dsVal;
  pVCpu->cpuState.ds  = dsVal;

  /* Setup the FPU */
  pFxData = (S_FXData*)(((uintptr_t)pVCpu->fxData + 0xF) & 0xFFFFFFFFFFFFFFF0);
  pFxData->mxcsr = MXCSR_PRECISION_EXC_MASK;

  return pVCpu;
}

void CPUDestroyVirtualCPU(S_KernelThread* pThread)
{
  KFree(pThread->pVCpu);
}

uint32_t CPUGetContextInterruptNumber(const S_KernelThread* kpThread)
{
  const S_VirtualCPU* pVCpu;

  pVCpu = kpThread->pVCpu;

  return pVCpu->intContext.intId;
}

uintptr_t CPUGetContextIP(const S_KernelThread* kpThread)
{
  const S_VirtualCPU* pVCpu;

  pVCpu = kpThread->pVCpu;

  return pVCpu->intContext.rip;
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
    sCPUConfiguration[cpuId].tss.rsp0 = ALIGN_DOWN(kpThread->kernelStackEnd -
                                                   ALIGN_16_BYTES,
                                                   ALIGN_16_BYTES);
  }

  /* Update the thread local storage */
  /* TODO */
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
  return sCPUConfiguration->physAddressWidth;
}

uint8_t CPUGetVirtualAddressWidth(void)
{
  return sCPUConfiguration->virtAddressWidth;
}

bool CPUGet1GBPageSupport(void)
{
  return sCPUConfiguration->cpu1GBPageSupport;
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

/* Stack protection support */
#ifdef _STACK_PROT
#define STACK_CHK_GUARD 0x595e9fbd94fda766ULL
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
__attribute__((noreturn)) void __stack_chk_fail(void);
__attribute__((noreturn)) void __stack_chk_fail(void)
{
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "Stack smashing detected", false);
  while (true)
  {
    CPUHalt();
  }
}
#endif
/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86CPUDriver);

/************************************ EOF *************************************/