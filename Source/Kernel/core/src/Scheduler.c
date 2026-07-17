/*******************************************************************************
 * @file Scheduler.c
 *
 * @see Scheduler.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/06/2024
 *
 * @version 6.0
 *
 * @brief Kernel's thread scheduler.
 *
 * @details Kernel's thread scheduler. Thread creation and management functions
 * are located in this file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <Memory.h>
#include <Critical.h>
#include <FastQueue.h>
#include <CtrlBlock.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <TimerManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <Scheduler.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "SCHEDULER"

/** @brief Main process name */
#define MAIN_PROCESS_NAME "ROOS_KERNEL\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
/** @brief Idle threads name */
#define IDLE_THREAD_NAME "ROOS_IDLE\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"

/** @brief Scheduler requests queue size in number of elements */
#define SCHEDULER_REQUEST_QUEUE_SIZE 25

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines a scheduling context. */
typedef struct
{
  /** @brief Currently executing thread in this context. */
  S_KernelThread* pCurrentThread;
  /** @brief Context idle thread. */
  S_KernelThread* pIdleThread;
  /** @brief Context ready thread list. */
  S_KernelQueue* pReadyList[KERNEL_LOWEST_PRIORITY + 1];
  /** @brief Context sleeping thread list. */
  S_KernelQueue* pSleepingList;
  /** @brief Stores the highest priority currently present in the context. */
  uint32_t highestPriority;
  /** @brief CPU identifier to which the context is associated. */
  uint32_t mappedCpu;
  /** @brief Stores the queues queues */
  S_FastQueue** ppRequestQueues;
  /** @brief Stores the context statistics. */
  S_ScheduleContextStatistics stats;
} S_ScheduleContext;

/** @brief Defines the types of scheduler request. */
typedef enum
{
  /** @brief Request to update a thread state. */
  REQUEST_THREAD_STATE_UPDATE
} E_SchedulerRequestType;

