/*******************************************************************************
 * @file Interrupts.h
 *
 * @see Interrupts.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
 *
 * @version 3.0
 *
 * @brief Interrupt manager.
 *
 * @details Interrupt manager. Allows to attach ISR to interrupt lines and
 * manage IRQ used by the CPU. We also define the general interrupt handler
 * here.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_INTERRUPTS_H_
#define __CORE_INTERRUPTS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief Interrupt types enumeration.
 */
typedef enum
{
  /** @brief Spurious interrupt type. */
  INTERRUPT_TYPE_SPURIOUS,
  /** @brief Regular interrupt type. */
  INTERRUPT_TYPE_REGULAR
} E_InterruptType;

/**
 * @brief Custom interrupt handler structure.
 *
 * @details Custom interrupt handler structure. This is the function called on
 * an interrupt handling.
 *
 * @return The function returns if the scheduler shall be called on return.
 */
typedef bool (*T_InterruptHandler)(void);

/** @brief Defines the basic interface for an interrupt management driver (let
 * it be PIC or IO APIC for instance).
 */
typedef struct
{
  /**
   * @brief The function should acknowleges an interrupt.
   *
   * @details The function should acknowleges an interrupt.
   *
   * @param[in] kInterruptNumber The interrupt number to acknowledge.
   */
  void (*pSetEndOfInterrupt)(const uint32_t kInterruptNumber);

  /**
   * @brief The function should check if the serviced interrupt is a spurious
   * interrupt. It also should handle the spurious interrupt.
   *
   * @details The function should check if the serviced interrupt is a
   * spurious interrupt. It also should handle the spurious interrupt.
   *
   * @param[in] kIntNumber The interrupt number of the interrupt to test.
   *
   * @return The function will return the interrupt type.
   * - INTERRUPT_TYPE_SPURIOUS if the current interrupt is a spurious one.
   * - INTERRUPT_TYPE_REGULAR if the current interrupt is a regular one.
   */
  E_InterruptType (*pHandleSpurious)(const uint32_t kIntNumber);
} S_InterruptDriver;

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
 * @brief Initializes the kernel's interrupt manager.
 *
 * @details Blanks the handlers memory, initializes panic and spurious interrupt
 * lines handlers.
 */
void InterruptInit(void);

/**
 * @brief Kernel's main interrupt handler.
 *
 * @details Generic and global interrupt handler. This function should only be
 * called by an assembly interrupt handler. The function will dispatch the
 * interrupt to the desired function to handle the interrupt.
 */
void InterruptMainHandler(void);

/**
 * @brief Set the driver to be used by the kernel to manage interrupts.
 *
 * @details Changes the current interrupt manager by the new driver given as
 * parameter. The old driver structure is removed from memory.
 *
 * @param[in] kpDriver The driver structure to be used by the interrupt manager.
 *
 * @return The success state or the error code.
 */
E_Return InterruptSetDriver(const S_InterruptDriver* kpDriver);

/**
 * @brief Registers an interrupt handler for the desired interrupt.
 *
 * @details Registers a custom interrupt handler to be executed. The interrupt
 * must be greater or equal to the minimal authorized custom interrupt and less
 * than the maximal one.
 *
 * @param[in] kInterruptId The interrupt to attach the handler to.
 * @param[in] kHandler The handler for the desired interrupt.
 *
 * @return The success state or the error code.
 */
E_Return InterruptRegister(const uint32_t           kInterruptId,
                           const T_InterruptHandler kHandler);

/**
 * @brief Unregisters a new interrupt handler for the desired interrupt line.
 *
 * @details Unregisters a custom interrupt handler to be executed. The interrupt
 * line must be greater or equal to the minimal authorized custom interrupt line
 * and less than the maximal one.
 *
 * @param[in] kInterruptId The interrupt line to deattach the handler from.
 *
 * @return The success state or the error code.
 */
E_Return InterruptRemove(const uint32_t kInterruptId);

/**
 * @brief Restores the CPU interrupts state.
 *
 * @details Restores the CPU interrupts.
 *
 * @param[in] kPreviousState The previous interrupts state that has to be
 * retored.
 */
void InterruptRestore(const uint32_t kPreviousState);

/**
 * @brief Disables the CPU interrupts.
 *
 * @details Disables the CPU interrupts.
 *
 * @return The current interrupt state is returned to be restored latter in the
 * execution of the kernel.
 */
uint32_t InterruptDisable(void);

/**
 * @brief Acknowleges an interrupt.
 *
 * @details Acknowleges an interrupt. Used for hardware interrupts.
 * @param[in] kTriggerMode The trigger mode of the interrupt.
 */
void InterruptSetEOI(const uint32_t kInterruptId);

/**
 * @brief Allocates an interrupt line.
 *
 * @details Allocates an interrupt line. The interrupt line is allocated from
 * the available interrupt lines. The function will return the allocated
 * interrupt line or -1 if no interrupt line is available.
 *
 * @return The allocated interrupt line is returned or -1 if no interrupt line
 * is available.
 */
int32_t AllocateInterruptLine(void);

/**
 * @brief Frees an interrupt line.
 *
 * @details Frees an interrupt line. The interrupt line is freed and can be
 * allocated again. The function will return an error code if the interrupt line
 * is not valid or if the interrupt line is not allocated.
 *
 * @param[in] kInterruptId The interrupt line to free.
 */
void FreeInterruptLine(const int32_t kInterruptId);

#endif /* #ifndef __CORE_INTERRUPTS_H_ */

/************************************ EOF *************************************/
