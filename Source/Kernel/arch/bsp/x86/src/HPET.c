/*******************************************************************************
 * @file HPET.c
 *
 * @see HPET.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/07/2024
 *
 * @version 1.0
 *
 * @brief HPET (High Precision Event Timer) driver.
 *
 * @details HPET (High Precision Event Timer) driver. Timer
 * source in the kernel. This driver provides basic access to the HPET and
 * its features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <MMIO.h>
#include <ACPI.h>
#include <Panic.h>
#include <Memory.h>
#include <ProcFS.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <Critical.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <TimerManager.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <HPET.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief FDT property for interrupt  */
#define HPET_FDT_INT_PROP "interrupts"
/** @brief FDT property for acpi handle */
#define HPET_FDT_ACPI_NODE_PROP "acpi-node"

/** @brief ACPI Procfs directory path */
#define PROCFS_DIR_PATH "hpet"
/** @brief Length of the PROCFS string */
#define PROCFS_STRING_LENGTH 256

/** @brief Defines the mask for the main counter tick period */
#define HPET_CAPABILITIES_PERIOD_MASK 0xFFFFFFFF00000000ULL
/** @brief Defines the shift for the main counter tick period */
#define HPET_CAPABILITIES_PERIOD_SHIFT 32
/** @brief Defines the mask for the size of the main counter (32/64) */
#define HPET_CAPABILITIES_SIZE_MASK 0x0000000000002000ULL
/** @brief Defines the shift for the size of the main counter (32/64) */
#define HPET_CAPABILITIES_SIZE_SHIFT 13
/** @brief Defines the mask for the number of comparators */
#define HPET_CAPABILITIES_COUNT_MASK 0x0000000000001F00ULL
/** @brief Defines the shift for the number of comparators */
#define HPET_CAPABILITIES_COUNT_SHIFT 8

/** @brief Enable count bit in the general configuration register */
#define HPET_CONFIGURATION_ENABLE_COUNT 0x1ULL

/** @brief Current module name */
#define MODULE_NAME "X86 HPET"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief The HPET comparators register structure */
typedef struct
{
  /** @brief Stores the configuration and capabilities data */
  uint64_t configurationReg;
  /** @brief Stores the comparator value */
  uint64_t comparatorValue;
  /** @brief Stores the FSB interrupt routing */
  uint64_t interruptRouting;
  /** @brief Padding */
  uint64_t padding;
} __attribute__((packed)) S_HPETComparator;

/** @brief The HPET general interrupt status register structure */
typedef struct
{
  /** @brief Interrupt status. */
  uint32_t interruptStatus;
  /** @brief Reserved bits */
  uint32_t reserved;
} __attribute__((packed)) S_HPETGeneralInterruptStatus;

/** @brief The HPET register structure */
typedef struct
{
  /** @brief General capabilities register */
  uint64_t capabilities;
  /** @brief Padding */
  uint64_t padding0;
  /** @brief HPET general configuration register */
  uint64_t configuration;
  /** @brief Padding */
  uint64_t padding1;
  /** @brief Stores the current interrupt status for the HPET */
  S_HPETGeneralInterruptStatus interruptStatus;
  /** @brief Padding */
  uint8_t padding2[0xC8];
  /** @brief Stores the counter value */
  uint64_t counterValue;
  /** @brief Padding */
  uint64_t padding3;
  /** @brief Variable size array of the comparator registers */
  S_HPETComparator comparator[];
} __attribute__((packed)) S_HPETRegisters;

