/*******************************************************************************
 * @file KernelWorker.c
 *
 * @see KernelWorker.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 15/10/2026
 *
 * @version 1.0
 *
 * @brief Kernel thread worker manager.
 *
 * @details Kernel thread worker manager. Workers are used to execute tasks in
 * the background.
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
#include <stdlib.h>
#include <Scheduler.h>
#include <KernelQueue.h>
#include <KernelError.h>
#include <KernelSemaphore.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <KernelWorker.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Module name for the kernel worker. */
#define MODULE_NAME "KWORKER"
/** @brief Priority for the kernel worker threads. */
#define KWORKER_THREAD_PRIORITY 1
/** @brief Stack size for the kernel worker threads. */
#define KWORKER_THREAD_STACK_SIZE 4096

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
typedef struct S_KernelWorkerTask
{
  /** @brief Pointer to the task function. */
  T_KernelWorkerTask pTask;
  /** @brief Pointer to the argument for the task function. */
  void* pArg;
  /** @brief Pointer to the completion semaphore for the task. */
  S_KernelSemaphore* pCompletionSemaphore;
} S_KernelWorkerTask;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the kernel worker to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the kernel worker to ensure correctness of
 * execution. Due to the critical nature of the kernel worker, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define KWORKER_ASSERT(COND, MSG, ERROR, IS_PROCESS) { \
  if ((COND) == false)                                 \
  {                                                    \
    PANIC(ERROR, MODULE_NAME, MSG, false, IS_PROCESS); \
  }                                                    \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Kernel worker thread routine.
 *
 * @details Kernel worker thread routine. This function is executed in a
 * separate thread and handles the execution of tasks submitted to the kernel
 * worker.
 *
 * @param[in] pArgs Pointer to the arguments passed to the kernel worker thread,
 * usually NULL.
 *
 * @return This function does not return. It runs indefinitely until the kernel
 * worker thread is terminated.
 */
static void* _KernelWorkerThreadRoutine(void* pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Pointer to the task queue for the kernel worker. */
static S_KernelQueue* spTaskQueue;
/** @brief Task semaphore for the kernel worker. */
static S_KernelSemaphore sTaskSemaphore;
/** @brief Task queue lock */
S_KernelSpinlock sTaskQueueLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _KernelWorkerThreadRoutine(void* pArgs)
{
  E_Return            retCode;
  S_KernelQueueNode*  pNode;
  S_KernelWorkerTask* pTask;

  (void)pArgs;

  while (true)
  {
    /* Wait for a task to be submitted */
    retCode = KernelSemaphoreWait(&sTaskSemaphore);
    KWORKER_ASSERT(retCode == NO_ERROR,
                   "Failed to wait for task semaphore.",
                   retCode,
                   false);

    /* Get the task */
    KERNEL_LOCK(sTaskQueueLock);
    pNode = KQueuePop(spTaskQueue);
    KERNEL_UNLOCK(sTaskQueueLock);
    KWORKER_ASSERT(pNode != NULL && pNode->pData != NULL,
                   "Failed to dequeue task from the task queue.",
                   ERR_INVALID_VALUE,
                   false);

    /* Execute the task */
    pTask = (S_KernelWorkerTask*)pNode->pData;
    pTask->pTask(pTask->pArg);

    /* Signal task completion */
    if (pTask->pCompletionSemaphore != NULL)
    {
      retCode = KernelSemaphorePost(pTask->pCompletionSemaphore);
      KWORKER_ASSERT(retCode == NO_ERROR,
                     "Failed to signal task completion semaphore.",
                     retCode,
                     false);
    }

    /* Free the task structure */
    KFree(pTask);
    KQueueDestroyNode(&pNode);
  }

  /* Kernel worker thread is not allowed to return */
  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "Kernel worker thread returned",
        false,
        false);

  return NULL;
}

void KWorkerInit(void)
{
  E_Return        retCode;
  uint32_t        cpuCount;
  uint32_t        i;
  S_KernelThread* pThread;
  char            name[THREAD_NAME_MAX_LENGTH];
  S_CPUMask       cpuMask;

  /* Spawn kernel worker threads */
  cpuCount = CPUGetCount();
  for (i = 0; i < cpuCount; ++i)
  {
    snprintf(name, THREAD_NAME_MAX_LENGTH, "kworker-%u", i);
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i);
    /* Create the kernel worker thread */
    retCode = CreateThread(&pThread,
                           true,
                           KWORKER_THREAD_PRIORITY,
                           name,
                           KWORKER_THREAD_STACK_SIZE,
                           cpuMask,
                           _KernelWorkerThreadRoutine,
                           NULL,
                           NULL);
    KWORKER_ASSERT(retCode == NO_ERROR,
                   "Failed to create kernel worker thread.",
                   retCode,
                   false);
  }

  /* Initialize the task queue */
  spTaskQueue = KQueueCreate(NULL);

  /* Initialize the task semaphore */
  retCode = KernelSemaphoreInit(&sTaskSemaphore,
                                KSEMAPHORE_FLAG_QUEUING_FIFO,
                                0);
  KWORKER_ASSERT(retCode == NO_ERROR,
                 "Failed to initialize task semaphore.",
                 retCode,
                 false);
}

E_Return KWorkerSubmitTask(T_KernelWorkerTask task,
                           void*              pArg,
                           S_KernelSemaphore* pCompletionSemaphore)
{
  S_KernelWorkerTask* pNewTask;
  S_KernelQueueNode*  pNode;
  E_Return            retCode;

  /* Create the task structure */
  pNewTask = KMalloc(sizeof(S_KernelWorkerTask), KMALLOC_FREE_POOL);
  pNewTask->pTask = task;
  pNewTask->pArg = pArg;
  pNewTask->pCompletionSemaphore = pCompletionSemaphore;

  /* Enqueue the task */
  pNode = KQueueCreateNode(pNewTask, NULL);
  KERNEL_LOCK(sTaskQueueLock);
  KQueuePush(pNode, spTaskQueue);
  KERNEL_UNLOCK(sTaskQueueLock);

  /* Signal the task semaphore */
  retCode = KernelSemaphorePost(&sTaskSemaphore);
  KWORKER_ASSERT(retCode == NO_ERROR,
                 "Failed to signal task semaphore.",
                 retCode,
                 false);

  return NO_ERROR;
}

/************************************ EOF *************************************/