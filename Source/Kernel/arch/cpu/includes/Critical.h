/*******************************************************************************
 * @file Critical.h
 *
 * @see Critical.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/06/2026
 *
 * @version 1.0
 *
 * @brief Kernel's concurency management module.
 *
 * @details Kernel's concurency management module. Defines the different basic
 * synchronization primitives used in the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_CRITICAL_H_
#define __CPU_CRITICAL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <Interrupts.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Regular spinlock initializer */
#define SPINLOCK_INIT_VALUE 0

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines an unsigned 32 bits atomic value. */
typedef volatile uint32_t T_U32Atomic;

/** @brief Defines a regular spinlock. */
typedef volatile uint32_t T_Spinlock;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Enters a local critical section in the kernel.
 *
 * @param[out] INT_STATE The local critical state at section's entrance.
 *
 * @details Enters a local critical section in the kernel. Save interrupt state
 * and disables interrupts.
 */
#define KERNEL_ENTER_CRITICAL_LOCAL(INT_STATE) {  \
  INT_STATE = InterruptDisable();                 \
}

/**
 * @brief Exits a local critical section in the kernel.
 *
 * @param[in] INT_STATE The local critical state at section's entrance.
 *
 * @details Exits a local critical section in the kernel. Restore the previous
 * interrupt state.
 */
#define KERNEL_EXIT_CRITICAL_LOCAL(INT_STATE) { \
  InterruptRestore(INT_STATE);                  \
}

/**
 * @brief Initializes a kernel spinlock.
 *
 * @details Initializes a kernel spinlock.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define SPINLOCK_INIT(LOCK) {  \
  LOCK = SPINLOCK_INIT_VALUE;  \
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
 * @brief Enables the CPU interrupts state.
 *
 * @details Enables the CPU interrupts state.
 */
void CPUInterruptEnable(void);

/**
 * @brief Disables the CPU interrupts.
 *
 * @details Disables the CPU interrupt after saving the current state.
 *
 * @return The current interrupt state is returned to be restored later in the
 * execution of the kernel.
 */
uint32_t CPUInterruptDisable(void);

/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode. This will
 * also disable preemption in the kernel.
 *
 * @param[in, out] pLock The pointer to the lock to lock.
*/
void SpinlockAcquire(T_Spinlock* pLock);

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode. This
 * might enable preemption in the kernel if it was disabled by the lock.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void SpinlockRelease(T_Spinlock* pLock);

/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] pLock The pointer to the lock to lock.
*/
void SpinlockAcquireISR(T_Spinlock* pLock);

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void SpinlockReleaseISR(T_Spinlock* pLock);

/**
 * @brief Atomically increments a value in memory.
 *
 * @details Atomically increments a value in memory. Thus function is safe in
 * kernel mode.
 *
 * @param[out] pValue The pointer to the value to atomically increment.
 *
 * @return The value of the atomic before incrementing is returned.
 */
uint32_t AtomicIncrement32(T_U32Atomic* pValue);

/**
 * @brief Atomically decrements a value in memory.
 *
 * @details Atomically decrements a value in memory. Thus function is safe in
 * kernel mode.
 *
 * @param[out] pValue The pointer to the value to atomically decrement.
 *
 * @return The value of the atomic before incrementing is returned.
 */
uint32_t AtomicDecrement32(T_U32Atomic* pValue);

#endif /* #ifndef __CPU_CRITICAL_H_ */

/************************************ EOF *************************************/