/** @brief Defines a scheduler request structure. */
typedef struct
{
  /** @brief The type of request */
  E_SchedulerRequestType type;
  /** @brief Parameters of the request */
  void *pParams[3];
} S_SchedulerRequest;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the driver manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the driver manager to ensure correctness of
 * execution. Due to the critical nature of the driver manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define SCHED_ASSERT(COND, MSG, ERROR) {           \
  if ((COND) == false)                             \
  {                                                \
    PANIC(ERROR, MODULE_NAME, MSG, false);         \
  }                                                \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief IDLE thread routine.
 *
 * @details IDLE thread routine. This thread should always be ready, it is the
 * only thread running when no other trhread are ready. It allows better power
 * consumption management and CPU usage computation.
 *
 * @param[in] pArgs The argument to send to the IDLE thread, usualy NULL or the
 * CPU id.
 *
 * @warning The IDLE thread routine should never return.
 */
static void* _IdleRoutine(void* pArgs);

/**
 * @brief Kernel thread entry point routine wrapper.
 *
 * @details Kernel thread launch routine. Wrapper for the actual thread routine.
 * The wrapper will call the thread routine, pass its arguments and gather the
 * return value of the thread function to allow the joining thread to retreive
 * it. Some statistics about the thread might be added in this function.
 */
static void _SchedulerThreadEntry(void);

/* TODO */
static void _SchedulerThreadExit(S_KernelThread* pThread);

/**
 * @brief Creates the kernel main process.
 *
 * @details Creates the kernel main process. This is the first and last process
 * that will execute. It contains the IDLE threads and other kernel-critical
 * threads. The main process uses the kernel page directory.
 *
 * @return The function returns a pointer to the created main proces.
 */
static S_KernelProcess* _CreateMainProcess(void);

/**
 * @brief Creates the idle thread for a CPU.
 *
 * @details Creates the idle thread for a CPU. The idle threds are part of
 * the kernel main process.
 *
 * @param[in/out] pContext The scheduling context to which the idle thread will
 * be added.
 * @param[in] kCPUId The CPU id for which the thread will be created.
 * @param[in/out] pMainProcess The pointer to the main process structure.
 */
static void _CreateIdleThread(S_ScheduleContext* pContext,
                              const uint32_t     kCPUId,
                              S_KernelProcess*   pMainProcess);

/**
 * @brief Elects the next running thread from a scheduler context.
 *
 * @details Elects the next running thread from a scheduler context. This will
 * set the thread to the running state and remove the thread from the context
 * ready list.
 *
 * @param[in/out] pContext The context to elect the thread from.
 *
 * @return The pointer to the elected thread is returned.
 */
static S_KernelThread* _ElectNextThread(S_ScheduleContext* pContext);

/**
 * @brief Manages the state of a thhread.
 *
 * TODO
 */
static void _ManageThreadState(S_KernelThread* pThread);

/* TODO */
static S_ScheduleContext* _SelectNextContext(S_KernelThread* pThread);

/**
 * @brief Sets a thread to the ready state.
 *
 * @details Sets a thread to the ready state. This will set the thread to ready
 * and add it to the context's ready list.
 *
 * @param[in/out] pContext The context to which the thread should be put on
 * the ready list.
 * @param[in/out] pThread The thread to set to ready.
 */
static void _SetThreadToReady(S_ScheduleContext* pContext,
                              S_KernelThread*    pThread);

/**
 * @brief Sets a thread to the sleeping state.
 *
 * @details Sets a thread to the sleeping state. This will set the thread to
 * sleeping add it to the context's sleeping list.
 *
 * @param[in/out] pContext The context to which the thread should be put on
 * the sleeping list.
 * @param[in/out] pThread The thread to set to sleeping.
 */
static void _SetThreadToSleeping(S_ScheduleContext* pContext,
                                 S_KernelThread*    pThread);

/**
 * @brief Sets a thread to the joining state.
 *
 * @details Sets a thread to the joining state. No context is updated as the
 * thread should be stored in the resource it is joining.
 *
 * @param[in/out] pThread The thread to set to joining.
 */
static void _SetThreadToJoining(S_KernelThread* pThread);

/**
 * @brief Sets a thread to the zombie state.
 *
 * @details Sets a thread to the zombie state.  No context is updated as the
 * thread should be stored in the resource that is joining it and will clean it.
 *
 * @param[in/out] pThread The thread to set to zombie.
 */
static void _SetThreadToZombie(S_KernelThread* pThread);

/**
 * @brief Releases the resources used by a thread and destroys it.
 *
 * @details Releases the resources used by a thread and destroys it. This
 * release all memory used by the thread and its internal kernel structures.
 *
 * @param[out] pThread The thread to clean.
 */
static void _CleanThread(S_KernelThread* pThread);


/* TODO */
static void _DispatchThreadStateUpdateRequest(S_ScheduleContext*  pContext,
                                              S_KernelThread*     pThread,
                                              const E_ThreadState kState);
static void _HandleSchedulerRequests(S_ScheduleContext* pContext);
static void _RequestThreadStateUpdate(S_ScheduleContext*  pContext,
                                      S_KernelThread*     pThread,
                                      const E_ThreadState kState);
static void _UpdateContextStatistics(S_ScheduleContext* pContext);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief Stores the pointers to the scheduling contexts. */
S_ScheduleContext** ppSchedulerContext;

/************************** Static global variables ***************************/
/** @brief Stores the last allocated process ID. */
static T_U32Atomic sLastPID;
/** @brief Stores the last allocated thread ID. */
static T_U32Atomic sLastTID;
/** @brief Stores the scheduler state. */
static bool sIsInit = false;
/** @brief Tables that contains the existing processes. */
/* TODO */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _IdleRoutine(void* pArgs)
{
  (void)pArgs;

  while (true)
  {
    InterruptRestore(1);
    CPUHalt();
  }

  /* IDLE is not allowed to return */
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "IDLE returned", false);

  return NULL;
}

static void _SchedulerThreadEntry(void)
{
  S_KernelThread* pCurrThread;

  pCurrThread = SchedulerGetCurrentThread();

  /* Set start time */
  pCurrThread->startTime     = TimeGetUptime();
  pCurrThread->executionTime = 0;

  /* Call the thread routine */
  pCurrThread->returnValue = pCurrThread->pRoutine(pCurrThread->pArgs);

  /* Call the exit function */
  _SchedulerThreadExit(pCurrThread);
}

