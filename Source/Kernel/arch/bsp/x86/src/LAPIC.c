/*******************************************************************************
 * @file LAPIC.c
 *
 * @see LAPIC.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/06/2024
 *
 * @version 2.0
 *
 * @brief Local APIC (Advanced programmable interrupt controller) driver.
 *
 * @details Local APIC (Advanced programmable interrupt controller) driver.
 * Manages x86 IRQs from the IO-APIC. IPI (inter processor interrupt) are also
 * possible thanks to the driver.
 * Manages  the X86 LAPIC timer using the LAPIC driver. The LAPIC timer can be
 * used for main timer or auxiliary timer.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <MMIO.h>
#include <ACPI.h>
#include <Panic.h>
#include <X64Cpu.h>
#include <stdlib.h>
#include <stdint.h>
#include <Memory.h>
#include <ProcFS.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelQueue.h>
#include <KernelError.h>
#include <TimerManager.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <LAPIC.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for acpi handle */
#define LAPIC_FDT_ACPI_NODE_PROP "acpi-node"

/** @brief FDT property for interrupt  */
#define LAPICT_FDT_INT_PROP "interrupts"
/** @brief FDT property for frequency */
#define LAPICT_FDT_SELFREQ_PROP "freq"
/** @brief FDT property for frequency divider */
#define LAPICT_FDT_DIVIDER_PROP "bus-freq-divider"
/** @brief FDT property for base-timer */
#define LAPICT_TIMER_FDT_BASE_TIMER_PROP "base-timer"
/** @brief FDT property for LAPIC */
#define LAPICT_FDT_LAPIC_NODE_PROP "lapic-node"

/** @brief PROCFS directory path */
#define PROCFS_DIR_PATH "lapic"
/** @brief PROCFS entry path */
#define PROCFS_ENTRY_PATH "info"
/** @brief PROCFS directory path for timers */
#define PROCFS_TIMER_DIR_PATH "timers"
/** @brief PROCFS string length */
#define PROCFS_STRING_LENGTH 256

/** @brief LAPIC local vector table timer register's offset. */
#define LAPIC_TIMER 0x0320
/** @brief LAPIC timer initial count register's offset. */
#define LAPIC_TICR 0x0380
/** @brief LAPIC timer current count register's offset. */
#define LAPIC_TCCR 0x0390
/** @brief LAPIC timer divide configuration register's offset. */
#define LAPIC_TDCR 0x03E0
/** @brief LAPIC local vector table timer register's offset. */

/** @brief LAPIC Timer divider value : 1. */
#define LAPICT_DIVIDER_1 0xB
/** @brief LAPIC Timer divider value : 2. */
#define LAPICT_DIVIDER_2 0x0
/** @brief LAPIC Timer divider value : 4. */
#define LAPICT_DIVIDER_4 0x1
/** @brief LAPIC Timer divider value : 8. */
#define LAPICT_DIVIDER_8 0x2
/** @brief LAPIC Timer divider value : 16. */
#define LAPICT_DIVIDER_16 0x3
/** @brief LAPIC Timer divider value : 32. */
#define LAPICT_DIVIDER_32 0x8
/** @brief LAPIC Timer divider value : 64. */
#define LAPICT_DIVIDER_64 0x9
/** @brief LAPIC Timer divider value : 128. */
#define LAPICT_DIVIDER_128 0xA

/** @brief LAPIC Timer mode flag: periodic. */
#define LAPIC_TIMER_MODE_PERIODIC 0x20000

/** @brief LAPIC Timer vector interrupt mask. */
#define LAPIC_LVT_INT_MASKED 0x10000

/** @brief Calibration time in NS: 10ms */
#define LAPICT_CALIBRATION_DELAY 1000000

/** @brief LAPIC ID register's offset. */
#define LAPIC_ID 0x0020
/** @brief LAPIC version register's offset. */
#define LAPIC_VER 0x0030
/** @brief LAPIC trask priority register's offset. */
#define LAPIC_TPR 0x0080
/** @brief LAPIC arbitration policy register's offset. */
#define LAPIC_APR 0x0090
/** @brief LAPIC processor priority register's offset. */
#define LAPIC_PPR 0x00A0
/** @brief LAPIC EOI register's offset. */
#define LAPIC_EOI 0x00B0
/** @brief LAPIC remote read register's offset. */
#define LAPIC_RRD 0x00C0
/** @brief LAPIC logical destination register's offset. */
#define LAPIC_LDR 0x00D0
/** @brief LAPIC destination format register's offset. */
#define LAPIC_DFR 0x00E0
/** @brief LAPIC Spurious interrupt vector register's offset. */
#define LAPIC_SVR 0x00F0
/** @brief LAPIC in service register's offset. */
#define LAPIC_ISR 0x0100
/** @brief LAPIC trigger mode register's offset. */
#define LAPIC_TMR 0x0180
/** @brief LAPIC interrupt request register's offset. */
#define LAPIC_IRR 0x0200
/** @brief LAPIC error status register's offset. */
#define LAPIC_ESR 0x0280
/** @brief LAPIC interrupt command (low) register's offset. */
#define LAPIC_ICRLO 0x0300
/** @brief LAPIC interrupt command (high) register's offset. */
#define LAPIC_ICRHI 0x0310
/** @brief LAPIC local vector table timer register's offset. */
#define LAPIC_TIMER 0x0320
/** @brief LAPIC local vector table thermal sensor register's offset. */
#define LAPIC_THERMAL 0x0330
/** @brief LAPIC local vector table PMC register's offset. */
#define LAPIC_PERF 0x0340
/** @brief LAPIC local vector table lint0 register's offset. */
#define LAPIC_LINT0 0x0350
/** @brief LAPIC local vector table lint1 register's offset. */
#define LAPIC_LINT1 0x0360
/** @brief LAPIC local vector table error register's offset. */
#define LAPIC_ERROR 0x0370

/** @brief LAPIC delivery mode fixed. */
#define ICR_FIXED 0x00000000
/** @brief LAPIC delivery mode lowest priority. */
#define ICR_LOWEST 0x00000100
/** @brief LAPIC delivery mode SMI. */
#define ICR_SMI 0x00000200
/** @brief LAPIC delivery mode NMI. */
#define ICR_NMI 0x00000400
/** @brief LAPIC delivery mode init IPI. */
#define ICR_INIT 0x00000500
/** @brief LAPIC delivery mode startup IPI. */
#define ICR_STARTUP 0x00000600
/** @brief LAPIC delivery mode external. */
#define ICR_EXTERNAL 0x00000700

/** @brief LAPIC destination mode physical. */
#define ICR_PHYSICAL 0x00000000
/** @brief LAPIC destination mode logical. */
#define ICR_LOGICAL 0x00000800

