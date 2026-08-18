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
  /** @brief The function should enable or diable an IRQ given the IRQ number
   * used as parameter.
   *
   * @details The function should enable or diable an IRQ given the IRQ number
   * used as parameter.
   *
   * @param[in] kIRQNumber The number of the IRQ to enable/disable.
   * @param[in] kEnabled Must be set to true to enable the IRQ and false to
   * disable the IRQ.
   */
  void (*pSetIRQMask)(const uint32_t kIRQNumber, const bool kEnabled);

  /**
   * @brief The function should acknowleges an IRQ.
   *
   * @details The function should acknowleges an IRQ.
   *
   * @param[in] kIRQNumber The irq number to acknowledge.
   */
  void (*pSetIRQEOI)(const uint32_t kIRQNumber);

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

  /**
   * @brief Returns the interrupt line attached to an IRQ.
   *
   * @details Returns the interrupt line attached to an IRQ. 0xFFFFFFFF is
   * returned if the IRQ number is not supported by the driver.
   *
   * @param[in] kIRQNumber The IRQ line for the requested interrupt line.
   *
   * @return The interrupt line attached to an IRQ. 0xFFFFFFFF is returned if
   * the IRQ number is not supported by the driver.
   */
  uint32_t (*pGetIRQInterruptLine)(const uint32_t kIRQNumber);
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
 * @param[in] kIsIRQ Tells if the interrupt is an IRQ.
 *
 * @return The success state or the error code.
 */
E_Return InterruptRegister(const uint32_t           kInterruptId,
                           const T_InterruptHandler kHandler,
                           const bool               kIsIRQ);

/**
 * @brief Unregisters a new interrupt handler for the desired interrupt line.
 *
 * @details Unregisters a custom interrupt handler to be executed. The interrupt
 * line must be greater or equal to the minimal authorized custom interrupt line
 * and less than the maximal one.
 *
 * @param[in] kInterruptId The interrupt line to deattach the handler from.
 * @param[in] kIsIRQ Tells if the interrupt is an IRQ.
 *
 * @return The success state or the error code.
 */
E_Return InterruptRemove(const uint32_t kInterruptId, const bool kIsIRQ);

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
 * @brief Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @param[in] kInterruptId The IRQ id to enable/disable.
 * @param[in] kEnabled Must be set to true to enable the IRQ or false
 * to disable the IRQ.
 */
void InterruptSetIRQMask(const uint32_t kIRQNumber, const bool kEnabled);

/**
 * @brief Acknowleges an interrupt.
 *
 * @details Acknowleges an interrupt. Used for hardware interrupts.
 *
 * @param[in] kInterruptId The interrupt number to acknowledge.
 */
void InterruptSetEOI(const uint32_t kInterruptId);

#endif /* #ifndef __CORE_INTERRUPTS_H_ */

/************************************ EOF *************************************/
