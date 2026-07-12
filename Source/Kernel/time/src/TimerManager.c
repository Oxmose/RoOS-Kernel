/*******************************************************************************
 * @file TimerManager.c
 *
 * @see TimerManager.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/05/2023
 *
 * @version 1.0
 *
 * @brief Kernel's time management methods.
 *
 * @details Kernel's time management method. Allow to define timers and keep
 * track on the system's time.
 *
 * @warning All the interrupt managers and timer sources drivers must be
 * initialized before using any of these functions.
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
#include <Critical.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelQueue.h>
#include <KernelError.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TimerManager.h>

/* Unit test header */
/* TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "TIME MGT"

/** @brief FDT Time node, time configuration node name */
#define FDT_TIMECONFIG_NODE_NAME "timeconfig"
/** @brief FDT Time node, main timer pHandle */
#define FDT_TIMECONFIG_MAIN_PROP "main"
/** @brief FDT Time node, RTC timer pHandle */
#define FDT_TIMECONFIG_RTC_PROP "rtc"
/** @brief FDT Time node, lifetime timer pHandle */
#define FDT_TIMECONFIG_LIFETIME_PROP "lifetime"
/** @brief FDT Time node, aux timer pHandles */
#define FDT_TIMECONFIG_AUX_PROP "aux"

/** @brief Defines the year of the Epoch */
#define TIMESPAMP_START_YEAR 1970ULL
/** @brief Defines the number of seconds in a leap year */
#define SEC_PER_LEAP_YEAR 31622400ULL
/** @brief Defines the number of seconds in a year */
#define SEC_PER_YEAR 31536000ULL
/** @brief Defines the number of seconds in a day */
#define SEC_PER_DAY 86400ULL
/** @brief Defines the number of seconds in an hour */
#define SEC_PER_HOUR 3600ULL
/** @brief Defines the number of seconds in a minute */
#define SEC_PER_MINUTES 60ULL
/** @brief Tells if the year X is a leap year. */
#define IS_LEAP_YEAR(X) ((((X) % 4 == 0) && ((X) % 100 != 0)) || \
                         ((X) % 400 == 0))

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */


/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the Time manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the Time manager to ensure correctness of
 * execution. Due to the critical nature of the Time manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define TIME_ASSERT(COND, MSG, ERROR) {                   \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief The kernel's main timer interrupt handler.
 *
 * @details The kernel's main timer interrupt handler. This must be connected to
 * the main timer of the system.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _MainTimerHandler(void);

/**
 * @brief The kernel's RTC timer interrupt handler.
 *
 * @details The kernel's RTC timer interrupt handler. This must be connected to
 * the RTC timer of the system.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _RTCTimerHandler(void);

/**
 * @brief Adds an auxiliary timer to the list.
 *
 * @details Adds an auxiliary timer to the list. The AUX timer list might be
 * initialized if not already and the timer is added to the list.
 *
 * @param[in] kpTimer The timer driver to add.
 */
static void _AddAuxTimer(const S_KernelTimer* kpTimer);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Stores the number of CPUs in the system */
static uint32_t sCpuCount;

/**
 * @brief The kernel's main timer interrupt source.
 *
 *  @details The kernel's main timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static S_KernelTimer sSysMainTimer =
{
  .pGetFrequency  = NULL,
  .pGetTimeNs     = NULL,
  .pSetTimeNs     = NULL,
  .pGetDate       = NULL,
  .pGetDaytime    = NULL,
  .pEnable        = NULL,
  .pDisable       = NULL,
  .pSetHandler    = NULL,
  .pRemoveHandler = NULL
};

/**
 * @brief The kernel's RTC timer interrupt source.
 *
 *  @details The kernel's RTC timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static S_KernelTimer sSysRtcTimer =
{
  .pGetFrequency  = NULL,
  .pGetTimeNs     = NULL,
  .pSetTimeNs     = NULL,
  .pGetDate       = NULL,
  .pGetDaytime    = NULL,
  .pEnable        = NULL,
  .pDisable       = NULL,
  .pSetHandler    = NULL,
  .pRemoveHandler = NULL,
};

/**
 * @brief The kernel's lifetime timer.
 *
 *  @details The kernel's lifetime timer. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static S_KernelTimer sSysLifetimeTimer =
{
  .pGetFrequency  = NULL,
  .pGetTimeNs     = NULL,
  .pSetTimeNs     = NULL,
  .pGetDate       = NULL,
  .pGetDaytime    = NULL,
  .pEnable        = NULL,
  .pDisable       = NULL,
  .pSetHandler    = NULL,
  .pRemoveHandler = NULL,
};

/**
 * @brief Stores the number of main kernel's timer tick since the
 * initialization of the time manager.
 */
static uint64_t* spSysTickCount;

/** @brief Auxiliary timers list */
static S_KernelQueue* spAuxTimersQueue = NULL;

/** @brief Active wait counter per CPU. */
static volatile uint64_t* spActiveWait;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static bool _MainTimerHandler(void)
{
  uint8_t cpuId;

  cpuId = CPUGetId();

  /* Add a tick count */
  ++spSysTickCount[cpuId];

  if (sSysMainTimer.pTickManager != NULL)
  {
    sSysMainTimer.pTickManager(sSysMainTimer.pDriverCtrl);
  }

  /* Use coarse active wait if not lifetime timer is present */
  if (spActiveWait[cpuId] != 0)
  {
    /* Use ticks */
    if (spActiveWait[cpuId] <= spSysTickCount[cpuId] * 1000000000 /
            sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl))
    {
      spActiveWait[cpuId] = 0;
    }
  }

  return true;
}