/** @brief LAPIC Delivery status idle. */
#define ICR_IDLE 0x00000000
/** @brief LAPIC Delivery status pending. */
#define ICR_SEND_PENDING 0x00001000

/** @brief LAPIC Level deassert enable flag. */
#define ICR_DEASSERT 0x00000000
/** @brief LAPIC Level deassert disable flag. */
#define ICR_ASSERT 0x00004000

/** @brief LAPIC trigger mode edge. */
#define ICR_EDGE 0x00000000
/** @brief LAPIC trigger mode level. */
#define ICR_LEVEL 0x00008000

/** @brief LAPIC destination shorthand none. */
#define ICR_NO_SHORTHAND 0x00000000
/** @brief LAPIC destination shorthand self only. */
#define ICR_SELF 0x00040000
/** @brief LAPIC destination shorthand all and self. */
#define ICR_ALL_INCLUDING_SELF 0x00080000
/** @brief LAPIC destination shorthand all but self. */
#define ICR_ALL_EXCLUDING_SELF 0x000C0000

/** @brief LAPIC destination flag shift. */
#define ICR_DESTINATION_SHIFT 24

/** @brief Delay between INIT and STARTUP IPI in NS (10ms) */
#define LAPIC_CPU_INIT_DELAY_NS 10000000

/** @brief Delay between two STARTUP IPI in NS (200us) */
#define LAPIC_CPU_STARTUP_DELAY_NS 200000

/** @brief Delay between the STARTUP IPI and waiting for the core to be booted. */
#define LAPIC_CPU_STARTUP_WAIT_DELAY_NS 10000000

/** @brief Defines the LAPIC memory size */
#define LAPIC_MEMORY_SIZE 0x3F4

/** @brief Defines the number of times the CPU startup IPI is retried */
#define LAPIC_CPU_STARTUP_RETRIES 5

/** @brief Current module name */
#define MODULE_NAME "X86 LAPIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 LAPIC driver controller. */
typedef struct S_LAPICControler
{
  /** @brief LAPIC base physical address */
  uintptr_t baseAddr;
  /** @brief LAPIC memory mapping size */
  size_t mappingSize;
  /** @brief CPU's spurious interrupt line */
  uint32_t spuriousIntLine;
  /** @brief List of present LAPICs from the ACPI. */
  const S_LAPICNode* pLAPICList;
} S_LAPICControler;

/** @brief x86 LAPIC Timer driver controller. */
typedef struct
{
  /** @brief LAPIC Timer interrupt number. */
  uint8_t interruptNumber;
  /** @brief LAPIC Timer internal frequency. One per CPU */
  uint64_t* pInternalFrequency;
  /** @brief Selected interrupt frequency. */
  uint64_t selectedFrequency;
  /** @brief Bus frequency divider. */
  uint32_t divider;
  /** @brief Keeps track on the LAPIC Timer enabled state. One per CPU */
  uint32_t* pDisabledNesting;
  /** @brief LAPIC base addresss */
  uintptr_t lapicBaseAddress;
  /** @brief Time base driver */
  const S_KernelTimer* kpBaseTimer;
} S_LAPICTimerControler;

/** @brief Structure for handling LAPIC timer entries in the proc filesystem. */
typedef struct
{
  /** @brief CPU identifier */
  uint32_t cpuId;
  /** @brief Current offset in the entry */
  size_t offset;
  /** @brief Type of the entry */
  uint32_t type;
} S_LAPICTimerProcFSHandle;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the LAPIC to ensure correctness of execution.
 *
 * @details Assert macro used by the LAPIC to ensure correctness of execution.
 * Due to the critical nature of the LAPIC, any error generates a kernel
 * panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define LAPIC_ASSERT(COND, MSG, ERROR) {                \
  if ((COND) == false)                                  \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);       \
  }                                                     \
}

/**
 * @brief Sets the value for a STARTUP IPI of the startup code.
 *
 * @details Sets the value for a STARTUP IPI of the startup code. The startup
 * IPI send the startup code address on a 4k page boundary. Thus we only send
 * the page ID.
 */
#define LAPIC_STARTUP_ADDR(ADDR) ((((uintptr_t)(ADDR)) >> 12) & 0xFF)

/** @brief Cast a pointer to a LAPIC timer driver controller */
#define GET_CONTROLER(PTR) ((S_LAPICTimerControler*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the LAPIC driver to the system.
 *
 * @details Attaches the LAPIC driver to the system. This function will use
 * the FDT to initialize the LAPIC hardware and retreive the LAPIC parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Sets END OF INTERRUPT for the current CPU Local APIC.
 *
 * @details Sets END OF INTERRUPT for the current CPU Local APIC.
 *
 * @param[in] kInterruptLine The interrupt line for which the EOI should be set.
 */
static void _SetIrqEOI(const uint32_t kInterruptLine);

/**
 * @brief Returns the base address of the local APIC.
 *
 * @details Returns the base address of the local APIC.
 *
 * @return The base address of the LAPIC is returned.
 */
static uintptr_t _GetBaseAddress(void);

/**
 * @brief Returns the LAPIC identifier.
 *
 * @details Returns the LAPIC identifier for the caller.
 *
 * ­@return The LAPIC identifier is returned.
 */
static uint8_t _GetLAPICId(void);

/**
 * @brief Enables a CPU given its LAPIC id.
 *
 * @details Enables a CPU given its LAPIC id. The startup sequence is
 * executed, using LAPIC IPI.
 *
 * @param[in] kLAPICId The LAPIC identifier for the CPU to start.
 */
static void _StartCpu(const uint8_t kLAPICId);

/**
 * @brief Sends an IPI to a CPU given its LAPIC id.
 *
 * @details Sends an IPI to a a CPU given its LAPIC id.
 *
 * @param[in] kLAPICId The LAPIC identifier IPI destination.
 * @param[in] kVector The vector used to trigger the IPI.
 */
static void _SendIPI(const uint8_t kLAPICId, const uint8_t kVector);

/**
 * @brief Returns the list of detected LAPICs in the system.
 *
 * @details Returns the list of detected LAPICs in the system.
 *
 * @return The list of detected LAPICs in the system is returned.
 */
static const S_LAPICNode* _GetLAPICList(void);

/**
 * @brief Initializes a secondary CPU LAPIC.
 *
 * @details Initializes a secondary CPU LAPIC. This function initializes
 * the secondary CPU LAPIC interrupts and settings.
 */
static void _InitApCPU(void);

/**
 * @brief Reads into the LAPIC controller memory.
 *
 * @details Reads into the LAPIC controller memory.
 *
 * @param[in] kBaseAddr The LAPIC base address.
 * @param[in] kRegister The register to read.
 *
 * @return The value contained in the register.
 */
