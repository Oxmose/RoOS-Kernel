/*******************************************************************************
 * @file lapic.h
 *
 * @see lapic.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/06/2024
 *
 * @version 2.0
 *
 * @brief Local APIC (Advanced programmable interrupt controler) driver.
 *
 * @details Local APIC (Advanced programmable interrupt controler) driver.
 * Manages x86 IRQs from the IO-APIC. IPI (inter processor interrupt) are also
 * possible thanks to the driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_LAPIC_H_
#define __X86_LAPIC_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <ACPI.h>
#include <stdint.h>
#include <stddef.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 LAPIC driver. */
typedef struct
{
  /**
   * @brief Sets END OF INTERRUPT for the current CPU Local APIC.
   *
   * @details Sets END OF INTERRUPT for the current CPU Local APIC.
   *
   * @param[in] kInterruptLine The interrupt line for which the EOI should be
   * set.
   */
  void (*pSetIRQEOI)(const uint32_t kInterruptLine);

  /**
   * @brief Returns the base address of the local APIC.
   *
   * @details Returns the base address of the local APIC.
   *
   * @return The base address of the LAPIC is returned.
   */
  uintptr_t (*pGetBaseAddress)(void);

  /**
   * @brief Returns the LAPIC identifier.
   *
   * @details Returns the LAPIC identifier for the caller.
   *
   * @return The LAPIC identifier is returned.
   */
  uint8_t (*pGetLAPICId)(void);

  /**
   * @brief Returns the list of detected LAPICs in the system.
   *
   * @details Returns the list of detected LAPICs in the system.
   *
   * @return The list of detected LAPICs in the system is returned.
   */
  const S_LAPICNode* (*pGetLAPICList)(void);

  /**
   * @brief Enables a CPU given its LAPIC id.
   *
   * @details Enables a CPU given its LAPIC id. The startup sequence is
   * executed, using LAPIC IPI.
   *
   * @param[in] kLAPICId The LAPIC identifier for the CPU to start.
   */
  void (*pStartCpu)(const uint8_t kLAPICId);

  /**
   * @brief Sends an IPI to a CPU given its LAPIC id.
   *
   * @details Sends an IPI to a a CPU given its LAPIC id.
   *
   * @param[in] kLAPICId The LAPIC identifier IPI destination.
   * @param[in] kVector The vector used to trigger the IPI.
   */
  void (*pSendIPI)(const uint8_t kLAPICId, const uint8_t kVector);

  /**
   * @brief Initializes a secondary CPU LAPIC.
   *
   * @details Initializes a secondary CPU LAPIC. This function initializes
   * the secondary CPU LAPIC interrupts and settings.
   */
  void (*pInitApCPU)(void);
} S_LAPICDriver;


/** @brief x86 LAPIC Timer driver. */
typedef struct
{
  /**
   * @brief Initializes a secondary CPU LAPIC Timer.
   *
   * @details Initializes a secondary CPU LAPIC Timer. This function
   * initializes the secondary CPU LAPIC timer interrupts and settings.
   *
   * @param[in] kCpuId The CPU identifier for which we should enable the LAPIC
   * timer.
   */
  void (*pInitApCPU)(const uint8_t kCpuId);
} S_LAPICTimerDriver;

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

/* None */

#endif /* #ifndef __X86_LAPIC_H_ */

/************************************ EOF *************************************/