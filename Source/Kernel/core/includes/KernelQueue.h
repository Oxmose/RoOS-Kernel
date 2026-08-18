/*******************************************************************************
 * @file KernelQueue.h
 *
 * @see KernelQueue.c
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

#ifndef __CORE_KERNEL_QUEUE_H_
#define __CORE_KERNEL_QUEUE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Queue node structure. */
typedef struct S_KernelQueueNode
{
  /** @brief Next node in the queue. */
  struct S_KernelQueueNode* pNext;
  /** @brief Previous node in the queue. */
  struct S_KernelQueueNode* pPrev;

  /** @brief Pointer to the node's queue, NULL if not enlisted. */
  void* pQueuePtr;

  /** @brief Node's priority, used when the queue is a priority queue. */
  uint64_t priority;

  /** @brief Node's data pointer. Store the address of the contained data. */
  void* pData;
} S_KernelQueueNode;

/** @brief Queue structure. */
typedef struct
{
  /** @brief Head of the queue. */
  S_KernelQueueNode* pHead;
  /** @brief Tail of the queue. */
  S_KernelQueueNode* pTail;

  /** @brief Current queue's size. */
  size_t size;
} S_KernelQueue;

/*******************************************************************************
 * MACROS
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
/**
 * @brief Creates a new queue node.
 *
 * @details Creates a node ready to be inserted in a queue. The data can be
 * modified later by accessing the data field of the node structure.
 *
 * @warning A node should be only used in one queue at most.
 *
 * @param[in] pData The pointer to the data to carry in the node.
 *
 * @return The node pointer is returned.
 */
S_KernelQueueNode* KQueueCreateNode(void* pData);

/**
 * @brief Initializes a new queue node.
 *
 * @details Initializes a node ready to be inserted in a queue. The data can be
 * modified later by accessing the data field of the node structure.
 *
 * @warning A node should be only used in one queue at most.
 *
 * @param[in] pNode The pointer to the node to initialize to carry in the node.
 * @param[in] pData The pointer to the data to carry in the node.
 */
void KQueueInitNode(S_KernelQueueNode* pNode, void* pData);

/**
 * @brief Deletes a queue node.
 *
 * @details Deletes a node from the memory. The node should not be used in any
 * queue. If it is the case, the function will return an error.
 *
 * @param[in, out] ppNode The node pointer of pointer to destroy.
 */
void KQueueDestroyNode(S_KernelQueueNode** ppNode);

/**
 * @brief Creates an empty queue ready to be used.
 *
 * @details Creates and initializes a new queue. The returned queue is
 * ready to be used.
 *
 * @return A pointer to the create queue is returned.
 */
S_KernelQueue* KQueueCreate(void);

/**
 * @brief Deletes a previously created queue.
 *
 * @details Delete a queue from the memory. If the queue is not empty an error
 * is returned.
 *
 * @param[in, out] ppQueue The queue pointer of pointer to destroy.
 */
void KQueueDestroy(S_KernelQueue** ppQueue);

/**
 * @brief Enlists a node in the queue.
 *
 * @details Enlists a node in the queue given as parameter. The data will be
 * placed in the tail of the queue.
 *
 * @param[in, out] pNode A now node to add in the queue.
 * @param[in, out] pQueue The queue to manage.
 */
void KQueuePush(S_KernelQueueNode* pNode, S_KernelQueue* pQueue);

/**
 * @brief Enlists a node in the queue.
 *
 * @details Enlist a node in the queue given as parameter. The data will be
 * placed in the queue with regard to the priority argument.
 *
 * @param[in, out] pNode A now node to add in the queue.
 * @param[in, out] pQueue The queue to manage.
 * @param[in] kPriority The element priority.
 */
void KQueuePushPrio(S_KernelQueueNode* pNode,
                    S_KernelQueue*     pQueue,
                    const uint64_t     kPriority);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from the queue given as parameter. The retreived node
 * that is returned is the one placed in the head of the QUEUE.
 *
 * @param[in, out] pQueue The queue to manage.
 *
 * @return The data pointer placed in the head of the queue is returned.
 */
S_KernelQueueNode* KQueuePop(S_KernelQueue* pQueue);

/**
 * @brief Finds a node containing the data given as parameter in the queue.
 *
 * @details Find a node containing the data given as parameter in the queue.
 * An error is set if not any node is found.
 *
 * @param[in] pQueue The queue to search the data in.
 * @param[in] data The data contained by the node to find.
 *
 * @return The function returns a pointer to the node if found, NULL otherwise.
 */
S_KernelQueueNode* KQueueFind(S_KernelQueue* pQueue, const void* kpData);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from a queue given as parameter. If the node is not
 * found, nothing is done and an error is returned.
 *
 * @param[in, out] pQueue The queue containing the node.
 * @param[in, out] pNode The node to remove.
 */
void KQueueRemove(S_KernelQueue*     pQueue,
                  S_KernelQueueNode* pNode);

/**
 * @brief Returns the size of a queue.
 *
 * @details Returns the size of a queue. If the queue does not exists 0 is
 * returned.
 *
 * @param[in] kpQueue The queue of which the size should be returned.
 *
 * @return Returns the size of a queue. If the queue does not exists 0 is
 * returned.
 */
size_t KQueueSize(const S_KernelQueue* kpQueue);

/**
 * @brief Cleans a queue. Removes all its node and destroy them.
 *
 * @details Cleans a queue. Removes all its node and destroy them. The memory
 * used by the node is released. In the case the user set the parameter
 * kCleanData to true and the nodes data are not NULL, the nodes data are also
 * freed using the kfree function.
 *
 * @param[in, out] pQueue The queue to clean.
 * @param[in] kCleanData Tells if the data in the detroyed node should be freed
 * using the kfree function.
 */
void KQueueClean(S_KernelQueue* pQueue);

#endif /* #ifndef __CORE_KERNEL_QUEUE_H_ */

/************************************ EOF *************************************/