static inline uint32_t _Read(const uintptr_t kBaseAddr,
                             const uint32_t kRegister);

/**
 * @brief Writes to the LAPIC controller memory.
 *
 * @details Writes to the LAPIC controller memory.
 *
 * @param[in] kBaseAddr The LAPIC base address.
 * @param[in] kRegister The register to write.
 * @param[in] kVal The value to write to the register.
 */
static inline void _Write(const uintptr_t kBaseAddr,
                          const uint32_t kRegister,
                          const uint32_t kVal);


/**
 * @brief Attaches the LAPIC Timer driver to the system.
 *
 * @details Attaches the LAPIC Timer driver to the system. This function will
 * use the FDT to initialize the LAPIC Timer hardware and retreive the LAPIC
 * Timer parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _TimerAttach(const S_FDTNode* pkFdtNode);

/**
 * @brief Calibrates the LAPIC Timer frequency.
 *
 * @details Calibrates the LAPIC Timer frequency. The LAPIC Timer has a base
 * frequency that needs to be detected. We use an alternate time base to
 * calculate it.
 *
 * @param[in] kCpuId The LAPIC Timer CPU id to calibrate
 */
static void _TimerCalibrate(const uint8_t kCpuId);

/**
 * @brief Initial LAPIC Timer interrupt handler.
 *
 * @details LAPIC Timer interrupt handler set at the initialization of the LAPIC
 * Timer. Dummy routine setting EOI.
 *
 * @param[in] pCurrThread Unused, the current thread at the
 * interrupt.
 *
 * @return Returns if the scheduler shall be called on return.
 */
static bool _TimerDummyHandler(void);

/**
 * @brief Enables LAPIC Timer ticks.
 *
 * @details Enables LAPIC Timer ticks by clearing the LAPIC Timer's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 */
static void _TimerEnable(void* pDrvCtrl);

/**
 * @brief Disables LAPIC Timer ticks.
 *
 * @details Disables LAPIC Timer ticks by setting the LAPIC Timer's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 */
static void _TimerDisable(void* pDrvCtrl);

/**
 * @brief Sets the LAPIC Timer's tick frequency.
 *
 * @details Sets the LAPIC Timer's tick frequency. The value must be between the
 * LAPIC Timer frequency range.
 *
 * @param[in] kFreq The new frequency to be set to the LAPIC Timer.
 * @param[in] kCpuId The CPU to set the LAPIC timer of.
 *
 * @warning The value must be between in the LAPIC Timer frequency range.
 */
static void _TimerSetFrequency(const uint64_t kFreq, const uint8_t kCpuId);

/**
 * @brief Returns the LAPIC Timer tick frequency in Hz.
 *
 * @details Returns the LAPIC Timer tick frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The LAPIC Timer tick frequency in Hz.
 */
static uint64_t _TimerGetFrequency(void* pDrvCtrl);

/**
 * @brief Sets the LAPIC Timer tick handler.
 *
 * @details Sets the LAPIC Timer tick handler. This function will be called at
 * each LAPIC Timer tick received.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 * @param[in] handler The handler of the LAPIC Timer interrupt.
 *
 * @return The success state or the error code.
 */
static E_Return _TimerSetHandler(void* pDrvCtrl, T_InterruptHandler handler);

/**
 * @brief Removes the LAPIC Timer tick handler.
 *
 * @details Removes the LAPIC Timer tick handler.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The success state or the error code.
 */
static E_Return _TimerRemoveHandler(void* pDrvCtrl);

/**
 * @brief Acknowledge interrupt.
 *
 * @details Acknowledge interrupt.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 */
static void _TimerAckInterrupt(void* pDrvCtrl);

/**
 * @brief Initializes a secondary CPU LAPIC Timer.
 *
 * @details Initializes a secondary CPU LAPIC Timer. This function
 * initializes the secondary CPU LAPIC timer interrupts and settings.
 *
 * @param[in] kCpuId The CPU identifier for which we should enable the LAPIC
 * timer.
 */
static void _TimerInitApCPU(const uint8_t kCpuId);

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
 * entry will be created in the /proc/lapic directory.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateProcFSEntry(void);

/**
 * @brief Opens the ProcFS timer entry.
 *
 * @details Opens the ProcFS timer entry. This function will open the
 * ProcFS timer entry and return a pointer to the file handle. The file handle
 * will be used to read the information from the kernel.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] kpPath The path to the ProcFS entry.
 * @param[in] flags The flags for opening the file.
 * @param[in] mode The mode for opening the file.
 *
 * @return The pointer to the file handle or -1 in case of error.
 */
static void* _ProcFSTimerOpen(void*       pDriverData,
                              const char* kpPath,
                              int32_t     flags,
                              int32_t     mode);

/**
 * @brief Closes the ProcFS timer entry.
 *
 * @details Closes the ProcFS timer entry. This function will close the
 * ProcFS timer entry and free the resources used by the entry.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSTimerClose(void* pDriverData, void* pFileHandle);

/**
 * @brief Read the ProcFS timer entry.
 *
 * @details Read the ProcFS timer entry. This function will read the
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
static ssize_t _ProcFSTimerRead(void*  pDriverData,
                                void*  pFileHandle,
                                void*  pBuffer,
                                size_t count);
/**
 * @brief Reads the directory entry for the ProcFS timer entry.
 *
 * @details Reads the directory entry for the ProcFS timer entry. This function
 * will read the directory entry for the ProcFS timer entry and write it to the
 * buffer given as parameter. The function will return 1 if there is a directory
 * entry to read, 0 if there is no more directory entry to or an error code.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pDirEntry The directory entry to write the information to.
 *
 * @return The function returns 1 if there is a directory entry to read, 0 if
 * there is no more directory entry to read or an error code.
 */
static int32_t _ProcFSTimerReadDir(void*             pDriverData,
                                   void*             pFileHandle,
                                   S_DirectoryEntry* pDirEntry);

/**
 * @brief Creates the ProcFS entry for the timer driver.
 *
 * @details Creates the ProcFS entry for the timer driver. This function will
 * create the ProcFS entry for the timer driver and register it in the ProcFS.
 * The entry will be used to read the information from the kernel. The
 * entry will be created in the /proc/lapic/timers/x directory.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateProcFSTimerEntry(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/

/** @brief Number of booted CPU counts defined in the CPU init assembly  */
extern volatile uint32_t _bootedCPUCount;

