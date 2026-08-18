/*******************************************************************************
 * @file KernelSemaphore.h
 *
 * @see KernelSemaphore.c
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

#ifndef __CORE_KERNEL_SEMAPHORE_H_
#define __CORE_KERNEL_SEMAPHORE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <KernelQueue.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Semaphore flag: semaphore has FIFO queuing discipline. */
#define KSEMAPHORE_FLAG_QUEUING_FIFO 0x00000001

/** @brief Semaphore flag: semaphore has priority based queuing discipline. */
#define KSEMAPHORE_FLAG_QUEUING_PRIO 0x00000002

/** @brief Semaphore flag: binary semaphore. */
#define KSEMAPHORE_FLAG_BINARY 0x00000004

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Semaphore structure definition. */
typedef struct
{
  /** @brief Semaphore flags */
  uint32_t flags;
   /** @brief Semaphore lock state */
  volatile int32_t lockState;
  /** @brief Semaphore lock. */
  S_KernelSpinlock lock;
  /** @brief Semaphore waiting threads queue */
  S_KernelQueue *pWaitingList;
} S_KernelSemaphore;

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
 * @brief Initializes the semaphore structure.
 *
 * @details Initializes the semaphore structure. The initial state of a
 * semaphore is available.
 *
 * @param[out] pSemaphore The pointer to the semaphore to initialize.
 * @param[in] kFlags The semaphore flags to be used.
 * @param[in] kInitialValue The initial value of the semaphore.
 *
 * @return The success state or the error code.
 */
E_Return KernelSemaphoreInit(S_KernelSemaphore* pSemaphore,
                             const uint32_t     kFlags,
                             const int32_t      kInitialValue);

/**
 * @brief Destroys the semaphore given as parameter.
 *
 * @details Destroys the semaphore given as parameter. Using a destroyed
 * semaphore produces undefined behaviors. The function will not destroy the
 * semaphore if threads are waiting on it.
 *
 * @param[in, out] pSemaphore The semaphore to destroy.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
E_Return KernelSemaphoreDestroy(S_KernelSemaphore* pSemaphore);

/**
 * @brief Locks on the semaphore given as parameter.
 *
 * @details Locks on the semaphore given as parameter. The calling thread will
 * block on this call until the semaphore is aquired.
 *
 * @param[in] pSemaphore The semaphore to wait on.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
E_Return KernelSemaphoreWait(S_KernelSemaphore* pSemaphore);

/**
 * @brief Unlocks the semaphore given as parameter.
 *
 * @param[in] pSemaphore The semaphore to unlock.
 *
 * @return The success state or the error code.
 *
 * @warning Only the semaphore thread owner can unlock a semaphore. Using a
 * non-initialized or destroyed semaphore produces undefined behavior.
 */
E_Return KernelSemaphorePost(S_KernelSemaphore* pSemaphore);

/**
 * @brief Try to wait on the semaphore given as parameter.
 *
 * @details Try to wait on the semaphore as parameter. The function will
 * return the current semaphore wait state. If possible the function will
 * aquire the semaphore.
 *
 * @param[in] pSemaphore The semaphore to wait.
 * @param[out] pLockState The buffer that receives the semaphore wait state.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
E_Return KernelSemaphoreTryWait(S_KernelSemaphore* pSemaphore,
                                int32_t*           pLockState);

#endif /* #ifndef __CORE_KERNEL_SEMAPHORE_H_ */

/************************************ EOF *************************************/