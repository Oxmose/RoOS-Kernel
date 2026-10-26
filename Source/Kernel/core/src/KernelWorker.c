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
#include <FastQueue.h>
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
/** @brief Size of the task queue for the kernel worker. */
#define KWORKER_TASK_QUEUE_SIZE 511

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
static S_FastQueue* spTaskQueue;
/** @brief Task semaphore for the kernel worker. */
static S_KernelSemaphore sTaskSemaphore;
/** @brief Task queue lock */
static S_KernelSpinlock sTaskQueueLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _KernelWorkerThreadRoutine(void* pArgs)
{
  E_Return           retCode;
  S_KernelWorkerTask task;
  bool               taskAvailable;

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
    taskAvailable = FQueuePop(spTaskQueue, &task);
    KERNEL_UNLOCK(sTaskQueueLock);
    KWORKER_ASSERT(taskAvailable == true,
                   "Failed to dequeue task from the task queue.",
                   ERR_INVALID_VALUE,
                   false);

    /* Execute the task */
    task.pTask(task.pArg);

    /* Signal task completion */
    if (task.pCompletionSemaphore != NULL)
    {
      retCode = KernelSemaphorePost(task.pCompletionSemaphore);
      KWORKER_ASSERT(retCode == NO_ERROR,
                     "Failed to signal task completion semaphore.",
                     retCode,
                     false);
    }
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
  spTaskQueue = FQueueCreate(KWORKER_TASK_QUEUE_SIZE,
                             sizeof(S_KernelWorkerTask));

  /* Initialize the task semaphore */
  retCode = KernelSemaphoreInit(&sTaskSemaphore,
                                KSEMAPHORE_FLAG_QUEUING_FIFO,
                                0);
  KWORKER_ASSERT(retCode == NO_ERROR,
                 "Failed to initialize task semaphore.",
                 retCode,
                 false);
}

void KWorkerSubmitTask(T_KernelWorkerTask task,
                       void*              pArg,
                       S_KernelSemaphore* pCompletionSemaphore)
{
  S_KernelWorkerTask newTask;
  E_Return           retCode;

  /* Create the task structure */
  newTask.pTask = task;
  newTask.pArg = pArg;
  newTask.pCompletionSemaphore = pCompletionSemaphore;

  /* Enqueue the task */
  KERNEL_LOCK(sTaskQueueLock);
  FQueuePush(spTaskQueue, &newTask);
  KERNEL_UNLOCK(sTaskQueueLock);

  /* Signal the task semaphore */
  retCode = KernelSemaphorePost(&sTaskSemaphore);
  KWORKER_ASSERT(retCode == NO_ERROR,
                 "Failed to signal task semaphore.",
                 retCode,
                 false);
}

/************************************ EOF *************************************/