/** @brief Startup code address for secondary CPUs */
extern uint8_t _START_LOW_AP_STARTUP_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief LAPIC driver instance. */
static S_Driver sX86LAPICDriver =
{
  .pName         = "X86 Local APIC Driver",
  .pDescription  = "X86 LAPIC Driver for roOs",
  .pCompatible   = "x86,x86-lapic",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/** @brief LAPIC Timer driver instance. */
static S_Driver sX86LAPICTimerDriver =
{
  .pName         = "X86 LAPIC Timer Driver",
  .pDescription  = "X86 LAPIC Timer Driver for roOs",
  .pCompatible   = "x86,x86-lapic-timer",
  .pVersion      = "1.0",
  .pDriverAttach = _TimerAttach
};

/** @brief LAPIC API driver. */
static S_LAPICDriver sLAPICAPIDriver =
{
  .pSetIRQEOI      = _SetIrqEOI,
  .pGetBaseAddress = _GetBaseAddress,
  .pGetLAPICId     = _GetLAPICId,
  .pStartCpu       = _StartCpu,
  .pSendIPI        = _SendIPI,
  .pGetLAPICList   = _GetLAPICList,
  .pInitApCPU      = _InitApCPU
};

/** @brief LAPIC driver controller instance. There will be only on for all
 * lapics, no need for dynamic allocation
 */
static S_LAPICControler sDrvCtrl =
{
  .baseAddr        = 0,
  .mappingSize     = 0,
  .spuriousIntLine = 0,
  .pLAPICList      = NULL
};

/** @brief LAPIC Timer API driver instance */
static S_LAPICTimerDriver sLAPICTimerAPIDriver =
{
  .pInitApCPU = _TimerInitApCPU
};

/** @brief Local timer controller instance, used by AP CPU */
static S_LAPICTimerControler* spDrvCtrl;

/** @brief ProcFS entry for the LAPIC driver */
static S_ProcFSDirEntry* spProcFSMainEntry = NULL;
/** @brief ProcFS main directory for the LAPIC driver */
static S_ProcFSDirEntry* spProcFSMainDirectory = NULL;
/** @brief ProcFS entry for the LAPIC timer driver  */
static S_ProcFSDirEntry* spProcFSTimerEntry = NULL;

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

/** @brief PROCFS timer operations */
static S_ProcFSFileOperations sProcFSTimerOps =
{
  .pOpen    = _ProcFSTimerOpen,
  .pClose   = _ProcFSTimerClose,
  .pRead    = _ProcFSTimerRead,
  .pWrite   = NULL,
  .pReadDir = _ProcFSTimerReadDir,
  .pIOCTL   = NULL
};

/** @brief PROCFS string */
static char sProcFSString[PROCFS_STRING_LENGTH] = {0};
/** @brief Length of the PROCFS string */
static size_t sProcFSStringLength = 0;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t*     kpUintProp;
  const S_ACPIDriver* skpACPIDriver;
  E_Return            retCode;
  size_t              propLen;
  uintptr_t           lapicPhysAddr;
  size_t              toMap;

  retCode = NO_ERROR;

  /* Get the cpu's spurious int line */
  sDrvCtrl.spuriousIntLine = CPUGetInterruptConfig()->spuriousInterruptLine;

  /* Get the ACPI pHandle */
  kpUintProp = FDTGetProp(pkFdtNode, LAPIC_FDT_ACPI_NODE_PROP, &propLen);
  LAPIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
               "Failed to get ACPI handle for LAPIC",
               ERR_INVALID_VALUE);

  /* Get the ACPI driver */
  skpACPIDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
  LAPIC_ASSERT(skpACPIDriver != NULL,
               "Failed to get ACPI driver for LAPIC",
               ERR_NOT_FOUND);

  /* Map the IO APIC */
  lapicPhysAddr = skpACPIDriver->pGetLAPICBaseAddress() & ~PAGE_SIZE_MASK;
  toMap = LAPIC_MEMORY_SIZE + (lapicPhysAddr & PAGE_SIZE_MASK);
  toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

  sDrvCtrl.baseAddr = (uintptr_t)MemoryKernelMap((void*)lapicPhysAddr,
                                                  toMap,
                                                  MEMMGR_MAP_HARDWARE |
                                                  MEMMGR_MAP_KERNEL   |
                                                  MEMMGR_MAP_RW,
                                                  &retCode);
  LAPIC_ASSERT(sDrvCtrl.baseAddr != (uintptr_t)NULL && retCode == NO_ERROR,
               "Failed to map LAPIC",
               retCode);

  sDrvCtrl.baseAddr |= skpACPIDriver->pGetLAPICBaseAddress() &
                        PAGE_SIZE_MASK;
  sDrvCtrl.mappingSize = toMap;

  /* Get the LAPIC list */
  sDrvCtrl.pLAPICList = skpACPIDriver->pGetLAPICList();

  /* Enable all interrupts */
  _Write(sDrvCtrl.baseAddr, LAPIC_TPR, 0);

  /* Set logical destination mode */
  _Write(sDrvCtrl.baseAddr, LAPIC_DFR, 0xffffffff);
  _Write(sDrvCtrl.baseAddr, LAPIC_LDR, 0x01000000);

  /* Set spurious interrupt vector */
  _Write(sDrvCtrl.baseAddr, LAPIC_SVR, 0x100 | sDrvCtrl.spuriousIntLine);

  /* Set the API driver */
  retCode = DriverManagerSetDeviceData(pkFdtNode, &sLAPICAPIDriver);
  LAPIC_ASSERT(retCode == NO_ERROR, "Failed to set LAPIC API driver", retCode);
  /* Register the driver in the CPU manager */
  CPURegisterLAPICDriver(&sLAPICAPIDriver);

  retCode = _CreateProcFSEntry();
  LAPIC_ASSERT(retCode == NO_ERROR, "Failed to create ProcFS entry", retCode);

  return retCode;
}

static void _SetIrqEOI(const uint32_t kInterruptLine)
{
  /* Interrupt line is not used by LAPIC */
  (void)kInterruptLine;

  _Write(sDrvCtrl.baseAddr, LAPIC_EOI, 0);
}

static uintptr_t _GetBaseAddress(void)
{
  return sDrvCtrl.baseAddr;
}

static uint8_t _GetLAPICId(void)
{
  return (uint8_t)(_Read(sDrvCtrl.baseAddr, LAPIC_ID) >> 24);
}

