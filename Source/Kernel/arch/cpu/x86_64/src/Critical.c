/*******************************************************************************
 * @file Critical.c
 *
 * @see Critical.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief Kernel critical section implementation.
 *
 * @details Kernel critical section implementation. Uses spinlock and interrupt
 * management to create critical sections.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <stdint.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* None TODO */

/* Header file */
#include <Critical.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/***************************
 * Misc CPU Definitions
 **************************/
/** @brief CPU flags interrupt enabled flag. */
#define CPU_RFLAGS_IF 0x000000200

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
void CPUInterruptEnable(void)
{
  __asm__ __volatile__("mfence\n\t"
                        "sti\n\t"
                        :
                        :
                        : "memory");
}

uint32_t CPUInterruptDisable(void)
{
  uint32_t prevState;
  uint64_t flags;

  /* Save current state */
  __asm__ __volatile__("pushfq\n\t"
                       "pop %0\n\t"
                       : "=g" (flags)
                       :
                       : "memory");
  prevState = ((flags & CPU_RFLAGS_IF) != 0);

  /* Clear interrupt */
  __asm__ __volatile__("cli\n\t"
                       "mfence\n\t"
                       :
                       :
                       : "memory");

  return prevState;
}

void KernelLock(S_KernelSpinlock* pLock)
{
  uint32_t interruptState;

  /* Get interrupt state, disable interrupts, lock spinlock */
  interruptState = InterruptDisable();
  SpinlockAcquire(&pLock->lock);
  pLock->interruptState = interruptState;
}

void KernelUnlock(S_KernelSpinlock* pLock)
{
  uint32_t interruptState;

  /* Unlock the spinlock and restore the interrupt state */
  interruptState = pLock->interruptState;
  SpinlockRelease(&pLock->lock);
  InterruptRestore(interruptState);
}

/************************************ EOF *************************************/