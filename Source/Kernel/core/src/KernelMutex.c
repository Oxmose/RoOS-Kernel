/*******************************************************************************
 * @file KernelMutex.c
 *
 * @see KernelMutex.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 07/08/2026
 *
 * @version 4.0
 *
 * @brief Kernel mutex synchronization primitive.
 *
 * @details Kernel mutex synchronization primitive implementation. Avoids
 * priority inversion by allowing the user to set a priority to the mutex, then
 * all threads that acquire this mutex will see their priority elevated to the
 * mutex's priority level.
 *
 * @warning Mutex can only be used when the current system is running and the
 * scheduler initialized.
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
#include <KernelMutex.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "KMUTEX"

/** @brief Defines the maximum recursiveness level of a mutex */
#define MUTEX_MAX_RECURSIVENESS (UINT32_MAX)

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
E_Return KernelMutexInit(S_KernelMutex* pMutex, const uint32_t kFlags)
{
  E_Return error;
  uint32_t priority;

  if (((kFlags & (KMUTEX_FLAG_QUEUING_FIFO | KMUTEX_FLAG_QUEUING_PRIO)) ==
       KMUTEX_FLAG_QUEUING_FIFO) ||
      ((kFlags & (KMUTEX_FLAG_QUEUING_FIFO | KMUTEX_FLAG_QUEUING_PRIO)) ==
       KMUTEX_FLAG_QUEUING_PRIO))
  {
    priority = (kFlags >> 16);
    if (((kFlags & KMUTEX_FLAG_PRIO_ELEVATION) == KMUTEX_FLAG_PRIO_ELEVATION &&
        priority <= KERNEL_LOWEST_PRIORITY) ||
      ((kFlags & KMUTEX_FLAG_PRIO_ELEVATION) == 0))
    {
      pMutex->pWaitingList = KQueueCreate(NULL);
      if (pMutex->pWaitingList != NULL)
      {
        pMutex->flags            = kFlags;
        pMutex->elevatedPriority = priority;
        pMutex->level            = 0;
        pMutex->lockState        = 1;
        pMutex->pAcquiredThread  = NULL;
        SPINLOCK_INIT(pMutex->lock);

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
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

E_Return KernelMutexDestroy(S_KernelMutex* pMutex)
{
  E_Return error;

  SpinlockAcquire(&pMutex->lock);
  if (pMutex->lockState != 0)
  {
    /* Destroy the waiting queue */
    KQueueDestroy(&pMutex->pWaitingList);
    error = NO_ERROR;
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }
  SpinlockRelease(&pMutex->lock);

  return error;
}

E_Return KernelMutexLock(S_KernelMutex* pMutex)
{
  E_Return        error;
  S_KernelThread* pCurThread;
  uint32_t        intState;

  SpinlockAcquire(&pMutex->lock);

  pCurThread = GetCurrentThread();

  if (pMutex->lockState == 0)
  {
    if ((pMutex->flags & KMUTEX_FLAG_RECURSIVE) != KMUTEX_FLAG_RECURSIVE ||
       pCurThread != pMutex->pAcquiredThread)
    {
      KERNEL_ENTER_CRITICAL_LOCAL(intState);
      do
      {

        /* Add to list */
        if ((pMutex->flags & KMUTEX_FLAG_QUEUING_FIFO) ==
            KMUTEX_FLAG_QUEUING_FIFO)
        {
          SetCurrentThreadToWaiting(pMutex->pWaitingList, false);
        }
        else
        {
          SetCurrentThreadToWaitingWithPriority(pMutex->pWaitingList,
                                                pCurThread->priority,
                                                false);
        }



        /* Unlock mutex and schedule */
        SpinlockRelease(&pMutex->lock);

        CPUSaveContextAndSchedule(pCurThread->pVCpu);

        /* Check that we got the mutex */
        SpinlockAcquire(&pMutex->lock);

      } while (pCurThread != pMutex->pAcquiredThread);

      KERNEL_EXIT_CRITICAL_LOCAL(intState);
      error = NO_ERROR;
    }
    else
    {
      /* If the mutex is recursive, allow the lock */
      if (pMutex->level < MUTEX_MAX_RECURSIVENESS)
      {
        ++pMutex->level;

        error = NO_ERROR;
      }
      else
      {
        error = ERR_EXCEEDED_LIMIT;
      }
    }
  }
  else
  {
    /* Acquire one value */
    pMutex->lockState              = 0;
    pMutex->level                  = 1;
    pMutex->pAcquiredThread        = pCurThread;
    pMutex->acquiredThreadPriority = pCurThread->priority;

    if ((pMutex->flags & KMUTEX_FLAG_PRIO_ELEVATION) ==
        KMUTEX_FLAG_PRIO_ELEVATION)
    {
      /* Update the process priority */
      SetThreadPriority(pCurThread, pMutex->elevatedPriority);
    }

    error = NO_ERROR;
  }

  SpinlockRelease(&pMutex->lock);

  return error;
}

E_Return KernelMutexUnlock(S_KernelMutex* pMutex)
{
  S_KernelThread*    pCurThread;
  S_KernelThread*    pReleasedThread;
  S_KernelQueueNode* pNode;
  E_Return           error;

  pCurThread = GetCurrentThread();

  SpinlockAcquire(&pMutex->lock);

  /* Only the owner can unlock the mutex */
  if (pCurThread == pMutex->pAcquiredThread)
  {
    if ((pMutex->flags & KMUTEX_FLAG_RECURSIVE) != KMUTEX_FLAG_RECURSIVE ||
        pMutex->level == 1)
    {
      if ((pMutex->flags & KMUTEX_FLAG_PRIO_ELEVATION) ==
          KMUTEX_FLAG_PRIO_ELEVATION)
      {
        /* Update the process priority */
        SetThreadPriority(pCurThread, pMutex->acquiredThreadPriority);
      }

      /* Check if there are waiting threads */
      pNode = KQueuePop(pMutex->pWaitingList);
      if (pNode != NULL)
      {
        /* Get the thread */
        pReleasedThread                = pNode->pData;
        pMutex->pAcquiredThread        = pReleasedThread;
        pMutex->acquiredThreadPriority = pReleasedThread->priority;

        if ((pMutex->flags & KMUTEX_FLAG_PRIO_ELEVATION) ==
             KMUTEX_FLAG_PRIO_ELEVATION)
        {
          /* Update the process priority */
          SetThreadPriority(pReleasedThread, pMutex->elevatedPriority);
        }

        /* Release the thread */
        SetThreadToReady(pReleasedThread);
      }
      else
      {
        /* Simply release the mutex */
        pMutex->pAcquiredThread = NULL;
        pMutex->lockState       = 1;
        pMutex->level           = 0;
      }
    }
    else
    {
      --pMutex->level;
    }

    error = NO_ERROR;
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }

  SpinlockRelease(&pMutex->lock);

  return error;
}

E_Return KernelMutexTryLock(S_KernelMutex* pMutex, int32_t* pLockState)
{
  E_Return        error;
  S_KernelThread* pCurThread;

  SpinlockAcquire(&pMutex->lock);

  pCurThread = GetCurrentThread();

  *pLockState = pMutex->lockState;
  if (pMutex->lockState == 0)
  {
    if ((pMutex->flags & KMUTEX_FLAG_RECURSIVE) == KMUTEX_FLAG_RECURSIVE &&
       pCurThread == pMutex->pAcquiredThread)
    {
      /* If the mutex is recursive, allow the lock */
      if (pMutex->level < MUTEX_MAX_RECURSIVENESS)
      {
        ++pMutex->level;
        error = NO_ERROR;
      }
      else
      {
        error = ERR_EXCEEDED_LIMIT;
      }
    }
    else
    {
      error = ERR_UNAUTHORIZED_ACTION;
    }
  }
  else
  {
    /* Acquire one value */
    pMutex->lockState              = 0;
    pMutex->level                  = 1;
    pMutex->pAcquiredThread        = pCurThread;
    pMutex->acquiredThreadPriority = pCurThread->priority;

    if ((pMutex->flags & KMUTEX_FLAG_PRIO_ELEVATION) ==
        KMUTEX_FLAG_PRIO_ELEVATION)
    {
      /* Update the process priority */
      SetThreadPriority(pCurThread, pMutex->elevatedPriority);
    }

    error = NO_ERROR;
  }

  SpinlockRelease(&pMutex->lock);

  return error;
}

/************************************ EOF *************************************/