static void _SchedulerThreadExit(S_KernelThread* pThread)
{
  S_ScheduleContext* pContext;
  uint32_t           intState;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);
  KERNEL_LOCK(pThread->lock);

  /* Set thread as zombie */
  _SetThreadToZombie(pThread);

  /* TODO: If main thread, kill all other threads and join them */

  /* Check if a thread is waiting to join */
  if (pThread->pJoiningThread != NULL)
  {
    pContext = _SelectNextContext(pThread->pJoiningThread);
    _SetThreadToReady(pContext, pThread->pJoiningThread);
  }

  /* Set end time */
  pThread->endTime = TimeGetUptime();

  KERNEL_UNLOCK(pThread->lock);

  /* Schedule, we should never come back */
  SchedulerSchedule();
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "Thread returned at exit", false);
  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static S_KernelProcess* _CreateMainProcess(void)
{
  S_KernelProcess* pMainProcess;

  pMainProcess = KMalloc(sizeof(S_KernelProcess),
                         ALIGN_ADDRESS,
                         KMALLOC_NO_FREE_POOL);
  memset(pMainProcess, 0, sizeof(S_KernelProcess));

  /* Initialize the attributes */
  pMainProcess->pid         = AtomicIncrement32(&sLastPID);
  pMainProcess->pParent     = NULL;
  pMainProcess->pChildren   = KQueueCreate();
  pMainProcess->pMainThread = NULL;
  pMainProcess->pThreads    = KQueueCreate();
  memcpy(pMainProcess->pName, MAIN_PROCESS_NAME, PROCESS_NAME_MAX_LENGTH);

  pMainProcess->pMemoryData = MemoryCreateProcessData();

  KERNEL_SPINLOCK_INIT(pMainProcess->lock);

  /* TODO: Add to process table */

  return pMainProcess;
}

static void _CreateIdleThread(S_ScheduleContext* pContext,
                              const uint32_t     kCPUId,
                              S_KernelProcess*   pMainProcess)
{
  S_KernelThread*    pIdle;
  S_KernelQueueNode* pNode;

  /* Create the thread */
  pIdle = KMalloc(sizeof(S_KernelThread), ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);
  pIdle->pThreadNode        = KQueueCreateNode(pIdle);
  pIdle->tid                = AtomicIncrement32(&sLastTID);
  pIdle->type               = THREAD_TYPE_KERNEL;
  pIdle->pArgs              = NULL;
  pIdle->pEntryPoint        = _SchedulerThreadEntry;
  pIdle->pRoutine           = _IdleRoutine;
  pIdle->stackSize          = 0;
  pIdle->stackEnd           = (uintptr_t)NULL;
  pIdle->kernelStackSize    = CPUGetStackSize();
  pIdle->kernelStackEnd     = CPUGetStackEnd(kCPUId);
  pIdle->priority           = KERNEL_LOWEST_PRIORITY;
  pIdle->currentState       = THREAD_STATE_RUNNING;
  pIdle->previousState      = THREAD_STATE_READY;
  pIdle->mappedCPU          = kCPUId;
  pIdle->pProcess           = pMainProcess;
  pIdle->isScheduled        = false;
  pIdle->pJoiningThread     = NULL;

  /* Create the VCPU after initializing the attributes */
  pIdle->pVCpu = CPUCreateVirtualCPU(pIdle);

  CPU_MASK_RESET(pIdle->affinity);
  CPU_MASK_SET(pIdle->affinity, kCPUId);

  memcpy(pIdle->pName, IDLE_THREAD_NAME, THREAD_NAME_MAX_LENGTH);

  KERNEL_SPINLOCK_INIT(pIdle->lock);

  /* Update context */
  pContext->pIdleThread     = pIdle;
  pContext->pCurrentThread = pIdle;

  /* Link to main process */
  pNode = KQueueCreateNode(pIdle);
  if (pMainProcess->pMainThread == NULL)
  {
    pMainProcess->pMainThread = pIdle;
  }
  KQueuePush(pNode, pMainProcess->pThreads);
}

