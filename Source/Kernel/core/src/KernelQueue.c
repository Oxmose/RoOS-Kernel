/*******************************************************************************
 * @file KernelQueue.c
 *
 * @see KernelQueue.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.5
 *
 * @brief Kernel specific queue structures.
 *
 * @details Kernel specific queue structures. These queues are used as priority
 * queue or regular queues. A queue can virtually store every type of data and
 * is just a wrapper.
 *
 * @warning This implementation is not thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <Panic.h>
#include <KernelHeap.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <KernelQueue.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "KQUEUE"

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
#define KQUEUE_ASSERT(COND, MSG, ERROR) {                \
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
S_KernelQueueNode* KQueueCreateNode(void* pData)
{
  S_KernelQueueNode* pNewNode;

  /* Create new node */
  pNewNode = KMalloc(sizeof(S_KernelQueueNode),
                     ALIGN_ADDRESS,
                     KMALLOC_FREE_POOL);

  /* Init the structure */
  memset(pNewNode, 0, sizeof(S_KernelQueueNode));
  pNewNode->pData = pData;

  return pNewNode;
}

void KQueueInitNode(S_KernelQueueNode* pNode, void* pData)
{
  KQUEUE_ASSERT(pNode != NULL, "Initialized NULL node", ERR_INVALID_PARAMETER);

  memset(pNode, 0, sizeof(S_KernelQueueNode));
  pNode->pData = pData;
}

void KQueueDestroyNode(S_KernelQueueNode** ppNode)
{
  KQUEUE_ASSERT((ppNode != NULL && *ppNode != NULL),
                "Tried to delete a NULL node",
                ERR_INVALID_PARAMETER);

  KQUEUE_ASSERT((*ppNode)->pQueuePtr == NULL,
                "Tried to delete an enlisted node",
                ERR_UNAUTHORIZED_ACTION);

  KFree(*ppNode);
  *ppNode = NULL;
}

S_KernelQueue* KQueueCreate(void)
{
  S_KernelQueue* pNewQueue;

  /* Create new queue */
  pNewQueue = KMalloc(sizeof(S_KernelQueue), ALIGN_ADDRESS, KMALLOC_FREE_POOL);

  /* Init the structure */
  memset(pNewQueue, 0, sizeof(S_KernelQueue));

  return pNewQueue;
}

void KQueueDestroy(S_KernelQueue** ppQueue)
{
  KQUEUE_ASSERT((ppQueue != NULL && *ppQueue != NULL),
                "Tried to delete a NULL queue",
                ERR_INVALID_PARAMETER);

  KQUEUE_ASSERT(((*ppQueue)->pHead == NULL && (*ppQueue)->pTail == NULL),
                "Tried to delete a non empty queue",
                ERR_UNAUTHORIZED_ACTION);

  KFree(*ppQueue);
  *ppQueue = NULL;
}

void KQueuePush(S_KernelQueueNode* pNode, S_KernelQueue* pQueue)
{
  KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                "Cannot push with NULL knode or NULL kqueue",
                ERR_INVALID_PARAMETER);
  KQUEUE_ASSERT(pNode->pQueuePtr == NULL,
                "Tried to push an enlisted node",
                ERR_UNAUTHORIZED_ACTION);

  /* If this queue is empty */
  if (pQueue->pHead == NULL)
  {
    /* Set the first item */
    pQueue->pHead = pNode;
    pQueue->pTail = pNode;
    pNode->pNext  = NULL;
    pNode->pPrev  = NULL;
  }
  else
  {
    /* Just put on the head */
    pNode->pNext         = pQueue->pHead;
    pNode->pPrev         = NULL;
    pQueue->pHead->pPrev = pNode;
    pQueue->pHead        = pNode;
  }

  ++pQueue->size;
  pNode->pQueuePtr = pQueue;

  KQUEUE_ASSERT((pNode->pNext != pNode->pPrev ||
                 pNode->pNext == NULL         ||
                 pNode->pPrev == NULL),
                "Cycle detected in KQueue",
                ERR_UNAUTHORIZED_ACTION);
}