/** @brief x86 HPET Timer driver controler. */
typedef struct
{
  /** @brief HPET Timer interrupt number. */
  uint8_t interruptNumber;
  /* @brief Stores if the counter is 32bits or 64bits wide */
  bool countIs64Bits;
  /** @brief Number of supported comparators */
  uint8_t comparatorsCount;
  /** @brief Keeps track on the HPET enabled state. */
  uint32_t disabledNesting;
  /** @brief HPET registers mapped in memory */
  S_HPETRegisters* pRegisters;
  /** @brief Stores the base tick period of the HPET */
  uint32_t basePeriod;
  /** @brief Time base driver */
  const S_KernelTimer* kpBaseTimer;
} S_HPETControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the HPET to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the HPET to ensure correctness of
 * execution. Due to the critical nature of the HPET, any error generates
 * a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define HPET_ASSERT(COND, MSG, ERROR) {                   \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/** @brief Cast a pointer to a LAPIC driver controler */
#define GET_CONTROLER(PTR) ((S_HPETControler*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the HPET driver to the system.
 *
 * @details Attaches the HPET driver to the system. This function will
 * use the FDT to initialize the HPET hardware and retreive the HPET
 * parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Initializes the HPET.
 *
 * @details Initializes the HPET with the values stores in the HPET controller.
 * This function searched for a HPET in the ACPI and use the first found.
 * Other HPETs are not used.
 *
 * @param[in, out] pCtrl The HPET controller to initialize.
 * @param[in] pkFdtNode The FDT node used to initialize the driver.
 *
 * @return The success of error state is returned.
 */
static E_Return _Init(S_HPETControler* pCtrl, const S_FDTNode* pkFdtNode);

/**
 * @brief Returns the time elasped since the last timer's reset in ns.
 *
 * @details Returns the time elasped since the last timer's reset in ns. The
 * timer can be set with the pSetTimeNs function.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The time in nanosecond since the last timer reset is returned.
 */
static uint64_t _GetTimeNs(void* pDrvCtrl);

/**
 * @brief Enables the HPET comparator 0 interrupt.
 *
 * @details Enables the HPET comparator 0 interrupt. Only one comparator is
 * allowed per instance.
 *
 * @param[in, out] pDrvCtrl The HPET controller to use.
 */
static void _Enable(void* pDrvCtrl);

/**
 * @brief Disables the HPET comparator 0 interrupt.
 *
 * @details Disables the HPET comparator 0 interrupt. Only one comparator is
 * allowed per instance.
 *
 * @param[in, out] pDrvCtrl The HPET controller to use.
 */
static void _Disable(void* pDrvCtrl);

/**
 * @brief Opens the HPET ProcFS main entry.
 *
 * @details Opens the HPET ProcFS main entry. This function will open the HPET
 * ProcFS main entry and return a pointer to the file handle. The file handle
 * will be used to read the HPET information from the kernel.
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
 * @brief Closes the HPET ProcFS main entry.
 *
 * @details Closes the HPET ProcFS main entry. This function will close the HPET
 * ProcFS main entry and free the resources used by the entry.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle);

/**
 * @brief Read the HPET ProcFS main entry.
 *
 * @details Read the HPET ProcFS main entry. This function will read the HPET
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the HPET information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count);

/**
 * @brief Creates the ProcFS entry for the HPET driver.
 *
 * @details Creates the ProcFS entry for the HPET driver. This function will
 * create the ProcFS entry for the HPET driver and register it in the ProcFS.
 * The entry will be used to read the HPET information from the kernel. The
 * entry will be created in the /proc/hpet directory.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateProcFSEntry(void);


/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief HPET driver instance. */
static S_Driver sX86HPETDriver =
{
  .pName         = "X86 HPET Driver",
  .pDescription  = "X86 High Precision Event Timer for roOs.",
  .pCompatible   = "x86,x86-hpet",
  .pVersion      = "1.0",
  .pDriverAttach = _Attach
};

/** @brief Local timer controller instance */
static S_HPETControler sDrvCtrl;

/** @brief PROCFS main entry */
static S_ProcFSDirEntry* spProcFSMainEntry = NULL;

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

/** @brief PROCFS string */
static char sProcFSString[PROCFS_STRING_LENGTH] = {0};
/** @brief Length of the PROCFS string */
static size_t sProcFSStringLength = 0;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t* kpUintProp;
  size_t          propLen;
  E_Return        retCode;
  S_KernelTimer*  pTimerDrv;

  pTimerDrv = NULL;

  /* Init structures */
  memset(&sDrvCtrl, 0, sizeof(S_HPETControler));

  pTimerDrv = KMalloc(sizeof(S_KernelTimer),
                      ALIGN_ADDRESS,
                      KMALLOC_NO_FREE_POOL);
  memset(pTimerDrv, 0, sizeof(S_KernelTimer));
  pTimerDrv->pEnable     = _Enable;
  pTimerDrv->pDisable    = _Disable;
  pTimerDrv->pGetTimeNs  = _GetTimeNs;
  pTimerDrv->pDriverCtrl = &sDrvCtrl;

  /* Get interrupt lines */
  kpUintProp = FDTGetProp(pkFdtNode, HPET_FDT_INT_PROP, &propLen);
  HPET_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t) * 2,
              "Failed to get the HPET interrupt",
              ERR_INVALID_VALUE);

  sDrvCtrl.interruptNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

  /* Initializes the HPET */
  retCode = _Init(&sDrvCtrl, pkFdtNode);
  HPET_ASSERT(retCode == NO_ERROR, "Failed to initialize the HPET", retCode);
  /* Set the API driver */
  retCode = DriverManagerSetDeviceData(pkFdtNode, pTimerDrv);
  HPET_ASSERT(retCode == NO_ERROR, "Failed to set the HPET driver", retCode);

  retCode = _CreateProcFSEntry();
  HPET_ASSERT(retCode == NO_ERROR, "Failed to create the HPET ProcFS", retCode);

  return retCode;
}