static S_KernelThread* _ElectNextThread(S_ScheduleContext* pContext)
{
  uint64_t           currentTime;
  uint32_t           i;
  S_KernelThread*    pNextThread;
  S_KernelQueueNode* pNode;
  S_KernelQueueNode* pWakeupNode;

  /* Update sleeping threads */
  currentTime = TimeGetUptime();
  pNode = pContext->pSleepingList->pTail;
  while (pNode != NULL)
  {
    /* If the current time is too low to wakeup the next ordered thread, stop */
    if (pNode->priority > currentTime)
    {
      break;
    }

    /* Save the next node */
    pNode = pNode->pPrev;

    /* Wakeup the thread */
    pWakeupNode = KQueuePop(pContext->pSleepingList);
    _SetThreadToReady(pContext, pWakeupNode->pData);
  }

  /* Get the next thread */
  pNode = KQueuePop(pContext->pReadyList[pContext->highestPriority]);
  pNextThread = (S_KernelThread*)pNode->pData;

  /* Update the thread */
  pNextThread->previousState = pNextThread->currentState;
  pNextThread->currentState  = THREAD_STATE_RUNNING;

  /* Update the priority */
  for (i = pContext->highestPriority; i < KERNEL_LOWEST_PRIORITY; ++i)
  {
    if (pContext->pReadyList[i]->size != 0)
    {
      break;
    }
  }
  pContext->highestPriority = i;

  return pNextThread;
}

static void _ManageThreadState(S_KernelThread* pThread)
{
  S_ScheduleContext* pContext;
  E_ThreadState      state;

  state = pThread->currentState;
  if (state == THREAD_STATE_RUNNING)
  {
    /* Select on which CPU the thread is going to execute */
    pContext = _SelectNextContext(pThread);

    /* Set the thread to ready */
    _SetThreadToReady(pContext, pThread);
  }
}

static S_ScheduleContext* _SelectNextContext(S_KernelThread* pThread)
{
  uint64_t highestScore;
  uint32_t cpuId;
  uint32_t i;
  uint32_t cpuCount;

  highestScore = 0;
  cpuCount     = CPUGetCount();

  /* Search for the core that has the lowest score */
  cpuId = 0;
  for (i = 0; i < cpuCount; ++i)
  {
    if (CPU_MASK_GET(pThread->affinity, i) != 0 &&
        ppSchedulerContext[i]->stats.score >= highestScore)
    {
      cpuId = i;
      highestScore = ppSchedulerContext[i]->stats.score;
    }
  }

  return ppSchedulerContext[cpuId];
}

static void _SetThreadToReady(S_ScheduleContext* pContext,
                              S_KernelThread*    pThread)
{
  uint32_t cpuId;

  cpuId = CPUGetId();

  if (pThread->currentState != THREAD_STATE_READY)
  {
    pThread->previousState = pThread->currentState;
    pThread->currentState  = THREAD_STATE_READY;
  }

  /* Check if we are in the correct context */
  if (cpuId == pContext->mappedCpu)
  {
    /* Set to ready and add to the ready list */
    pThread->mappedCPU = cpuId;
    KQueuePush(pThread->pThreadNode, pContext->pReadyList[pThread->priority]);

    /* Update the priority */
    if (pContext->highestPriority > pThread->priority)
    {
      pContext->highestPriority = pThread->priority;
    }
  }
  else
  {
    /* Send the request to the desired CPU */
    _RequestThreadStateUpdate(pContext, pThread, THREAD_STATE_READY);
  }
}

static void _SetThreadToSleeping(S_ScheduleContext* pContext,
                                 S_KernelThread*    pThread)
{
  uint32_t cpuId;

  cpuId = CPUGetId();

  if (pThread->currentState != THREAD_STATE_SLEEPING)
  {
    pThread->previousState = pThread->currentState;
    pThread->currentState  = THREAD_STATE_SLEEPING;
  }

  /* Check if we are in the correct context */
  if (cpuId == pContext->mappedCpu)
  {
    /* Set to sleeping and add to the sleeping list */
    pThread->mappedCPU = cpuId;
    KQueuePushPrio(pThread->pThreadNode,
                   pContext->pSleepingList,
                   pThread->wakeupTime);
  }
  else
  {
    /* Send the request to the desired CPU */
    _RequestThreadStateUpdate(pContext, pThread, THREAD_STATE_SLEEPING);
  }
}

static void _SetThreadToJoining(S_KernelThread* pThread)
{
  if (pThread->currentState != THREAD_STATE_SLEEPING)
  {
    pThread->previousState = pThread->currentState;
    pThread->currentState  = THREAD_STATE_SLEEPING;
  }
}

static void _SetThreadToZombie(S_KernelThread* pThread)
{
  if (pThread->currentState != THREAD_STATE_ZOMBIE)
  {
    pThread->previousState = pThread->currentState;
    pThread->currentState  = THREAD_STATE_ZOMBIE;
  }
}