void KQueuePushPrio(S_KernelQueueNode* pNode,
                    S_KernelQueue*     pQueue,
                    const uint64_t     kPriority)
{
  S_KernelQueueNode* pCursor;

  KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                "Cannot push with NULL knode or NULL kqueue",
                ERR_INVALID_PARAMETER);
  KQUEUE_ASSERT(pNode->pQueuePtr == NULL,
                "Tried to push an enlisted node",
                ERR_UNAUTHORIZED_ACTION);

  pNode->priority = kPriority;

  /* If this queue is empty */
  if (pQueue->pHead != NULL)
  {
    pCursor = pQueue->pHead;
    while (pCursor != NULL && pCursor->priority > kPriority)
    {
      pCursor = pCursor->pNext;
    }

    if (pCursor != NULL)
    {
      pNode->pNext   = pCursor;
      pNode->pPrev   = pCursor->pPrev;
      pCursor->pPrev = pNode;

      if (pNode->pPrev != NULL)
      {
          pNode->pPrev->pNext = pNode;
      }
      else
      {
          pQueue->pHead = pNode;
      }
    }
    else
    {
      /* Just put on the tail */
      pNode->pPrev         = pQueue->pTail;
      pNode->pNext         = NULL;
      pQueue->pTail->pNext = pNode;
      pQueue->pTail        = pNode;
    }
  }
  else
  {
    /* Set the first item */
    pQueue->pHead = pNode;
    pQueue->pTail = pNode;
    pNode->pNext  = NULL;
    pNode->pPrev  = NULL;
  }

  ++pQueue->size;
  pNode->pQueuePtr = pQueue;

  KQUEUE_ASSERT((pNode->pNext != pNode->pPrev ||
                 pNode->pNext == NULL ||
                 pNode->pPrev == NULL),
                "Cycle detected in KQueue",
                ERR_UNAUTHORIZED_ACTION);
}

S_KernelQueueNode* KQueuePop(S_KernelQueue* pQueue)
{
  S_KernelQueueNode* pNode;

  KQUEUE_ASSERT(pQueue != NULL,
                "Cannot pop NULL kqueue",
                ERR_INVALID_PARAMETER);

  /* Check for empty queue. */
  if (pQueue->pHead != NULL)
  {
    /* Dequeue the last item */
    pNode = pQueue->pTail;

    if (pNode->pPrev != NULL)
    {
      pNode->pPrev->pNext = NULL;
      pQueue->pTail       = pNode->pPrev;
    }
    else
    {
      pQueue->pHead = NULL;
      pQueue->pTail = NULL;
    }

    --pQueue->size;

    pNode->pNext     = NULL;
    pNode->pPrev     = NULL;
    pNode->pQueuePtr = NULL;
  }
  else
  {
    pNode = NULL;
  }

  return pNode;
}

S_KernelQueueNode* KQueueFind(S_KernelQueue* pQueue, const void* kpData)
{
  S_KernelQueueNode* pNode;

  KQUEUE_ASSERT(pQueue != NULL,
                "Cannot find in NULL kqueue",
                ERR_INVALID_PARAMETER);

  /* Search for the data */
  pNode = pQueue->pHead;
  while (pNode != NULL && pNode->pData != kpData)
  {
    pNode = pNode->pNext;
  }

  return pNode;
}

void KQueueRemove(S_KernelQueue* pQueue, S_KernelQueueNode* pNode)
{
  KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                "Cannot remove with NULL knode or NULL kqueue",
                ERR_INVALID_PARAMETER);
  KQUEUE_ASSERT((pNode->pQueuePtr == pQueue),
                "Cannot remove with knode from different queue.",
                ERR_INVALID_PARAMETER);

  /* Manage link */
  if (pNode->pPrev != NULL && pNode->pNext != NULL)
  {
    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;
  }
  else if (pNode->pPrev == NULL && pNode->pNext != NULL)
  {
    pQueue->pHead = pNode->pNext;
    pNode->pNext->pPrev = NULL;
  }
  else if (pNode->pPrev != NULL && pNode->pNext == NULL)
  {
    pQueue->pTail = pNode->pPrev;
    pNode->pPrev->pNext = NULL;
  }
  else
  {
    pQueue->pHead = NULL;
    pQueue->pTail = NULL;
  }

  pNode->pNext     = NULL;
  pNode->pPrev     = NULL;
  pNode->pQueuePtr = NULL;

  --pQueue->size;
}

size_t KQueueSize(const S_KernelQueue* kpQueue)
{
  KQUEUE_ASSERT(kpQueue != NULL,
                "Cannot get size of NULL kqueue",
                ERR_INVALID_PARAMETER);

  return kpQueue->size;
}

void KQueueClean(S_KernelQueue* pQueue)
{
  S_KernelQueueNode* pNode;
  S_KernelQueueNode* pSaveNode;

  KQUEUE_ASSERT(pQueue != NULL,
                "Cannot clean NULL kqueue",
                ERR_INVALID_PARAMETER);

  pNode = pQueue->pHead;
  while (pNode != NULL)
  {
    /* Get next node */
    pSaveNode = pNode;
    pNode = pNode->pNext;

    /* Destroy the node */
    pSaveNode->pQueuePtr = NULL;
    KQueueDestroyNode(&pSaveNode);
  }

  pQueue->pHead = NULL;
  pQueue->pTail = NULL;
}

/************************************ EOF *************************************/