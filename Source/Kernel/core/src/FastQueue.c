/*******************************************************************************
 * @file FastQueue.h
 *
 * @see FastQueue.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.5
 *
 * @brief Fast, race condition free queues.
 *
 * @details Fast, race condition free queues. Those queues can be read and
 * written without race condition at the same time and without using locks.
 * Those queues can only have one reader and one writer.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <KernelHeap.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <FastQueue.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "FQUEUE"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the kernel queues to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the kernel queues to ensure correctness of
 * execution. Due to the critical nature of the kernel queues, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define FQUEUE_ASSERT(COND, MSG, ERROR) {                \
  if ((COND) == false)                                   \
  {                                                      \
    PANIC(ERROR, MODULE_NAME, MSG, false);               \
  }                                                      \
}

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
S_FastQueue* FQueueCreate(const size_t kElementCount, const size_t kElementSize)
{
  S_FastQueue* pNewQueue;

  /* Create new queue */
  pNewQueue = KMalloc(sizeof(S_FastQueue), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
  pNewQueue->pBuffer = KMalloc((kElementCount + 1) * kElementSize,
                                ALIGN_ADDRESS,
                                KMALLOC_FREE_POOL);

  /* Init the structure */
  pNewQueue->head = 0;
  pNewQueue->tail = 0;
  pNewQueue->maxSize     = kElementCount + 1;
  pNewQueue->elementSize = kElementSize;

  return pNewQueue;
}

void FQueueDestroy(S_FastQueue* pQueue)
{
  FQUEUE_ASSERT(pQueue != NULL,
                "Tried to delete a NULL queue",
                ERR_INVALID_PARAMETER);

  /* Free the memory */
  KFree(pQueue->pBuffer);
  KFree(pQueue);
}

void FQueuePush(S_FastQueue* pQueue, const void* kpData)
{
  size_t nextIdx;

  /* Get the next write index */
  nextIdx = (pQueue->head + 1) % pQueue->maxSize;

  FQUEUE_ASSERT(nextIdx != pQueue->tail,
                "Full Fast Queue",
                ERR_UNAUTHORIZED_ACTION);

  /* Copy the data */
  memcpy(pQueue->pBuffer + pQueue->head * pQueue->elementSize,
          kpData,
          pQueue->elementSize);

  /* Ensure the copy is made and update the write index */
  CPUMemoryFenceRelease();
  pQueue->head = nextIdx;
}

bool FQueuePop(S_FastQueue* pQueue, void* pData)
{
  bool available;

  /* Check if available */
  if (pQueue->tail != pQueue->head)
  {
    /* Wait for all read to be done */
    CPUMemoryFenceAcquire();

    /* Copy the data */
    memcpy(pData,
           pQueue->pBuffer + pQueue->tail * pQueue->elementSize,
           pQueue->elementSize);

    /* Wait for all read to be done */
    CPUMemoryFenceAcquire();

    /* Get the next read index */
    pQueue->tail = (pQueue->tail + 1) % pQueue->maxSize;

    available = true;
  }
  else
  {
    available = false;
  }

  return available;
}

/************************************ EOF *************************************/