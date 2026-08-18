/*******************************************************************************
 * @file tsc.c
 *
 * @see tsc.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 04/06/2024
 *
 * @version 1.0
 *
 * @brief TSC (Timestamp Counter) driver.
 *
 * @details TSC (Timestamp Counter) driver. Used as the tick timer
 * source in the kernel. This driver provides basic access to the TSC.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <Panic.h>
#include <string.h>
#include <stdint.h>
#include <Interrupts.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <TimerManager.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TSC.h>

/* Unit test header */
/* TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief FDT property for base timer */
#define TSC_FDT_BASE_TIMER_PROP "base-timer"
/** @brief Defines the calibration time in nanoseconds for the TSC. */
#define TSC_CALIBRATION_DELAY 100000000
/** @brief Current module name */
#define MODULE_NAME "X86 TSC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 TSC driver controler. */
typedef struct
{
  /** @brief Counter frequency. */
  uint64_t frequency;
  /** @brief Offset time */
  int64_t offsetTime;
} S_TSCControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the TSC to ensure correctness of execution.
 *
 * @details Assert macro used by the TSC to ensure correctness of execution.
 * Due to the critical nature of the TSC, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define TSC_ASSERT(COND, MSG, ERROR) {                    \
  if ((COND) == false)                                     \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
  }                                                       \
}

/** @brief Cast a pointer to a TSC driver controler */
#define GET_CONTROLER(PTR) ((S_TSCControler*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the TSC driver to the system.
 *
 * @details Attaches the TSC driver to the system. This function will use the
 * FDT to initialize the TSC hardware and retreive the TSC parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Unused, TSC does not support enabling / disabling.
 *
 * @details Unused, TSC does not support enabling / disabling.
 *
 * @param[in, out] pDrvCtrl Unused.
 */
static void _Enable(void* pDrvCtrl);

/**
 * @brief Unused, TSC does not support enabling / disabling.
 *
 * @details Unused, TSC does not support enabling / disabling.
 *
 * @param[in, out] pDrvCtrl Unused.
 */
static void _Disable(void* pDrvCtrl);

/**
 * @brief Returns the TSC count frequency in Hz.
 *
 * @details Returns the TSC count frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The TSC count frequency in Hz.
 */
static uint64_t _GetFrequency(void* pDrvCtrl);

/**
 * @brief Unused, TSC does not support interrupts.
 *
 * @details Unused, TSC does not support interrupts.
 *
 * @param[in, out] pDrvCtrl Unused.
 * @param[in] handler Unused.
 *
 * @return OS_ERR_NOT_SUPPORTED is always returned.
 */
static E_Return _SetHandler(void* pDrvCtrl, T_InterruptHandler handler);

/**
 * @brief Unused, TSC does not support interrupts.
 *
 * @details Unused, TSC does not support interrupts.
 *
 * @param[in, out] pDrvCtrl Unused.
 *
 * @return OS_ERR_NOT_SUPPORTED is always returned.
 */