static void _StartCpu(const uint8_t kLAPICId)
{
  uint32_t tryCount;
  uint32_t oldBootedCpuCount;

  /* Send the INIT IPI */
  _Write(sDrvCtrl.baseAddr, LAPIC_ICRHI, kLAPICId << ICR_DESTINATION_SHIFT);
  _Write(sDrvCtrl.baseAddr, LAPIC_ICRLO,
              ICR_ASSERT | ICR_INIT | ICR_PHYSICAL | ICR_EDGE |
              ICR_NO_SHORTHAND);

  /* Wait for pending sends */
  while ((_Read(sDrvCtrl.baseAddr, LAPIC_ICRLO) & ICR_SEND_PENDING) != 0){}

  /* Wait 10ms */
  TimeWaitNoScheduler(LAPIC_CPU_INIT_DELAY_NS);

  tryCount = 0;
  oldBootedCpuCount = _bootedCPUCount;

  do
  {
    /* Send the STARTUP IPI */
    _Write(sDrvCtrl.baseAddr,
           LAPIC_ICRHI,
           ((uint32_t)kLAPICId) << ICR_DESTINATION_SHIFT);
    _Write(sDrvCtrl.baseAddr, LAPIC_ICRLO,
            LAPIC_STARTUP_ADDR(&_START_LOW_AP_STARTUP_ADDR) |
            ICR_ASSERT | ICR_STARTUP | ICR_PHYSICAL | ICR_EDGE |
            ICR_NO_SHORTHAND);

    /* Wait for pending sends */
    while ((_Read(sDrvCtrl.baseAddr, LAPIC_ICRLO) & ICR_SEND_PENDING) != 0){}

    /* Wait 100ms and check if the number of CPUs was updated */
    TimeWaitNoScheduler(LAPIC_CPU_STARTUP_DELAY_NS);
    if (oldBootedCpuCount != _bootedCPUCount)
    {
      /* Our CPU increased the booted CPU count, stop */
      break;
    }
    ++tryCount;
  } while (tryCount < 5);

  TimeWaitNoScheduler(LAPIC_CPU_STARTUP_WAIT_DELAY_NS);

  LAPIC_ASSERT(oldBootedCpuCount != _bootedCPUCount,
               "Failed to start CPU",
               ERR_UNAUTHORIZED_ACTION);
}

static void _SendIPI(const uint8_t kLAPICId, const uint8_t kVector)
{
  uint32_t intState;

  /* Check if init */
  if (sDrvCtrl.baseAddr == 0)
  {
    return;
  }

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  /* Wait for pending sends */
  while ((_Read(sDrvCtrl.baseAddr, LAPIC_ICRLO) & ICR_SEND_PENDING) != 0){}

  /* Send IPI */
  _Write(sDrvCtrl.baseAddr, LAPIC_ICRHI, kLAPICId << ICR_DESTINATION_SHIFT);
  _Write(sDrvCtrl.baseAddr, LAPIC_ICRLO,
         (kVector & 0xFF) |
         ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

  /* Wait for pending sends */
  while ((_Read(sDrvCtrl.baseAddr, LAPIC_ICRLO) & ICR_SEND_PENDING) != 0){}

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static const S_LAPICNode* _GetLAPICList(void)
{
  return sDrvCtrl.pLAPICList;
}

static void _InitApCPU(void)
{
  /* We are in a secondary CPU (AP CPU), just setup interrupts */
  _Write(sDrvCtrl.baseAddr, LAPIC_TPR, 0);

  /* Set logical destination mode */
  _Write(sDrvCtrl.baseAddr, LAPIC_DFR, 0xffffffff);
  _Write(sDrvCtrl.baseAddr, LAPIC_LDR, 0x01000000);

  /* Set spurious interrupt vector */
  _Write(sDrvCtrl.baseAddr, LAPIC_SVR, 0x100 | sDrvCtrl.spuriousIntLine);

  /* Set EOI */
  _Write(sDrvCtrl.baseAddr, LAPIC_EOI, 0);
}

static E_Return _TimerAttach(const S_FDTNode* pkFdtNode)
{
  const uint32_t*        kpUintProp;
  size_t                 propLen;
  uint32_t               cpuCount;
  E_Return               retCode;
  S_LAPICTimerControler* pDrvCtrl;
  S_KernelTimer*         pTimerDrv;
  S_LAPICDriver*         pLAPICDriver;

  pDrvCtrl  = NULL;
  pTimerDrv = NULL;

  retCode = NO_ERROR;

  cpuCount = CPUGetCount();

  /* Init structures */
  pDrvCtrl = KMalloc(sizeof(S_LAPICTimerControler), KMALLOC_NO_FREE_POOL);
  spDrvCtrl = pDrvCtrl;
  memset(pDrvCtrl, 0, sizeof(S_LAPICTimerControler));

  pDrvCtrl->pInternalFrequency = KMalloc(sizeof(uint64_t) * cpuCount,
                                         KMALLOC_NO_FREE_POOL);
  memset(pDrvCtrl->pInternalFrequency, 0, sizeof(uint64_t) * cpuCount);

  pDrvCtrl->pDisabledNesting = KMalloc(sizeof(uint32_t) * cpuCount,
                                       KMALLOC_NO_FREE_POOL);
  memset(pDrvCtrl->pDisabledNesting, 0, sizeof(uint32_t) * cpuCount);

  pTimerDrv = KMalloc(sizeof(S_KernelTimer), KMALLOC_NO_FREE_POOL);
  memset(pTimerDrv, 0, sizeof(S_KernelTimer));

  pTimerDrv->pGetFrequency  = _TimerGetFrequency;
  pTimerDrv->pEnable        = _TimerEnable;
  pTimerDrv->pDisable       = _TimerDisable;
  pTimerDrv->pSetHandler    = _TimerSetHandler;
  pTimerDrv->pRemoveHandler = _TimerRemoveHandler;
  pTimerDrv->pTickManager   = _TimerAckInterrupt;
  pTimerDrv->pDriverCtrl    = pDrvCtrl;

  /* Get interrupt lines */
  kpUintProp = FDTGetProp(pkFdtNode, LAPICT_FDT_INT_PROP, &propLen);
  LAPIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t) * 2,
               "Invalid Timer FDT configuration.",
               ERR_INVALID_VALUE);
  pDrvCtrl->interruptNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

  /* Get selected frequency */
  kpUintProp = FDTGetProp(pkFdtNode, LAPICT_FDT_SELFREQ_PROP, &propLen);
  LAPIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
               "Invalid Timer FDT configuration.",
               ERR_INVALID_VALUE);
  pDrvCtrl->selectedFrequency = FDTTOCPU32(*kpUintProp);

  /* Get bus frequency divider */
  kpUintProp = FDTGetProp(pkFdtNode, LAPICT_FDT_DIVIDER_PROP, &propLen);
  LAPIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
               "Invalid Timer FDT configuration.",
               ERR_INVALID_VALUE);
  pDrvCtrl->divider = FDTTOCPU32(*kpUintProp);
  switch (pDrvCtrl->divider)
  {
    case 1:
      pDrvCtrl->divider = LAPICT_DIVIDER_1;
      break;
    case 2:
      pDrvCtrl->divider = LAPICT_DIVIDER_2;
      break;
    case 4:
      pDrvCtrl->divider = LAPICT_DIVIDER_4;
      break;
    case 8:
      pDrvCtrl->divider = LAPICT_DIVIDER_8;
      break;
    case 16:
      pDrvCtrl->divider = LAPICT_DIVIDER_16;
      break;
    case 32:
      pDrvCtrl->divider = LAPICT_DIVIDER_32;
      break;
    case 64:
      pDrvCtrl->divider = LAPICT_DIVIDER_64;
      break;
    case 128:
      pDrvCtrl->divider = LAPICT_DIVIDER_128;
      break;
    default:
      PANIC(ERR_NOT_SUPPORTED, MODULE_NAME, "Invalid Divider.", false, false);
  }

  /* Get the LAPIC pHandle */
  kpUintProp = FDTGetProp(pkFdtNode, LAPICT_FDT_LAPIC_NODE_PROP, &propLen);
  LAPIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
               "Invalid Timer FDT configuration.",
               ERR_INVALID_VALUE);

  /* Get the LAPIC driver */
  pLAPICDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
  LAPIC_ASSERT(pLAPICDriver != NULL,
               "LAPIC Timer needs the LAPIC driver to function.",
               ERR_NOT_SUPPORTED);

  /* Get the base timer pHandle */
  kpUintProp = FDTGetProp(pkFdtNode,
                          LAPICT_TIMER_FDT_BASE_TIMER_PROP,
                          &propLen);
  LAPIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
               "Invalid Timer FDT configuration.",
               ERR_INVALID_VALUE);

  /* Get the base timer driver */
  pDrvCtrl->kpBaseTimer = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
  LAPIC_ASSERT(pDrvCtrl->kpBaseTimer != NULL,
               "LAPIC Timer needs the timer driver to function.",
               ERR_NOT_SUPPORTED);
  LAPIC_ASSERT(pDrvCtrl->kpBaseTimer->pGetTimeNs != NULL,
               "LAPIC Timer needs the timer nanosecond support to function.",
               ERR_NOT_SUPPORTED);

  /* Set the base address */
  pDrvCtrl->lapicBaseAddress = pLAPICDriver->pGetBaseAddress();

  /* Init system times */
  pDrvCtrl->pDisabledNesting[0] = 1;

  /* Calibrate the LAPIC Timer */
  _TimerCalibrate(0);

  /* Set LAPIC Timer frequency */
  _TimerSetFrequency(pDrvCtrl->selectedFrequency, 0);

  /* Set interrupt EOI */
  _TimerAckInterrupt(pDrvCtrl);

  /* Register the driver in the CPU manager */
  CPURegisterLAPICTimerDriver(&sLAPICTimerAPIDriver);

  /* Set the API driver */
  retCode = DriverManagerSetDeviceData(pkFdtNode, pTimerDrv);
  LAPIC_ASSERT(retCode == NO_ERROR,
               "Failed to register LAPIC Timer data.",
               retCode);

  retCode = _CreateProcFSTimerEntry();
  LAPIC_ASSERT(retCode == NO_ERROR,
               "Failed to create ProcFS timer entry",
               retCode);

  return retCode;
}

