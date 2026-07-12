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

#ifndef __CORE_FAST_QUEUE_H_
#define __CORE_FAST_QUEUE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Queue structure. */
typedef struct
{
  /** @brief Head of the queue. */
  size_t head;
  /** @brief Tail of the queue. */
  size_t tail;
  /** @brief Ring buffer used to store the messages. */
  void* pBuffer;
  /** @brief Stores the maximal queue size. */
  size_t maxSize;
  /** @brief Stores the element size. */
  size_t elementSize;
} S_FastQueue;

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
 * @brief Creates an empty queue ready to be used.
 *
 * @details Creates and initializes a new queue. The returned queue is
 * ready to be used.
 *
 * @param[in] kElementCount The number of elements in the queue.
 * @param[in] kElementSize The size of one element in the queue.
 *
 * @return A pointer to the create queue is returned.
 */
S_FastQueue* FQueueCreate(const size_t kElementCount,
                          const size_t kElementSize);

/**
 * @brief Deletes a previously created queue.
 *
 * @details Delete a queue from the memory. If the queue is not empty an error
 * is returned.
 *
 * @param[in, out] ppQueue The queue pointer to destroy.
 */
void FQueueDestroy(S_FastQueue* ppQueue);

/**
 * @brief Enlists a data in the queue.
 *
 * @details Enlists a data in the queue given as parameter. The data will be
 * placed in the tail of the queue.
 *
 * @param[in, out] pQueue The queue to manage.
 * @param[in] const pData The data to enque.
 */
void FQueuePush(S_FastQueue* pQueue, const void* kpData);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from the queue given as parameter. The retreived node
 * that is returned is the one placed in the head of the QUEUE.
 *
 * @param[in, out] pQueue The queue to manage.
 * @param[out] pData The buffer tha received the poped data.
 *
 * @return Returns true when a data was available and read.
 */
bool FQueuePop(S_FastQueue* pQueue, void* ppData);

#endif /* #ifndef __CORE_FAST_QUEUE_H_ */

/************************************ EOF *************************************/