static E_Return _RemoveHandler(void* pDrvCtrl);

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

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief TSC driver instance. */
static S_Driver sX86TSCDriver =
{
  .pName         = "X86 TSC Driver",
  .pDescription  = "X86 Timestamp Counter for roOs",
  .pCompatible   = "x86,x86-tsc",
  .pVersion      = "1.0",
  .pDriverAttach = _Attach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t*      kpUintProp;
  size_t               propLen;
  E_Return             retCode;
  S_TSCControler*      pDrvCtrl;
  S_KernelTimer*       pTimerDrv;
  const S_KernelTimer* kpBaseTimer;
  uint64_t             startTime;
  uint64_t             endTime;
  uint64_t             period;
  uint64_t             startTick;
  uint64_t             endTick;
  uint64_t             tickCount;
  uint32_t             highPart;
  uint32_t             lowPart;

  pDrvCtrl  = NULL;
  pTimerDrv = NULL;

  /* Init structures */
  pDrvCtrl = KMalloc(sizeof(S_TSCControler), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
  memset(pDrvCtrl, 0, sizeof(S_TSCControler));

  pTimerDrv = KMalloc(sizeof(S_KernelTimer), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
  memset(pTimerDrv, 0, sizeof(S_KernelTimer));

  pTimerDrv->pGetFrequency  = _GetFrequency;
  pTimerDrv->pGetTimeNs     = _GetTimeNs;
  pTimerDrv->pEnable        = _Enable;
  pTimerDrv->pDisable       = _Disable;
  pTimerDrv->pSetHandler    = _SetHandler;
  pTimerDrv->pRemoveHandler = _RemoveHandler;
  pTimerDrv->pDriverCtrl    = pDrvCtrl;

  /* Get the base timer pHandle */
  kpUintProp = FDTGetProp(pkFdtNode,
                          TSC_FDT_BASE_TIMER_PROP,
                          &propLen);
  TSC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
             "Invalid TSC FDT configuration.",
             ERR_INVALID_VALUE);

  /* Get the base timer driver */
  kpBaseTimer = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
  TSC_ASSERT(kpBaseTimer != NULL,
             "TSC Timer needs the timer driver to function.",
             ERR_NOT_SUPPORTED);
  TSC_ASSERT(kpBaseTimer->pGetTimeNs != NULL,
             "TSC Timer needs the timer nanosecond support to function.",
             ERR_NOT_SUPPORTED);


  /* Detect the TSC frequency */
  __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
  startTick = (((uint64_t)highPart << 32) | (uint64_t)lowPart);
  startTime = kpBaseTimer->pGetTimeNs(kpBaseTimer->pDriverCtrl);
  /* Wait for calibration */
  do
  {
    endTime = kpBaseTimer->pGetTimeNs(kpBaseTimer->pDriverCtrl);
    __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
    endTick = (((uint64_t)highPart << 32) | (uint64_t)lowPart);
  } while (endTime < startTime + TSC_CALIBRATION_DELAY);


  /* If the period is smaller than the tick count, we cannot calibrate */
  period = (endTime - startTime) * 1000000000;
  tickCount = endTick - startTick;
  TSC_ASSERT(period >= tickCount,
             "TSC calibration period is too short.",
             ERR_EXCEEDED_LIMIT);

  pDrvCtrl->frequency = 1000000000000000000ULL / ((period) / (tickCount));

  /* Set the API driver */
  retCode = DriverManagerSetDeviceData(pkFdtNode, pTimerDrv);
  TSC_ASSERT(retCode == NO_ERROR,
             "Failed to register the TSC device",
             retCode);

  return NO_ERROR;
}

static void _Enable(void* pDrvCtrl)
{
  (void)pDrvCtrl;
}

static void _Disable(void* pDrvCtrl)
{
  (void)pDrvCtrl;
}

static uint64_t _GetFrequency(void* pDrvCtrl)
{
  S_TSCControler* pTscCtrl;

  pTscCtrl = GET_CONTROLER(pDrvCtrl);

  return pTscCtrl->frequency;
}

static E_Return _SetHandler(void* pDrvCtrl, T_InterruptHandler handler)
{
  (void)pDrvCtrl;
  (void)handler;

  return ERR_NOT_SUPPORTED;
}

static E_Return _RemoveHandler(void* pDrvCtrl)
{
  (void)pDrvCtrl;
  return ERR_NOT_SUPPORTED;
}

static uint64_t _GetTimeNs(void* pDrvCtrl)
{
  S_TSCControler* pTscCtrl;
  uint64_t        time;
  uint32_t        highPart;
  uint32_t        lowPart;

  pTscCtrl = GET_CONTROLER(pDrvCtrl);

  /* Get time */
  __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
  /* Manage Ghz frequencies: use picoseconds */
  time = 1000000000000UL / pTscCtrl->frequency *
          (((uint64_t)highPart << 32) | (uint64_t)lowPart);

  /* Return to ns and apply offset */
  time /= 1000;

  return time;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86TSCDriver);

/************************************ EOF *************************************/