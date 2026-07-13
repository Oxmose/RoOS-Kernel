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
#include <stdint.h>
#include <Memory.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <TimerManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* None TODO */

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

  /** @brief Stores the number of threads present in the context. */
  uint32_t threadCount;
} S_ScheduleContext;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

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
 * @brief Creates the idle thread for a core.
 *
 * @details Creates the idle thread for a core. The idle threds are part of
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
S_KernelThread* _ElectNextThread(S_ScheduleContext* pContext);

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
static U32Atomic sLastPID;
/** @brief Stores the last allocated thread ID. */
static U32Atomic sLastTID;
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

  /* If we return better go away and cry in a corner */
  SCHED_ASSERT(false, "IDLE returned", ERR_UNAUTHORIZED_ACTION);

  return NULL;
}

static void _SchedulerThreadEntry(void)
{
  void*           pThreadReturnValue;
  S_KernelThread* pCurrThread;

  pCurrThread = SchedulerGetCurrentThread();

  /* Set start time */
  pCurrThread->startTime = TimeGetUptime();

  /* Call the thread routine */
  pThreadReturnValue = pCurrThread->pRoutine(pCurrThread->pArgs);

  /* Set end time */
  pCurrThread->endTime = TimeGetUptime();

  /* Call the exit function TODO */
  (void)pThreadReturnValue;
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
  pIdle->pThreadNode        = KQueueCreateNode(pIdle);
  pIdle->mappedCPU          = kCPUId;
  pIdle->preemptionDisabled = false;
  pIdle->pProcess           = pMainProcess;

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


static void _SetThreadToReady(S_ScheduleContext* pContext,
                              S_KernelThread*    pThread)
{
  /* Set to ready and add to the ready list */
  pThread->previousState = pThread->currentState;
  pThread->currentState  = THREAD_STATE_READY;
  pThread->mappedCPU     = CPUGetId();
  KQueuePush(pThread->pThreadNode, pContext->pReadyList[pThread->priority]);

  /* Update the priority */
  if (pContext->highestPriority > pThread->priority)
  {
    pContext->highestPriority = pThread->priority;
  }

  /* Update the number of thread in the context */
  ++pContext->threadCount;
}

S_KernelThread* _ElectNextThread(S_ScheduleContext* pContext)
{
  uint32_t           i;
  S_KernelThread*    pNextThread;
  S_KernelQueueNode* pNode;

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

  /* Update the number of thread in the context */
  --pContext->threadCount;

  return pNextThread;
}

void SchedulerInit(void)
{
  uint32_t         i;
  uint32_t         j;
  uint32_t         cpuCount;
  S_KernelProcess* pMainProcess;
  E_Return         errorCode;
  uint32_t         schedInt;

  /* Initialize parameters */
  cpuCount = CPUGetCount();
  sLastPID = 0;
  sLastTID = 0;

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
    ppSchedulerContext[i]->threadCount     = 1;

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
}

bool SchedulerSchedule(void)
{
  uint32_t           cpuId;
  S_ScheduleContext* pCPUContext;
  S_KernelThread*    pCurrThread;

  /* Get the scheduling context for the current CPU */
  cpuId       = CPUGetId();
  pCPUContext = ppSchedulerContext[cpuId];
  pCurrThread = pCPUContext->pCurrentThread;

  /* If the process is running */
  if (pCurrThread->currentState == THREAD_STATE_RUNNING)
  {
    /* Check if a thread with higher of equal priority can run */
    if (pCurrThread->preemptionDisabled == false &&
        pCPUContext->highestPriority <= pCurrThread->priority)
    {
      /* Release the current thread */
      _SetThreadToReady(pCPUContext, pCurrThread);

      /* Elect the new thread */
      pCurrThread = _ElectNextThread(pCPUContext);

      /* Save the new current thread */
      pCPUContext->pCurrentThread = pCurrThread;
    }
  }

  /* Update the memory configuration */
  CPUUpdateMemoryConfig(pCurrThread);

  /* Restore the context, we will never return from this call */
  CPURestoreContext(pCurrThread);

  return false;
}

S_KernelThread* SchedulerGetCurrentThread(void)
{
  uint32_t cpuId;

  cpuId = CPUGetId();
  return ppSchedulerContext[cpuId]->pCurrentThread;
}

void SchedulerSetThreadErrored(S_KernelThread* pThread)
{
  pThread->previousState = pThread->currentState;
  pThread->currentState  = THREAD_STATE_ERRORED;
}

bool SchedulerIsInitialized(void)
{
  return sIsInit;
}
/************************************ EOF *************************************/