static void _TimerCalibrate(const uint8_t kCpuId)
{
  uint64_t             startTime;
  uint64_t             endTime;
  uint64_t             period;
  uint32_t             lapicTimerCount;
  uintptr_t            kLAPICBaseAddress;
  const S_KernelTimer* kpBaseTimer;

  kLAPICBaseAddress = spDrvCtrl->lapicBaseAddress;
  kpBaseTimer = spDrvCtrl->kpBaseTimer;

  /* Set the LAPIC Timer frequency divider */
  _Write(kLAPICBaseAddress, LAPIC_TDCR, spDrvCtrl->divider);

  /* Write the initial count to the counter */
  _Write(kLAPICBaseAddress, LAPIC_TICR, 0xFFFFFFFFULL);

  /* Get start time */
  startTime = kpBaseTimer->pGetTimeNs(kpBaseTimer->pDriverCtrl);

  /* Wait for calibration */
  do
  {
    endTime = kpBaseTimer->pGetTimeNs(kpBaseTimer->pDriverCtrl);
  } while (endTime < startTime + LAPICT_CALIBRATION_DELAY);

  /* Now that we waited LAPICT_CALIBRATION_DELAY ns calculate the frequency */
  lapicTimerCount = 0xFFFFFFFFULL - _Read(kLAPICBaseAddress, LAPIC_TCCR);

  /* If the period is smaller than the tick count, we cannot calibrate */
  period = (endTime - startTime);
  LAPIC_ASSERT(period >= lapicTimerCount,
               "LAPIC Timer calibration period is too short.",
               ERR_EXCEEDED_LIMIT);

  /* Get the actual frequency and compute the interrupt count */
  spDrvCtrl->pInternalFrequency[kCpuId] = 1000000000 /
                                          (period / lapicTimerCount);
}

static bool _TimerDummyHandler(void)
{
  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "LAPIC Timer Dummy handler called",
        true, false);

  return false;
}