static void _CleanThread(S_KernelThread* pThread)
{
  S_KernelQueueNode* pNode;

  /* Free the memory used by the structures */
  KQueueDestroyNode(&pThread->pThreadNode);
  MemoryUnmapStack(pThread->kernelStackEnd,
                   pThread->kernelStackSize,
                   true,
                   pThread->pProcess);
  if (pThread->type == THREAD_TYPE_USER)
  {
    MemoryUnmapStack(pThread->stackEnd,
                     pThread->stackSize,
                     false,
                     pThread->pProcess);
  }
  CPUDestroyVirtualCPU(pThread);

  /* Unlink from process */
  KERNEL_LOCK(pThread->pProcess->lock);
  pNode = pThread->pProcess->pThreads->pHead;
  while (pNode != NULL)
  {
    if (pNode->pData == pThread)
    {
      KQueueRemove(pThread->pProcess->pThreads, pNode);
      break;
    }

    pNode = pNode->pNext;
  }
  KQueueDestroyNode(&pNode);
  KERNEL_UNLOCK(pThread->pProcess->lock);

  KFree(pThread);
}

static void _RequestThreadStateUpdate(S_ScheduleContext*  pContext,
                                      S_KernelThread*     pThread,
                                      const E_ThreadState kState)
{
  uint32_t           cpuId;
  S_SchedulerRequest request;

  cpuId = CPUGetId();

  /* Prepare the request */
  request.type = REQUEST_THREAD_STATE_UPDATE;
  request.pParams[0] = pThread;
  request.pParams[1] = (void*)kState;

  /* Send the request */
  FQueuePush(pContext->ppRequestQueues[cpuId], &request);
}

static void _HandleSchedulerRequests(S_ScheduleContext* pContext)
{
  uint32_t           cpuCount;
  uint32_t           i;
  bool               available;
  S_SchedulerRequest request;

  cpuCount = CPUGetCount();

  /* Dispatch the request from all cores */
  for (i = 0; i < cpuCount; ++i)
  {
    available = FQueuePop(pContext->ppRequestQueues[i], &request);
    if (available == true)
    {
      switch (request.type)
      {
        case REQUEST_THREAD_STATE_UPDATE:
          _DispatchThreadStateUpdateRequest(pContext,
                                            request.pParams[0],
                                            (E_ThreadState)request.pParams[1]);
          break;
        default:
          PANIC(ERR_INVALID_VALUE, MODULE_NAME, "Invalid request.", false);
      }
    }
  }
}

static void _DispatchThreadStateUpdateRequest(S_ScheduleContext*  pContext,
                                              S_KernelThread*     pThread,
                                              const E_ThreadState kState)
{
  switch (kState)
  {
    case THREAD_STATE_READY:
      _SetThreadToReady(pContext, pThread);
      break;
    case THREAD_STATE_SLEEPING:
      _SetThreadToSleeping(pContext, pThread);
      break;
    default:
      PANIC(ERR_INVALID_VALUE, MODULE_NAME, "Invalid state update.", false);
  }
}

static void _UpdateContextStatistics(S_ScheduleContext* pContext)
{
  uint64_t currentTime;
  uint64_t elapsedTime;
  uint32_t timeIdx;

  /* Get the current time */
  currentTime = TimeGetUptime();
  elapsedTime = currentTime - pContext->stats.lastTime;

  timeIdx = pContext->stats.timesIdx;

  /* Retrieve the averages */
  pContext->stats.idleTime -= pContext->stats.idleTimes[timeIdx];
  pContext->stats.totalTime -= pContext->stats.totalTimes[timeIdx];

  /* Store the time to the correct index */
  if (pContext->pCurrentThread == pContext->pIdleThread)
  {
    pContext->stats.idleTimes[timeIdx] = elapsedTime;
  }
  else
  {
    pContext->stats.idleTimes[timeIdx] = 0;
  }
  pContext->stats.totalTimes[timeIdx] = elapsedTime;

  /* Update the averages */
  pContext->stats.idleTime += pContext->stats.idleTimes[timeIdx];
  pContext->stats.totalTime += pContext->stats.totalTimes[timeIdx];

  /* Update the last recorded time */
  pContext->stats.lastTime = currentTime;

  /* Calculate the context score */
  pContext->stats.score = pContext->stats.idleTime  * 10000ULL /
                          (pContext->stats.totalTime + 1);

  /* Update the elapsed time for the thread */
  pContext->pCurrentThread->executionTime += elapsedTime;

  /* Update the index */
  pContext->stats.timesIdx = (timeIdx + 1) % CPU_LOAD_TICK_WINDOW;

}

