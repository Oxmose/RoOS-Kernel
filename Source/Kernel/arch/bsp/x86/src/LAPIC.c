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
 * @brief Local APIC (Advanced programmable interrupt controler) driver.
 *
 * @details Local APIC (Advanced programmable interrupt controler) driver.
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
#include <stdint.h>
#include <Memory.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelQueue.h>
#include <KernelError.h>
#include <TimerManager.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <LAPIC.h>

/* Unit test header */
/* TODO */

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

/** @brief Defines the LAPIC memory size */
#define LAPIC_MEMORY_SIZE 0x3F4

/** @brief Defines the number of times the CPU startup IPI is retried */
#define LAPIC_CPU_STARTUP_RETRIES 5

/** @brief Current module name */
#define MODULE_NAME "X86 LAPIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 LAPIC driver controler. */
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

/** @brief x86 LAPIC Timer driver controler. */
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
    PANIC(ERROR, MODULE_NAME, MSG, false);              \
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

/** @brief Cast a pointer to a LAPIC timer driver controler */
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
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _TimerEnable(void* pDrvCtrl);

/**
 * @brief Disables LAPIC Timer ticks.
 *
 * @details Disables LAPIC Timer ticks by setting the LAPIC Timer's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
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
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
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
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
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
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The success state or the error code.
 */
static E_Return _TimerRemoveHandler(void* pDrvCtrl);

/**
 * @brief Acknowledge interrupt.
 *
 * @details Acknowledge interrupt.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
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

/** @brief LAPIC driver controler instance. There will be only on for all
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
  if (kpUintProp != NULL && propLen == sizeof(uint32_t))
  {
    /* Get the ACPI driver */
    skpACPIDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if (skpACPIDriver != NULL)
    {
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
      if (sDrvCtrl.baseAddr != (uintptr_t)NULL && retCode == NO_ERROR)
      {
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
        if (retCode == NO_ERROR)
        {
          /* Register the driver in the CPU manager */
          CPURegisterLAPICDriver(&sLAPICAPIDriver);
        }
        else
        {
          retCode = MemoryKernelUnmap((void*)sDrvCtrl.baseAddr,
                                      sDrvCtrl.mappingSize);
          LAPIC_ASSERT(retCode == NO_ERROR, "Failed to unmap LAPIC", retCode);
        }
      }
    }
    else
    {
      retCode = ERR_INVALID_VALUE;
    }
  }
  else
  {
    retCode = ERR_INVALID_VALUE;
  }
  /* LAPIC is mandatory */
  LAPIC_ASSERT(retCode == NO_ERROR, "Failed to init LAPIC", retCode);

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
  pDrvCtrl = KMalloc(sizeof(S_LAPICTimerControler),
                     ALIGN_ADDRESS,
                     KMALLOC_NO_FREE_POOL);
  spDrvCtrl = pDrvCtrl;
  memset(pDrvCtrl, 0, sizeof(S_LAPICTimerControler));

  pDrvCtrl->pInternalFrequency = KMalloc(sizeof(uint64_t) * cpuCount,
                                         ALIGN_8_BYTES,
                                         KMALLOC_NO_FREE_POOL);
  memset(pDrvCtrl->pInternalFrequency, 0, sizeof(uint64_t) * cpuCount);

  pDrvCtrl->pDisabledNesting = KMalloc(sizeof(uint32_t) * cpuCount,
                                       ALIGN_4_BYTES,
                                       KMALLOC_NO_FREE_POOL);
  memset(pDrvCtrl->pDisabledNesting, 0, sizeof(uint32_t) * cpuCount);

  pTimerDrv = KMalloc(sizeof(S_KernelTimer),
                      ALIGN_ADDRESS,
                      KMALLOC_NO_FREE_POOL);
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
      PANIC(ERR_NOT_SUPPORTED, MODULE_NAME, "Invalid Timer Divider.", false);
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
        true);

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

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86LAPICDriver);
DRIVERMGR_REG_FDT(sX86LAPICTimerDriver);

/************************************ EOF *************************************/