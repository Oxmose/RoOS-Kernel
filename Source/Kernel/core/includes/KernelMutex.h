/*******************************************************************************
 * @file KernelMutex.h
 *
 * @see KernelMutex.c
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

#ifndef __CORE_KERNEL_MUTEX_H_
#define __CORE_KERNEL_MUTEX_H_

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
/** @brief Mutex flag: mutex has FIFO queuing discipline. */
#define KMUTEX_FLAG_QUEUING_FIFO 0x00000001

/** @brief Mutex flag: mutex has priority based queuing discipline. */
#define KMUTEX_FLAG_QUEUING_PRIO 0x00000002

/** @brief Mutex flag: recursive mutex. */
#define KMUTEX_FLAG_RECURSIVE 0x00000004

/** @brief Mutex flag: priority elevation mutex. */
#define KMUTEX_FLAG_PRIO_ELEVATION 0x00000008

/** @brief Mutex flag: priority elevation value. */
#define KMUTEX_FLAG_PRIORITY(X) ((X) << 16)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Mutex structure definition. */
typedef struct
{
  /** @brief Mutex flags */
  uint32_t flags;
  /** @brief Acquired thread's initial priority */
  uint32_t acquiredThreadPriority;
  /** @brief Priority elevation value. */
  uint32_t elevatedPriority;
  /** @brief Mutex recursive level */
  uint32_t level;
   /** @brief Mutex lock state */
  volatile int32_t lockState;
  /** @brief Mutex lock. */
  S_KernelSpinlock lock;
  /** @brief Acquired thread pointer */
  S_KernelThread* pAcquiredThread;
  /** @brief Mutex locked threads queue */
  S_KernelQueue *pWaitingList;
} S_KernelMutex;

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
 * @brief Initializes the mutex structure.
 *
 * @details Initializes the mutex structure. The initial state of a
 * mutex is available.
 *
 * @param[out] pMutex The pointer to the mutex to initialize.
 * @param[in] kFlags The mutex flags to be used.
 *
 * @return The success state or the error code.
 */
E_Return KernelMutexInit(S_KernelMutex* pMutex, const uint32_t kFlags);

/**
 * @brief Destroys the mutex given as parameter.
 *
 * @details Destroys the mutex given as parameter. Using a destroyed mutex
 * produces undefined behaviors.The function will not destroy the mutex if
 * threads are waiting on it.
 *
 * @param[in, out] pMutex The mutex to destroy.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed mutex produces undefined
 * behavior.
 */
E_Return KernelMutexDestroy(S_KernelMutex* pMutex);

/**
 * @brief Locks on the mutex given as parameter.
 *
 * @details Locks on the mutex given as parameter. The calling thread will
 * block on this call until the mutex is aquired.
 *
 * @param[in] pMutex The mutex to lock.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed mutex produces undefined
 * behavior.
 */
E_Return KernelMutexLock(S_KernelMutex* pMutex);

/**
 * @brief Unlocks the mutex given as parameter.
 *
 * @param[in] pMutex The mutex to unlock.
 *
 * @return The success state or the error code.
 *
 * @warning Only the mutex thread owner can unlock a mutex. Using a
 * non-initialized or destroyed mutex produces undefined behavior.
 */
E_Return KernelMutexUnlock(S_KernelMutex* pMutex);

/**
 * @brief Try to lock on the mutex given as parameter.
 *
 * @details Try to lock on the mutex mutex as parameter. The function will
 * return the current mutex lock state. If possible the function will
 * aquire the mutex.
 *
 * @param[in] pMutex The mutex to lock.
 * @param[out] pLockState The buffer that receives the mutex lock state.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed mutex produces undefined
 * behavior.
 */
E_Return KernelMutexTryLock(S_KernelMutex* pMutex, int32_t* pLockState);

#endif /* #ifndef __SYNC_KMUTEX_H_ */

/************************************ EOF *************************************/