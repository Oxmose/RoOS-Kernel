/*******************************************************************************
 * @file Libcinternal.h
 *
 * @see Libcinternal.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/10/2024
 *
 * @version 1.0
 *
 * @brief Lib C internal functions for roOs.
 *
 * @details Lib C internal functions for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_USER_LIBC_INTERNAL_H_
#define __LIB_USER_LIBC_INTERNAL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Regular spinlock initializer */
#define SPINLOCK_INIT_VALUE 0

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines a regular spinlock. */
typedef volatile uint32_t T_Spinlock;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] LOCK The lock to lock.
*/
#define LIBC_SPINLOCK_LOCK(LOCK) { \
  SpinlockAcquire(&(LOCK));        \
}

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to unlock.
*/
#define LIBC_SPINLOCK_UNLOCK(LOCK) { \
  SpinlockRelease(&(LOCK));          \
}

/**
 * @brief Initializes a kernel spinlock.
 *
 * @details Initializes a kernel spinlock.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define LIBC_SPINLOCK_INIT(LOCK) { \
  (LOCK) = SPINLOCK_INIT_VALUE;    \
}

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
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] pLock The pointer to the lock to lock.
*/
void SpinlockAcquire(T_Spinlock* pLock);

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void SpinlockRelease(T_Spinlock* pLock);

/**
 * @brief Peforms a memory fence for acquire operations.
 *
 * @details Peforms a memory fence for acquire operations. This ensures all
 * accesses are performed after the fence.
 */
void CPUMemoryFenceAcquire(void);

/**
 * @brief Peforms a memory fence for release operations.
 *
 * @details Peforms a memory fence for release operations. This ensures all
 * accesses are performed before the fence.
 */
void CPUMemoryFenceRelease(void);


#endif /* #ifndef __LIB_USER_LIBC_INTERNAL_H_ */

/************************************ EOF *************************************/