static void _TimerEnable(void* pDrvCtrl)
{
  S_LAPICTimerControler* pLAPICTimerCtrl;
  uint32_t               lapicInitCount;
  uint8_t                cpuId;
  uint32_t               intState;

  pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

  KERNEL_ENTER_CRITICAL_LOCAL(intState);
  cpuId = CPUGetId();


  if (pLAPICTimerCtrl->pDisabledNesting[cpuId] > 0)
  {
    --pLAPICTimerCtrl->pDisabledNesting[cpuId];
  }

  if (pLAPICTimerCtrl->pDisabledNesting[cpuId] == 0)
  {
    /* Set the frequency to set the init counter */
    lapicInitCount = pLAPICTimerCtrl->pInternalFrequency[cpuId] /
                     pLAPICTimerCtrl->selectedFrequency;

    /* Write the initial count to the counter */
    _Write(pLAPICTimerCtrl->lapicBaseAddress, LAPIC_TICR, lapicInitCount);

    /* Enable interrupts */
    _Write(pLAPICTimerCtrl->lapicBaseAddress,
           LAPIC_TIMER,
           pLAPICTimerCtrl->interruptNumber |
           LAPIC_TIMER_MODE_PERIODIC);
  }

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static void _TimerDisable(void* pDrvCtrl)
{
  S_LAPICTimerControler* pLAPICTimerCtrl;
  uint8_t                cpuId;
  uint32_t               intState;

  pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  cpuId = CPUGetId();

  if (pLAPICTimerCtrl->pDisabledNesting[cpuId] < UINT32_MAX)
  {
    ++pLAPICTimerCtrl->pDisabledNesting[cpuId];
  }

  /* Disable interrupt */
  _Write(pLAPICTimerCtrl->lapicBaseAddress, LAPIC_TIMER, LAPIC_LVT_INT_MASKED);

  /* Set counter to 0 */
  _Write(pLAPICTimerCtrl->lapicBaseAddress, LAPIC_TICR, 0);

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static void _TimerSetFrequency(const uint64_t kFreq, const uint8_t kCpuId)
{
  uint32_t lapicInitCount;
  uint32_t intState;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  if (kFreq != 0)
  {
    lapicInitCount = spDrvCtrl->pInternalFrequency[kCpuId] / kFreq;
    if (lapicInitCount != 0)
    {
      /* Write the initial count to the counter */
      _Write(spDrvCtrl->lapicBaseAddress, LAPIC_TICR, lapicInitCount);
      spDrvCtrl->selectedFrequency = kFreq;
    }
  }

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static uint64_t _TimerGetFrequency(void* pDrvCtrl)
{
  uint32_t               intState;
  uint64_t               frequency;
  S_LAPICTimerControler* pLAPICTimerCtrl;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);
  frequency = pLAPICTimerCtrl->selectedFrequency;

  KERNEL_EXIT_CRITICAL_LOCAL(intState);

  return frequency;
}

static E_Return _TimerSetHandler(void* pDrvCtrl, T_InterruptHandler handler)
{
  E_Return               err;
  uint32_t               intState;
  S_LAPICTimerControler* pLAPICTimerCtrl;

  if (handler != NULL)
  {
    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);
    _TimerDisable(pDrvCtrl);

    err = InterruptRegister(pLAPICTimerCtrl->interruptNumber, handler, false);
    if (err == NO_ERROR)
    {
      _TimerEnable(pDrvCtrl);
    }

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
  }
  else
  {
    err = ERR_INVALID_PARAMETER;
  }

  return err;
}

static E_Return _TimerRemoveHandler(void* pDrvCtrl)
{
  return _TimerSetHandler(pDrvCtrl, _TimerDummyHandler);
}

static void _TimerAckInterrupt(void* pDrvCtrl)
{
  S_LAPICTimerControler* pLAPICTimerCtrl;

  pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

  /* Set EOI */
  InterruptSetEOI(pLAPICTimerCtrl->interruptNumber);
}

static void _TimerInitApCPU(const uint8_t kCpuId)
{
  /* We are in a secondary CPU (AP CPU), just setup the counter as all
   * LAPIC timers should have the same frequency
   */
  spDrvCtrl->pDisabledNesting[kCpuId] = 1;

  /* Calibrate the timer */
  _TimerCalibrate(kCpuId);

  /* Set LAPIC Timer frequency */
  _TimerSetFrequency(spDrvCtrl->selectedFrequency, kCpuId);

  /* Enable the timer is needed based on the main cpu */
  if (spDrvCtrl->pDisabledNesting[0] == 0)
  {
    _TimerEnable(spDrvCtrl);
  }

  /* Set interrupt EOI */
  _TimerAckInterrupt(spDrvCtrl);
}

static inline uint32_t _Read(const uintptr_t kBaseAddr,
                             const uint32_t  kRegister)
{
  return _MMIORead32((void*)(kBaseAddr + kRegister));
}

static inline void _Write(const uintptr_t kBaseAddr,
                          const uint32_t  kRegister,
                          const uint32_t  kVal)
{
  _MMIOWrite32((void*)(kBaseAddr + kRegister), kVal);
}

static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode)
{
  size_t* pEntryOffset;

  (void)pDriverData;
  (void)mode;

  if (flags == O_RDONLY && *kpPath == 0)
  {
    pEntryOffset = KMallocUser(sizeof(size_t), NULL);
    if (pEntryOffset == NULL)
    {
      pEntryOffset = (void*)-1;
    }
    else
    {
      *pEntryOffset = 0;
    }
  }
  else
  {
    pEntryOffset = (void*)-1;
  }

  return pEntryOffset;
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
  int32_t retCode;
  size_t* pEntryOffset;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    pEntryOffset = (size_t*)pFileHandle;

    if (*pEntryOffset < sProcFSStringLength)
    {
      retCode = (int32_t)MIN(count, sProcFSStringLength - *pEntryOffset);
      memcpy(pBuffer, sProcFSString + *pEntryOffset, retCode);
      *pEntryOffset += retCode;
    }
    else
    {
      retCode = 0;
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
  E_Return internalCode;

  /* Create the main directory for the lapic and the lapic timers */
  retCode = ProcFSCreateDir(PROCFS_DIR_PATH, NULL, &spProcFSMainDirectory);
  if (retCode == NO_ERROR)
  {
    retCode = ProcFSCreateEntry(PROCFS_ENTRY_PATH,
                                0,
                                spProcFSMainDirectory,
                                &sProcFSOps,
                                NULL,
                                &spProcFSMainEntry);
    if (retCode == NO_ERROR)
    {
      sProcFSStringLength = snprintf(sProcFSString,
                                      PROCFS_STRING_LENGTH,
                                      "Base Address: 0x%p\n"
                                      "Mapping Size: %d\n"
                                      "Spurious Interrupt Line: %d\n",
                                      sDrvCtrl.baseAddr,
                                      sDrvCtrl.mappingSize,
                                      sDrvCtrl.spuriousIntLine);
    }
    else
    {
      internalCode = ProcFSRemoveDir(&spProcFSMainDirectory);
      LAPIC_ASSERT(internalCode == NO_ERROR,
                   "Failed to remove ProcFS main directory",
                   internalCode);
    }
  }

  return retCode;
}

static void* _ProcFSTimerOpen(void*       pDriverData,
                              const char* kpPath,
                              int32_t     flags,
                              int32_t     mode)
{
  S_LAPICTimerProcFSHandle* pHandle;
  const char*               kpPathCursor;
  uint32_t                  timerId;

  (void)pDriverData;
  (void)mode;
  if (flags == O_RDONLY &&
     *kpPath != 0 &&
     strncmp(kpPath, PROCFS_TIMER_DIR_PATH, 6) == 0 &&
     strlen(kpPath) > 6 &&
     kpPath[6] == VFS_PATH_DELIMITER)
  {
    kpPath += 7;

    /* Check that the path is valid */
    kpPathCursor = kpPath;
    while (*kpPathCursor != 0)
    {
      if (*kpPathCursor < '0' || *kpPathCursor > '9')
      {
        break;
      }
      ++kpPathCursor;
    }

    if (*kpPathCursor == 0)
    {
      timerId = (uint32_t)strtoul(kpPath, NULL, 10);
      if (timerId < CPUGetCount())
      {
        pHandle = KMallocUser(sizeof(S_LAPICTimerProcFSHandle), NULL);
        if (pHandle != NULL)
        {
          pHandle->offset = 0;
          pHandle->cpuId  = timerId;
          pHandle->type   = 0;
        }
        {
          pHandle = (void*)-1;
        }
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
  }
  else if (flags == O_RDONLY && *kpPath == 0)
  {
    pHandle = KMallocUser(sizeof(S_LAPICTimerProcFSHandle), NULL);
    if (pHandle != NULL)
    {
      pHandle->offset = 0;
      pHandle->cpuId  = 0;
      pHandle->type   = 1;
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

static int32_t _ProcFSTimerClose(void* pDriverData, void* pFileHandle)
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

static ssize_t _ProcFSTimerRead(void*  pDriverData,
                                void*  pFileHandle,
                                void*  pBuffer,
                                size_t count)
{
  int32_t                      retCode;
  char                         tempBuffer[128];
  size_t                       size;
  ssize_t                      written;
  size_t                       offset;
  size_t                       toWrite;
  const S_LAPICNode*           kpLapicNode;
  uint32_t                     cpuId;
  S_LAPICTimerProcFSHandle*    pHandle;
  size_t                       bufferOffset;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    retCode = 0;
    pHandle = (S_LAPICTimerProcFSHandle*)pFileHandle;
    bufferOffset = pHandle->offset;

    if (pHandle->type == 0)
    {
      /* Generate CPU info */
      size = snprintf(tempBuffer,
                      sizeof(tempBuffer),
                      "CPU Identifier: %d\n",
                      pHandle->cpuId);
      written = size - bufferOffset;

      if (written > 0)
      {
        offset  = size - written;
        toWrite = MIN(written, (ssize_t)count);
        memcpy(pBuffer, tempBuffer + offset, toWrite);
        pHandle->offset += toWrite;
        pBuffer         += toWrite;
        retCode         += toWrite;
        count           -= toWrite;
        bufferOffset    = 0;
      }
      else
      {
        bufferOffset -= size;
      }

      if (count > 0)
      {
        /* Get the current entry */
        cpuId       = pHandle->cpuId;
        kpLapicNode = sDrvCtrl.pLAPICList;
        while (kpLapicNode != NULL && cpuId != 0)
        {
          kpLapicNode = kpLapicNode->pNext;
          --cpuId;
        }
        if (kpLapicNode != NULL)
        {
          size = snprintf(tempBuffer,
                          sizeof(tempBuffer),
                          "LAPIC CPU Identifier: %d\n"
                          "LAPIC Identifier: %d\n"
                          "Flags: %x\n",
                          kpLapicNode->lapic.cpuId,
                          kpLapicNode->lapic.lapicId,
                          kpLapicNode->lapic.flags);
          written = size - bufferOffset;

          if (written > 0)
          {
            offset  = size - written;
            toWrite = MIN(written, (ssize_t)count);
            memcpy(pBuffer, tempBuffer + offset, toWrite);
            pHandle->offset += toWrite;
            pBuffer         += toWrite;
            retCode         += toWrite;
            count           -= toWrite;
            bufferOffset    = 0;
          }
          else
          {
            bufferOffset -= size;
          }

          if (count > 0)
          {
            size = snprintf(tempBuffer,
                            sizeof(tempBuffer),
                            "Interrupt: %d\n"
                            "Frequency: %llu Hz\n"
                            "Interrupt Frequency: %llu Hz\n",
                            spDrvCtrl->interruptNumber,
                            spDrvCtrl->pInternalFrequency[pHandle->cpuId],
                            spDrvCtrl->selectedFrequency);
            written = size - bufferOffset;

            if (written > 0)
            {
              offset  = size - written;
              toWrite = MIN(written, (ssize_t)count);
              memcpy(pBuffer, tempBuffer + offset, toWrite);
              pHandle->offset += toWrite;
              pBuffer         += toWrite;
              retCode         += toWrite;
              count           -= toWrite;
              bufferOffset    = 0;
            }
            else
            {
              bufferOffset -= size;
            }
          }

          if (count > 0)
          {
            size = snprintf(tempBuffer,
                            sizeof(tempBuffer),
                            "Divider: %d\n"
                            "Nesting: %d\n"
                            "Base Address: 0x%p\n",
                            spDrvCtrl->divider,
                            spDrvCtrl->pDisabledNesting[pHandle->cpuId],
                            (void*)spDrvCtrl->lapicBaseAddress);
            written = size - bufferOffset;

            if (written > 0)
            {
              offset  = size - written;
              toWrite = MIN(written, (ssize_t)count);
              memcpy(pBuffer, tempBuffer + offset, toWrite);
              pHandle->offset += toWrite;
              pBuffer         += toWrite;
              retCode         += toWrite;
              count           -= toWrite;
              bufferOffset    = 0;
            }
            else
            {
              bufferOffset -= size;
            }
          }
        }
        else
        {
          size = snprintf(tempBuffer,
                      sizeof(tempBuffer),
                      "Unknown Timer\n");
          written = size - bufferOffset;

          if (written > 0)
          {
            offset  = size - written;
            toWrite = MIN(written, (ssize_t)count);
            memcpy(pBuffer, tempBuffer + offset, toWrite);
            pHandle->offset += toWrite;
            pBuffer         += toWrite;
            retCode         += toWrite;
            count           -= toWrite;
          }
        }
      }
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

static int32_t _ProcFSTimerReadDir(void*             pDriverData,
                                   void*             pFileHandle,
                                   S_DirectoryEntry* pDirEntry)
{
  int32_t                   retCode;
  S_LAPICTimerProcFSHandle* pHandle;
  uint32_t                  cpuCount;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    pHandle = (S_LAPICTimerProcFSHandle*)pFileHandle;

    if (pHandle->type == 1)
    {
      cpuCount = CPUGetCount();

      if (pHandle->offset < cpuCount - 1)
      {
        snprintf(pDirEntry->pName,
                sizeof(pDirEntry->pName),
                "%d",
                pHandle->offset);
        ++pHandle->offset;
        retCode = 1;
      }
      else if (pHandle->offset == cpuCount - 1)
      {
        snprintf(pDirEntry->pName,
                sizeof(pDirEntry->pName),
                "%d",
                pHandle->offset);
        ++pHandle->offset;
        retCode = 0;
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
  }
  else
  {
    retCode = -1;
  }

  return retCode;

}

static E_Return _CreateProcFSTimerEntry(void)
{
  E_Return retCode;

  /* Create the entry */
  retCode = ProcFSCreateEntry(PROCFS_TIMER_DIR_PATH,
                              0,
                              spProcFSMainDirectory,
                              &sProcFSTimerOps,
                              NULL,
                              &spProcFSTimerEntry);
  return retCode;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86LAPICDriver);
DRIVERMGR_REG_FDT(sX86LAPICTimerDriver);

/************************************ EOF *************************************/