void SchedulerInit(void)
{
  uint32_t         i;
  uint32_t         j;
  uint32_t         cpuCount;
  S_KernelProcess* pMainProcess;
  E_Return         errorCode;
  uint32_t         schedInt;
  S_FastQueue*     pQueue;
  uint64_t         currentTime;

  /* Initialize parameters */
  cpuCount = CPUGetCount();
  sLastPID = 0;
  sLastTID = 0;
  currentTime = TimeGetUptime();

  /* Create the Main Kernel Process */
  pMainProcess = _CreateMainProcess();

  /* Create the scheduling contexts. */
  ppSchedulerContext = KMalloc(sizeof(S_ScheduleContext*) * cpuCount,
                              ALIGN_ADDRESS,
                              KMALLOC_NO_FREE_POOL);
  /* Initialize the contexts */
  for (i = 0; i < cpuCount; ++i)
  {
    ppSchedulerContext[i] = KMalloc(sizeof(S_ScheduleContext),
                                    ALIGN_ADDRESS,
                                    KMALLOC_NO_FREE_POOL);
    /* Create the ready list queues */
    for (j = 0; j <= KERNEL_LOWEST_PRIORITY; ++j)
    {
      ppSchedulerContext[i]->pReadyList[j] = KQueueCreate();
    }

    /* Create the sleeping list queue */
    ppSchedulerContext[i]->pSleepingList = KQueueCreate();

    ppSchedulerContext[i]->highestPriority = KERNEL_LOWEST_PRIORITY;
    ppSchedulerContext[i]->mappedCpu       = i;
    ppSchedulerContext[i]->stats.score     = 0;

    /* Create the requests queues */
    ppSchedulerContext[i]->ppRequestQueues = KMalloc(sizeof(S_FastQueue*) *
                                                      cpuCount,
                                                     ALIGN_ADDRESS,
                                                     KMALLOC_NO_FREE_POOL);
    for (j = 0; j < cpuCount; ++j)
    {
      pQueue = FQueueCreate(SCHEDULER_REQUEST_QUEUE_SIZE,
                            sizeof(S_SchedulerRequest));
      ppSchedulerContext[i]->ppRequestQueues[j] = pQueue;
    }

    /* Initialize the statistics */
    memset(&ppSchedulerContext[i]->stats,
           0,
           sizeof(S_ScheduleContextStatistics));
    ppSchedulerContext[i]->stats.idleTime = currentTime;
    ppSchedulerContext[i]->stats.idleTimes[0] = currentTime;
    ppSchedulerContext[i]->stats.totalTime = currentTime;
    ppSchedulerContext[i]->stats.totalTimes[0] = currentTime;
    ppSchedulerContext[i]->stats.timesIdx = 1;
    ppSchedulerContext[i]->stats.lastTime = currentTime;

    /* Create the Idle thread for the context */
    _CreateIdleThread(ppSchedulerContext[i], i, pMainProcess);
  }

  /* Register the scheduler interrupt  */
  schedInt  = CPUGetInterruptConfig()->schedulerInterruptLine;
  errorCode = InterruptRegister(schedInt, SchedulerSchedule, false);
  SCHED_ASSERT(errorCode == NO_ERROR,
               "Failed to register scheduler interrupt",
               errorCode);

  sIsInit = true;

  TEST_POINT_FUNCTION_CALL(SchedulerTest, TEST_SCHEDULER_ENABLED);
}