static bool _RTCTimerHandler(void)
{
  if (sSysRtcTimer.pTickManager != NULL)
  {
    sSysRtcTimer.pTickManager(sSysRtcTimer.pDriverCtrl);
  }

  return false;
}

static void _AddAuxTimer(const S_KernelTimer* kpTimer)
{
  S_KernelQueueNode* pNewNode;

  /* Create queue is it does not exist */
  if (spAuxTimersQueue == NULL)
  {
    spAuxTimersQueue = KQueueCreate();
  }

  /* Create the new node */
  pNewNode = KQueueCreateNode((void*)kpTimer);

  /* Add the timer to the queue */
  KQueuePush(pNewNode, spAuxTimersQueue);
}

void TimeManagerInit(void)
{
  E_Return             retCode;
  const S_FDTNode*     kpTimerNode;
  const uint32_t*      kpUintProp;
  size_t               propLen;
  size_t               i;
  const S_KernelTimer* kpTimer;

  /* Get the number of CPUs */
  sCpuCount = CPUGetCount();

  /* Initialize the structures */
  spSysTickCount = KMalloc(sizeof(uint64_t) * sCpuCount,
                           ALIGN_8_BYTES,
                           KMALLOC_NO_FREE_POOL);
  spActiveWait = KMalloc(sizeof(uint64_t) * sCpuCount,
                         ALIGN_8_BYTES,
                         KMALLOC_NO_FREE_POOL);

  /* Get the FDT timers node */
  kpTimerNode = FDTGetNodeByName(FDT_TIMECONFIG_NODE_NAME);
  TIME_ASSERT(kpTimerNode != NULL,
              "The FDT must contain a timer configuration.",
              ERR_INVALID_VALUE);

  /* Get the main timer driver */
  kpUintProp = FDTGetProp(kpTimerNode, FDT_TIMECONFIG_MAIN_PROP, &propLen);
  TIME_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
              "The FDT time configuration must contain only one main.",
              ERR_INVALID_VALUE);

  kpTimer = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
  TIME_ASSERT(kpTimer != NULL &&
              kpTimer->pGetFrequency != NULL &&
              kpTimer->pEnable != NULL &&
              kpTimer->pDisable != NULL &&
              kpTimer->pSetHandler != NULL &&
              kpTimer->pRemoveHandler != NULL &&
              kpTimer->pDriverCtrl != NULL,
              "Invalid main timer driver",
              ERR_INVALID_VALUE);
  sSysMainTimer = *kpTimer;
  retCode = sSysMainTimer.pSetHandler(sSysMainTimer.pDriverCtrl,
                                      _MainTimerHandler);
  TIME_ASSERT(retCode == NO_ERROR, "Failed to setup the main timer", retCode);
  sSysMainTimer.pEnable(sSysMainTimer.pDriverCtrl);

  /* Get the RTC driver */
  kpUintProp = FDTGetProp(kpTimerNode, FDT_TIMECONFIG_RTC_PROP, &propLen);
  if (kpUintProp != NULL)
  {
    TIME_ASSERT(propLen == sizeof(uint32_t),
                "The FDT time configuration must contain at most one RTC.",
                ERR_INVALID_VALUE);

    kpTimer = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    TIME_ASSERT(kpTimer != NULL &&
                kpTimer->pEnable != NULL &&
                kpTimer->pDisable != NULL &&
                kpTimer->pGetDaytime != NULL &&
                kpTimer->pGetDate != NULL &&
                kpTimer->pDriverCtrl != NULL,
                "Invalid RTC timer driver",
                ERR_INVALID_VALUE);
    sSysRtcTimer = *kpTimer;
    retCode = sSysRtcTimer.pSetHandler(sSysRtcTimer.pDriverCtrl,
                                       _RTCTimerHandler);
    TIME_ASSERT(retCode == NO_ERROR, "Failed to setup the RTC timer", retCode);
    sSysRtcTimer.pEnable(sSysRtcTimer.pDriverCtrl);
  }

  /* Get the lifetime driver */
  kpUintProp = FDTGetProp(kpTimerNode, FDT_TIMECONFIG_LIFETIME_PROP, &propLen);
  if (kpUintProp != NULL)
  {
    TIME_ASSERT(propLen == sizeof(uint32_t),
                "The FDT time configuration must contain at most one "
                "lifetime.",
                ERR_INVALID_VALUE);

    kpTimer = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    TIME_ASSERT(kpTimer != NULL &&
                kpTimer->pGetTimeNs != NULL &&
                kpTimer->pEnable != NULL &&
                kpTimer->pDisable != NULL &&
                kpTimer->pDriverCtrl != NULL,
                "Invalid lifetime timer driver",
                ERR_INVALID_VALUE);
    sSysLifetimeTimer = *kpTimer;
    sSysLifetimeTimer.pEnable(sSysLifetimeTimer.pDriverCtrl);
  }

  /* Get other auxiliary timers */
  kpUintProp = FDTGetProp(kpTimerNode, FDT_TIMECONFIG_AUX_PROP, &propLen);
  if (kpUintProp != NULL)
  {
    for (i = 0; i < propLen / sizeof(uint32_t); ++i)
    {
      kpTimer = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
      TIME_ASSERT(kpTimer != NULL &&
                  kpTimer->pEnable != NULL &&
                  kpTimer->pDisable != NULL &&
                  kpTimer->pDriverCtrl != NULL,
                  "Invalid aux timer driver",
                  ERR_INVALID_VALUE);
      _AddAuxTimer(kpTimer);
    }
  }
}

