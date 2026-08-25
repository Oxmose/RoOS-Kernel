/*******************************************************************************
 * @file KernelSemaphore.c
 *
 * @see KernelSemaphore.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/08/2026
 *
 * @version 4.0
 *
 * @brief Kernel semaphore synchronization primitive.
 *
 * @details Kernel's semaphore API. This module implements the semaphore
 * management. The semaphore are used to synchronyse the threads. The semaphore
 * waiting list is a FIFO with no regards of the waiting threads priority.
 *
 * @warning Semaphores can only be used when the current system is running and
 * the scheduler initialized.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <stdint.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <Scheduler.h>
#include <KernelQueue.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <KernelSemaphore.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "KSEMAPHORE"

/** @brief Defines the maximal semaphore wake value */
#define SEMAPHORE_MAX_LEVEL 0x7FFFFFFF

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
E_Return KernelSemaphoreInit(S_KernelSemaphore* pSemaphore,
                             const uint32_t     kFlags,
                             const int32_t      kInitialValue)
{
  E_Return error;

  if ((((kFlags & (KSEMAPHORE_FLAG_QUEUING_FIFO | KSEMAPHORE_FLAG_QUEUING_PRIO)) ==
       KSEMAPHORE_FLAG_QUEUING_FIFO) ||
      ((kFlags & (KSEMAPHORE_FLAG_QUEUING_FIFO | KSEMAPHORE_FLAG_QUEUING_PRIO)) ==
       KSEMAPHORE_FLAG_QUEUING_PRIO)) &&
       kInitialValue >= 0 &&
       kInitialValue <= SEMAPHORE_MAX_LEVEL)
  {
    if ((kFlags & KSEMAPHORE_FLAG_BINARY) == KSEMAPHORE_FLAG_BINARY)
    {
      pSemaphore->lockState = kInitialValue == 0 ? 0 : 1;
    }
    else
    {
      pSemaphore->lockState = kInitialValue;
    }

    pSemaphore->pWaitingList = KQueueCreate(NULL);
    if (pSemaphore->pWaitingList != NULL)
    {
      pSemaphore->flags = kFlags;
      KERNEL_SPINLOCK_INIT(pSemaphore->lock);

      error = NO_ERROR;
    }
    else
    {
      error = ERR_NO_MEMORY;
    }
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

E_Return KernelSemaphoreDestroy(S_KernelSemaphore* pSemaphore)
{
  E_Return error;

  KERNEL_LOCK(pSemaphore->lock);
  if (pSemaphore->lockState != 0)
  {
    /* Destroy the waiting queue */
    KQueueDestroy(&pSemaphore->pWaitingList);
    error = NO_ERROR;
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }
  KERNEL_UNLOCK(pSemaphore->lock);

  return error;
}

E_Return KernelSemaphoreWait(S_KernelSemaphore* pSemaphore)
{
  E_Return        error;
  S_KernelThread* pCurThread;
  uint32_t        intState;

  KERNEL_ENTER_CRITICAL_LOCAL(intState);
  KERNEL_LOCK(pSemaphore->lock);

  pCurThread = SchedulerGetCurrentThread();

  if (pSemaphore->lockState == 0)
  {
    /* Set thread to waiting */
    SchedulerSetCurrentThreadToWaiting();

    /* Add to list */
    if ((pSemaphore->flags & KSEMAPHORE_FLAG_QUEUING_FIFO) ==
        KSEMAPHORE_FLAG_QUEUING_FIFO)
    {
      KQueuePush(pCurThread->pThreadNode, pSemaphore->pWaitingList);
    }
    else
    {
      KQueuePushPrio(pCurThread->pThreadNode,
                     pSemaphore->pWaitingList,
                     pCurThread->priority);
    }

    /* Unlock semaphore and schedule */
    KERNEL_UNLOCK(pSemaphore->lock);
    CPUSaveContextAndSchedule(pCurThread->pVCpu);

  }
  else
  {
    /* Acquire one value */
    --pSemaphore->lockState;

    KERNEL_UNLOCK(pSemaphore->lock);
  }
  error = NO_ERROR;

  KERNEL_EXIT_CRITICAL_LOCAL(intState);

  return error;
}

E_Return KernelSemaphorePost(S_KernelSemaphore* pSemaphore)
{
  S_KernelQueueNode* pNode;
  S_KernelThread*    pReleasedThread;
  E_Return           error;

  KERNEL_LOCK(pSemaphore->lock);

  if (pSemaphore->lockState == 0)
  {
    /* Check if there are waiting threads */
    pNode = KQueuePop(pSemaphore->pWaitingList);
    if (pNode != NULL)
    {
      /* Get the thread */
      pReleasedThread = pNode->pData;

      /* Release the thread */
      SchedulerSetThreadToReady(pReleasedThread);
    }
    else
    {
      /* Simply release the semaphore */
      pSemaphore->lockState = 1;
    }

    error = NO_ERROR;
  }
  else if (pSemaphore->lockState < SEMAPHORE_MAX_LEVEL)
  {
    if ((pSemaphore->flags & KSEMAPHORE_FLAG_BINARY) == KSEMAPHORE_FLAG_BINARY)
    {
      pSemaphore->lockState = 1;
    }
    else
    {
      ++pSemaphore->lockState;
    }
    error = NO_ERROR;
  }
  else
  {
    error = ERR_EXCEEDED_LIMIT;
  }

  KERNEL_UNLOCK(pSemaphore->lock);

  return error;
}

E_Return KernelSemaphoreTryWait(S_KernelSemaphore* pSemaphore, int32_t* pLockState)
{
  E_Return error;

  KERNEL_LOCK(pSemaphore->lock);

  *pLockState = pSemaphore->lockState;
  if (pSemaphore->lockState > 0)
  {
    /* Acquire one value */
    --pSemaphore->lockState;
    error = NO_ERROR;
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }

  KERNEL_UNLOCK(pSemaphore->lock);

  return error;
}

/************************************ EOF *************************************/