bool SchedulerSchedule(void)
{
  uint32_t           cpuId;
  uint32_t           intState;
  S_ScheduleContext* pCPUContext;
  S_KernelThread*    pCurrThread;
  S_KernelThread*    pOldThread;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  /* Get the scheduling context for the current CPU */
  cpuId       = CPUGetId();
  pCPUContext = ppSchedulerContext[cpuId];
  pCurrThread = pCPUContext->pCurrentThread;
  pOldThread  = pCurrThread;

  /* Update the context statistics */
  _UpdateContextStatistics(pCPUContext);

  /* Handle the requests for the current context */
  _HandleSchedulerRequests(pCPUContext);

  /* If the process is running */
  if (pCurrThread->currentState == THREAD_STATE_RUNNING)
  {
    /* Manage the next state for the current thread */
    _ManageThreadState(pCurrThread);
  }

  /* Elect the new thread */
  pCurrThread = _ElectNextThread(pCPUContext);
  pCPUContext->pCurrentThread = pCurrThread;

  /* Update the memory configuration */
  CPUUpdateMemoryConfig(pCurrThread);

  /* Notify that the old thread is ready to be scheduled */
  pOldThread->isScheduled = false;

  /* Wait for the thread to be schedulable */
  while (pCurrThread->isScheduled == true) {}
  CPUMemoryFenceAcquire();
  pCurrThread->isScheduled = true;

  /* Restore the context, we will never return from this call */
  CPURestoreContext(pCurrThread);

  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "Scheduler returned after context restore.",
        false);

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
  return false;
}

S_KernelThread* SchedulerGetCurrentThread(void)
{
  uint32_t        cpuId;
  uint32_t        intState;
  S_KernelThread *pThread;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  cpuId = CPUGetId();
  pThread = ppSchedulerContext[cpuId]->pCurrentThread;

  KERNEL_EXIT_CRITICAL_LOCAL(intState);

  return pThread;
}

S_KernelProcess* SchedulerGetCurrentProcess(void)
{
  uint32_t         cpuId;
  uint32_t         intState;
  S_KernelProcess *pProcess;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  cpuId = CPUGetId();
  pProcess = ppSchedulerContext[cpuId]->pCurrentThread->pProcess;

  KERNEL_EXIT_CRITICAL_LOCAL(intState);

  return pProcess;
}

