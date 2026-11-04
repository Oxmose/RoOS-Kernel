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
#include <KernelError.h>
#include <TimerManager.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <TSC.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the calibration time in nanoseconds for the TSC. */
#define TSC_CALIBRATION_DELAY 100000000
/** @brief Current module name */
#define MODULE_NAME "X86 TSC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 TSC driver controller. */
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
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/** @brief Cast a pointer to a TSC driver controller */
#define GET_CONTROLER(PTR) ((S_TSCControler*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the TSC driver to the system.
 *
 * @details Attaches the TSC driver to the system. This function will
 * initialize the TSC hardware and retreive the TSC parameters.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(void);

/**
 * @brief Returns the TSC count frequency in Hz.
 *
 * @details Returns the TSC count frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
 *
 * @return The TSC count frequency in Hz.
 */
static uint64_t _GetFrequency(void* pDrvCtrl);

/**
 * @brief Returns the time elasped since the last timer's reset in ns.
 *
 * @details Returns the time elasped since the last timer's reset in ns. The
 * timer can be set with the pSetTimeNs function.
 *
 * @param[in, out] pDrvCtrl The driver controller used by the registered
 * driver.
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
static S_LayerDriver  sX86TSCDriver =
{
  .pName         = "X86 TSC Driver",
  .pDescription  = "X86 Timestamp Counter for roOs",
  .pVersion      = "1.0",
  .layer         = 2,
  .pDriverAttach = _Attach
};

/** @brief TSC driver controller instance */
static S_TSCControler sController;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(void)
{
  E_Return             retCode;
  S_KernelTimer*       pTimerDrv;
  uint64_t             startTime;
  uint64_t             endTime;
  uint64_t             period;
  uint64_t             startTick;
  uint64_t             endTick;
  uint64_t             tickCount;
  uint32_t             highPart;
  uint32_t             lowPart;
  uint32_t             highPartEnd;
  uint32_t             lowPartEnd;

  pTimerDrv = NULL;

  pTimerDrv = KMalloc(sizeof(S_KernelTimer), KMALLOC_FREE_POOL);
  memset(pTimerDrv, 0, sizeof(S_KernelTimer));

  pTimerDrv->pGetFrequency  = _GetFrequency;
  pTimerDrv->pGetTimeNs     = _GetTimeNs;
  pTimerDrv->pEnable        = NULL;
  pTimerDrv->pDisable       = NULL;
  pTimerDrv->pSetHandler    = NULL;
  pTimerDrv->pRemoveHandler = NULL;
  pTimerDrv->pDriverCtrl    = &sController;

  /* Detect the TSC frequency */
  __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
  startTime = TimeGetAuxTimeNs(0);
  /* Wait for calibration */
  do
  {
    __asm__ __volatile__ ("rdtsc" : "=a"(lowPartEnd), "=d"(highPartEnd));
    endTime = TimeGetAuxTimeNs(0);
  } while (endTime < startTime + TSC_CALIBRATION_DELAY);

  startTick = (((uint64_t)highPart << 32) | (uint64_t)lowPart);
  endTick = (((uint64_t)highPartEnd << 32) | (uint64_t)lowPartEnd);

  /* If the period is smaller than the tick count, we cannot calibrate */
  period = (endTime - startTime) * 1000000000;
  tickCount = endTick - startTick;
  TSC_ASSERT(period >= tickCount,
             "TSC calibration period is too short.",
             ERR_EXCEEDED_LIMIT);

  sController.frequency = 1000000000000000000ULL / ((period) / (tickCount));

  retCode = TimeManagerAddTimer(pTimerDrv, LIFETIME_TIMER);
  TSC_ASSERT(retCode == NO_ERROR, "Failed to register TSC driver.", retCode);

  KPrintfInfo("Detected TSC Frequency: %lluHz\n", sController.frequency);

  return NO_ERROR;
}

static uint64_t _GetFrequency(void* pDrvCtrl)
{
  S_TSCControler* pTscCtrl;

  pTscCtrl = GET_CONTROLER(pDrvCtrl);

  return pTscCtrl->frequency;
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

uint64_t TSCGetTimeNs(void)
{
  return _GetTimeNs(&sController);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_LAYER(sX86TSCDriver);

/************************************ EOF *************************************/