uint64_t TimeGetUptime(void)
{
  uint32_t i;
  uint64_t time;
  uint64_t maxTick;

  if (sSysLifetimeTimer.pGetTimeNs != NULL)
  {
    time = sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl);
  }
  else if (sSysMainTimer.pGetTimeNs != NULL)
  {
    time = sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl);
  }
  else if (sSysMainTimer.pGetFrequency != NULL)
  {
    /* Get the highest time tick */
    maxTick = 0;
    for (i = 0; i < sCpuCount; ++i)
    {
      if (maxTick < spSysTickCount[i])
      {
        maxTick = spSysTickCount[i];
      }
    }
    time = maxTick * 1000000000ULL /
           sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
  }
  else
  {
    time = 0;
  }

  return time;
}

S_DayTime TimeGetDayTime(void)
{
  S_DayTime time;

  if (sSysRtcTimer.pGetDaytime != NULL)
  {
    time = sSysRtcTimer.pGetDaytime(sSysRtcTimer.pDriverCtrl);
  }
  else
  {
    time.hours   = 0;
    time.minutes = 0;
    time.seconds = 0;
  }

  return time;
}

S_Date TimeGetDate(void)
{
  S_Date date;

  if (sSysRtcTimer.pGetDaytime != NULL)
  {
    date = sSysRtcTimer.pGetDate(sSysRtcTimer.pDriverCtrl);
  }
  else
  {
    date.day     = 0;
    date.month   = 0;
    date.weekday = 0;
    date.year    = 0;
  }

  return date;
}

uint64_t TimeGetTicks(const uint8_t kCpuId)
{
  if (kCpuId < sCpuCount)
  {
    return spSysTickCount[kCpuId];
  }
  else
  {
    return 0;
  }
}

void TimeWaitNoScheduler(const uint64_t kNS)
{
  uint64_t currTime;
  uint8_t  cpuId;

  cpuId = CPUGetId();

  spActiveWait[cpuId] = 0;

  if (sSysLifetimeTimer.pGetTimeNs == NULL)
  {
    if (sSysMainTimer.pGetTimeNs != NULL)
    {
      /* Use precise main timer time */
      currTime = sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl);
      while (sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl) <
             currTime + kNS){}
    }
    else if (sSysMainTimer.pGetFrequency != NULL)
    {
      /* Use ticks */
      spActiveWait[cpuId] = kNS + spSysTickCount[cpuId] * 1000000000 /
                    sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
      while (spActiveWait[cpuId] > 0){}
    }
  }
  else
  {
    /* Get current time form lifetimer and wait */
    currTime = sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl);
    while (sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl) <
           currTime + kNS){}
  }
}

/************************************ EOF *************************************/