void SchedulerSetCurrentThreadErrored(void)
{
  S_KernelThread* pThread;
  uint32_t        cpuId;
  uint32_t        intState;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  cpuId = CPUGetId();

  pThread = ppSchedulerContext[cpuId]->pCurrentThread;

  /* TODO: Manage error registration */
  pThread->returnValue = NULL;
  _SchedulerThreadExit(pThread);

  KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

bool SchedulerIsInitialized(void)
{
  return sIsInit;
}

E_Return CreateThread(S_KernelThread**      ppThread,
                      const bool            kIsKernel,
                      const uint8_t         kPriority,
                      const char            kName[THREAD_NAME_MAX_LENGTH],
                      const size_t          kStackSize,
                      const S_CPUMask       kMappedCPUs,
                      const T_ThreadRoutine kRoutine,
                      void*                 args)
{
  S_KernelThread*    pThread;
  S_KernelQueueNode* pNode;
  S_KernelProcess*   pCurrentProcess;
  E_ThreadType       type;
  S_ScheduleContext* pContext;
  E_Return           error;
  bool               validCPUSet;

  validCPUSet = CPUValidateCPUMask(&kMappedCPUs);

  /* Perform checks */
  if (validCPUSet == true &&
      ppThread != NULL &&
      kPriority <= KERNEL_LOWEST_PRIORITY &&
      kName != NULL &&
      kStackSize != 0 &&
      kRoutine != NULL)
  {

    if (kIsKernel == true)
    {
      type = THREAD_TYPE_KERNEL;
    }
    else
    {
      (void)kStackSize;
      type = THREAD_TYPE_USER;
      PANIC(ERR_NOT_SUPPORTED,
            MODULE_NAME,
            "User threads are not supported currently.",
            false);
    }

    pCurrentProcess = SchedulerGetCurrentProcess();

    /* Create the thread */
    pThread = KMalloc(sizeof(S_KernelThread), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
    pThread->pThreadNode     = KQueueCreateNode(pThread);
    pThread->tid             = AtomicIncrement32(&sLastTID);
    pThread->type            = type;
    pThread->pArgs           = args;
    pThread->pEntryPoint     = _SchedulerThreadEntry;
    pThread->pRoutine        = kRoutine;
    pThread->stackSize       = 0;
    pThread->stackEnd        = (uintptr_t)NULL;
    pThread->kernelStackSize = CPUGetStackSize();
    pThread->kernelStackEnd  = MemoryMapStack(pThread->kernelStackSize,
                                              kIsKernel,
                                              pCurrentProcess);
    pThread->priority        = kPriority;
    pThread->currentState    = THREAD_STATE_READY;
    pThread->previousState   = THREAD_STATE_READY;
    pThread->mappedCPU       = 0xFFFFFFFF;
    pThread->pProcess        = pCurrentProcess;
    pThread->isScheduled     = false;
    pThread->pJoiningThread  = NULL;

    /* Create the VCPU after initializing the attributes */
    pThread->pVCpu = CPUCreateVirtualCPU(pThread);

    CPU_MASK_RESET(pThread->affinity);
    CPU_MASK_COPY(pThread->affinity, kMappedCPUs);

    memcpy(pThread->pName, kName, THREAD_NAME_MAX_LENGTH);
    pThread->pName[THREAD_NAME_MAX_LENGTH - 1] = 0;

    KERNEL_SPINLOCK_INIT(pThread->lock);

    /* Link to main process */
    pNode = KQueueCreateNode(pThread);

    KERNEL_LOCK(pCurrentProcess->lock);
    if (pCurrentProcess->pMainThread == NULL)
    {
      pCurrentProcess->pMainThread = pThread;
    }
    KQueuePush(pNode, pCurrentProcess->pThreads);
    KERNEL_UNLOCK(pCurrentProcess->lock);

    /* Put the thread in the scheduler context */
    pContext = _SelectNextContext(pThread);
    _SetThreadToReady(pContext, pThread);

    /* Set return values */
    *ppThread = pThread;
    error = NO_ERROR;
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

E_Return JoinThread(S_KernelThread* pThread,
                    void**          ppReturnValue)
{
  S_KernelThread* pCurrentThread;
  E_Return        error;
  uint32_t        intState;

  pCurrentThread = SchedulerGetCurrentThread();

  /* Ensure we are allowed to join the thread */
  if (pThread->pProcess == pCurrentThread->pProcess &&
      pThread != pCurrentThread &&
      pThread != pThread->pProcess->pMainThread)
  {
    KERNEL_LOCK(pThread->lock);
    /* Check if a thread is already trying to join */
    if (pThread->pJoiningThread == NULL)
    {
      /* Check if the thread is not a zombie, block */
      if (pThread->currentState != THREAD_STATE_ZOMBIE)
      {
        KERNEL_ENTER_CRITICAL_LOCAL(intState);
        _SetThreadToJoining(pCurrentThread);
        pThread->pJoiningThread = pCurrentThread;
        KERNEL_UNLOCK(pThread->lock);
        CPUSaveContextAndSchedule(pCurrentThread->pVCpu);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        KERNEL_LOCK(pThread->lock);
      }
      /* Ensure the thread is completely done */
      while (pThread->isScheduled == true) {}

      /* Setup the return information */
      *ppReturnValue = pThread->returnValue;

      /* Clean the thread */
      _CleanThread(pThread);

      error = NO_ERROR;
    }
    else
    {
      error = ERR_UNAUTHORIZED_ACTION;
    }
    KERNEL_UNLOCK(pThread->lock);
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }

  return error;
}

E_Return SleepNs(const uint64_t timeNs)
{
  E_Return           error;
  uint64_t           currentTime;
  uint64_t           wakeupTime;
  uint32_t           intState;
  S_KernelThread*    pThread;
  S_ScheduleContext* pNewContext;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);

  /* Calculate the wakeup time and check for rollback */
  currentTime = TimeGetUptime();
  wakeupTime  = currentTime + timeNs;
  if(wakeupTime >= currentTime)
  {
    pThread = SchedulerGetCurrentThread();

    /* Get the next context */
    pNewContext = _SelectNextContext(pThread);

    /* Update the thread status */
    pThread->wakeupTime = wakeupTime;
    _SetThreadToSleeping(pNewContext, pThread);

    /* Schedule the thread*/
    CPUSaveContextAndSchedule(pThread->pVCpu);

    error = NO_ERROR;
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  KERNEL_EXIT_CRITICAL_LOCAL(intState);

  return error;
}

const S_ScheduleContextStatistics* SchedulerGetStatistics(const uint32_t kCPU)
{
  S_ScheduleContextStatistics* pData;
  pData = &ppSchedulerContext[kCPU]->stats;
  return pData;
}

/************************************ EOF *************************************/