static E_Return _Init(S_HPETControler* pCtrl, const S_FDTNode* pkFdtNode)
{
  const uint32_t*         kpUintProp;
  const S_HPETNode*       kpHpetNode;
  const S_ACPIDriver*     kpACPIDriver;
  const S_HPETDescriptor* kpDesc;
  uintptr_t               basePhysAddr;
  size_t                  propLen;
  size_t                  hpetSize;
  E_Return                retCode;

  kpHpetNode = NULL;

  /* Get the ACPI pHandle */
  kpUintProp = FDTGetProp(pkFdtNode, HPET_FDT_ACPI_NODE_PROP, &propLen);
  if (kpUintProp != NULL && propLen == sizeof(uint32_t))
  {
    /* Get the ACPI driver */
    kpACPIDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if (kpACPIDriver != NULL)
    {
      /* Get the HPET list */
      kpHpetNode = kpACPIDriver->pGetHPETList();
    }
  }

  if (kpHpetNode != NULL)
  {
    /* We only support one HPET, get the first */
    kpDesc = &kpHpetNode->hpet;

    /* Get the HPET size */
    hpetSize = sizeof(S_HPETRegisters) +
               sizeof(S_HPETComparator) * kpDesc->comparatorCount;

    /* Align address and update size to map */
    basePhysAddr = kpDesc->address & ~PAGE_SIZE_MASK;
    hpetSize += kpDesc->address - basePhysAddr;

    /* Align size and map the memory */
    hpetSize = (hpetSize + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;
    pCtrl->pRegisters = MemoryKernelMap((void*)basePhysAddr,
                                        hpetSize,
                                        MEMMGR_MAP_HARDWARE |
                                        MEMMGR_MAP_KERNEL   |
                                        MEMMGR_MAP_RW,
                                        &retCode);
    if (retCode == NO_ERROR)
    {
      pCtrl->pRegisters = (void*)((uintptr_t)pCtrl->pRegisters |
                                  (kpDesc->address & PAGE_SIZE_MASK));

      /* Enable the count */
      pCtrl->pRegisters->configuration |= HPET_CONFIGURATION_ENABLE_COUNT;

      /* Init the controller */
      pCtrl->basePeriod = (pCtrl->pRegisters->capabilities &
                           HPET_CAPABILITIES_PERIOD_MASK) >>
                           HPET_CAPABILITIES_PERIOD_SHIFT;
      pCtrl->countIs64Bits = (pCtrl->pRegisters->capabilities &
                              HPET_CAPABILITIES_SIZE_MASK) >>
                              HPET_CAPABILITIES_SIZE_SHIFT;
      pCtrl->comparatorsCount = 1 + ((pCtrl->pRegisters->capabilities &
                                      HPET_CAPABILITIES_COUNT_MASK) >>
                                     HPET_CAPABILITIES_COUNT_SHIFT);
    }
  }
  else
  {
    retCode = ERR_INVALID_VALUE;
  }

  return retCode;
}

static uint64_t _GetTimeNs(void* pDrvCtrl)
{
  uint64_t         timeTicks;
  S_HPETControler* pCtrl;

  pCtrl = GET_CONTROLER(pDrvCtrl);

  /* Read the timer counter */
  timeTicks = pCtrl->pRegisters->counterValue * pCtrl->basePeriod / 1000000;

  return timeTicks;
}

static void _Enable(void* pDrvCtrl)
{
  S_HPETControler* pCtrl;

  pCtrl = GET_CONTROLER(pDrvCtrl);

  /* Enable the count */
  pCtrl->pRegisters->configuration |= HPET_CONFIGURATION_ENABLE_COUNT;
}

static void _Disable(void* pDrvCtrl)
{
  S_HPETControler* pCtrl;

  pCtrl = GET_CONTROLER(pDrvCtrl);

  /* Disable the count */
  pCtrl->pRegisters->configuration &= ~HPET_CONFIGURATION_ENABLE_COUNT;
}

static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode)
{
  size_t* pEntryOffset;

  (void)pDriverData;
  (void)mode;

  if(flags == O_RDONLY && *kpPath == 0)
  {
    pEntryOffset = KMallocUser(sizeof(size_t), ALIGN_ADDRESS, NULL);
    if (pEntryOffset != NULL)
    {
      *pEntryOffset = 0;
    }
    else
    {
      pEntryOffset = (void*)-1;
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

  if(pFileHandle != (void*)-1 && pFileHandle != NULL)
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

  if(pFileHandle != (void*)-1 && pFileHandle != NULL)
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
  E_Return                 retCode;

  retCode = ProcFSCreateEntry(PROCFS_DIR_PATH,
                              0,
                              NULL,
                              &sProcFSOps,
                              NULL,
                              &spProcFSMainEntry);
  if (retCode == NO_ERROR)
  {
    sProcFSStringLength = snprintf(sProcFSString,
                                  PROCFS_STRING_LENGTH,
                                  "Interrupt: %d\n"
                                  "Is 64 Bits: %d\n"
                                  "Comparators Count: %d\n"
                                  "Disabled Nesting: %d\n"
                                  "Base Period (fs): %d\n",
                                  sDrvCtrl.interruptNumber,
                                  sDrvCtrl.countIs64Bits,
                                  sDrvCtrl.comparatorsCount,
                                  sDrvCtrl.disabledNesting,
                                  sDrvCtrl.basePeriod);
  }

  return retCode;
}
/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86HPETDriver);

/************************************ EOF *************************************/