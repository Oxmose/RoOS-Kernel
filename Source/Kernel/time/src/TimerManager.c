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
#include <KernelHeap.h>
#include <KernelQueue.h>
#include <KernelError.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <TimerManager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "TIME MGT"

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
/** @brief A node in the periodic task table. */
typedef struct S_TimerPeriodicNode
{
  /** @brief Pointer to the task function. */
  S_TimerPeriodicTask* pInfo;
  /** @brief Next release time of the task in nanoseconds. */
  uint64_t nextReleaseTime;
  /** @brief CPU ID where the task is running. */
  uint32_t cpuId;

  /** @brief Pointer to the next node in the periodic task table. */
  struct S_TimerPeriodicNode* pNext;
} S_TimerPeriodicNode;



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
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
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

/** @brief The table of periodic tasks */
static S_TimerPeriodicNode** spPeriodicTaskTable = NULL;

/** @brief The lock for the periodic task table */
static T_Spinlock* spPeriodicTaskTableLock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static bool _MainTimerHandler(void)
{
  uint8_t              cpuId;
  uint64_t             currentTimeNs;
  S_TimerPeriodicTask* pTask;
  S_TimerPeriodicNode* pNode;
  cpuId = CPUGetId();

  /* Add a tick count */
  ++spSysTickCount[cpuId];

  /* Execute periodic tasks */
  currentTimeNs = TimeGetUptime();
  SpinlockAcquire(&spPeriodicTaskTableLock[cpuId]);
  pNode = spPeriodicTaskTable[cpuId];
  while (pNode != NULL)
  {
    if (currentTimeNs >= pNode->nextReleaseTime)
    {
      pTask = pNode->pInfo;
      /* Update the next execution time */
      pNode->nextReleaseTime = currentTimeNs + pTask->periodNs;

      /* Call the task */
      pTask->pTask(pTask->pArg);
    }
    pNode = pNode->pNext;
  }
  SpinlockRelease(&spPeriodicTaskTableLock[cpuId]);

  /* Call the main timer's tick manager */
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
    spAuxTimersQueue = KQueueCreate(NULL);
  }

  /* Create the new node */
  pNewNode = KQueueCreateNode((void*)kpTimer, NULL);

  /* Add the timer to the queue */
  KQueuePush(pNewNode, spAuxTimersQueue);
}

void TimeManagerInit(void)
{
  uint32_t i;

  /* Get the number of CPUs */
  sCpuCount = CPUGetCount();

  /* Initialize the structures */
  spSysTickCount = KMalloc(sizeof(uint64_t) * sCpuCount, KMALLOC_NO_FREE_POOL);
  spActiveWait = KMalloc(sizeof(uint64_t) * sCpuCount, KMALLOC_NO_FREE_POOL);

  spPeriodicTaskTable = KMalloc(sizeof(S_TimerPeriodicNode*) * sCpuCount,
                                KMALLOC_NO_FREE_POOL);
  spPeriodicTaskTableLock = KMalloc(sizeof(T_Spinlock) * sCpuCount,
                                    KMALLOC_NO_FREE_POOL);

  /* Create the threads table */
  for (i = 0; i < sCpuCount; ++i)
  {
    spPeriodicTaskTable[i] = NULL;
    SPINLOCK_INIT(spPeriodicTaskTableLock[i]);
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

T_TimerHandle TimeManagerAddTask(S_TimerPeriodicTask* pTask,
                                 const uint32_t       kPeriodNs,
                                 const uint32_t       kCpuId)
{
  S_TimerPeriodicNode* pNode;

  pNode = KMalloc(sizeof(S_TimerPeriodicNode), KMALLOC_FREE_POOL);

  pTask->periodNs        = kPeriodNs;
  pNode->pInfo           = pTask;
  pNode->nextReleaseTime = TimeGetUptime() + kPeriodNs;
  pNode->cpuId           = kCpuId;

  SpinlockAcquire(&spPeriodicTaskTableLock[kCpuId]);
  pNode->pNext = spPeriodicTaskTable[kCpuId];
  spPeriodicTaskTable[kCpuId] = pNode;
  SpinlockRelease(&spPeriodicTaskTableLock[kCpuId]);

  return pNode;
}

void TimeManagerRemoveTask(T_TimerHandle taskHandle)
{
  S_TimerPeriodicNode* pNode;
  S_TimerPeriodicNode* pPrevious;
  uint32_t             cpuId;

  pNode = (S_TimerPeriodicNode*)taskHandle;
  cpuId = pNode->cpuId;

  SpinlockAcquire(&spPeriodicTaskTableLock[cpuId]);
  if (pNode == spPeriodicTaskTable[cpuId])
  {
    spPeriodicTaskTable[cpuId] = pNode->pNext;
  }
  else
  {
    pPrevious = spPeriodicTaskTable[cpuId];
    while (pPrevious != NULL && pPrevious->pNext != pNode)
    {
      pPrevious = pPrevious->pNext;
    }

    if (pPrevious != NULL)
    {
      pPrevious->pNext = pNode->pNext;
    }
  }
  SpinlockRelease(&spPeriodicTaskTableLock[cpuId]);

  KFree(pNode);
}

E_Return TimeManagerAddTimer(S_KernelTimer*    pTimer,
                             const E_TimerType kTimerType)
{
  E_Return retCode;

  if (kTimerType == MAIN_TIMER)
  {
    if (pTimer->pSetHandler != NULL)
    {
      sSysMainTimer = *pTimer;
      sSysMainTimer.pSetHandler(sSysMainTimer.pDriverCtrl, _MainTimerHandler);
      retCode = NO_ERROR;
    }
    else
    {
      retCode = ERR_INVALID_PARAMETER;
    }
  }
  else if (kTimerType == RTC_TIMER)
  {
    sSysRtcTimer = *pTimer;
    if (sSysRtcTimer.pSetHandler != NULL)
    {
      sSysRtcTimer = *pTimer;
      sSysRtcTimer.pSetHandler(sSysRtcTimer.pDriverCtrl, _RTCTimerHandler);
    }
    retCode = NO_ERROR;
  }
  else if (kTimerType == LIFETIME_TIMER)
  {
    sSysLifetimeTimer = *pTimer;
    retCode = NO_ERROR;
  }
  else if (kTimerType == AUX_TIMER)
  {
    _AddAuxTimer(pTimer);
    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

uint64_t TimeGetAuxTimeNs(const uint8_t kAuxTimerId)
{
  S_KernelQueueNode* pNode;
  S_KernelTimer*     pTimer;
  uint8_t            auxTimerCount;
  uint64_t           time;

  time = 0;
  if (spAuxTimersQueue->pHead != NULL)
  {
    pNode = spAuxTimersQueue->pHead;
    auxTimerCount = 0;
    while (pNode != NULL && kAuxTimerId > auxTimerCount)
    {
      pNode = pNode->pNext;
      ++auxTimerCount;
    }

    if (pNode != NULL)
    {
      pTimer = (S_KernelTimer*)pNode->pData;
      if (pTimer->pGetTimeNs != NULL)
      {
        time = pTimer->pGetTimeNs(pTimer->pDriverCtrl);
      }
    }
  }
  return time;
}
/************************************ EOF *************************************/