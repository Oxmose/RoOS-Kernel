/*******************************************************************************
 * @file IOAPIC.h
 *
 * @see IOAPIC.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 05/06/2024
 *
 * @version 2.0
 *
 * @brief IO-APIC (IO advanced programmable interrupt controller) driver.
 *
 * @details IO-APIC (IO advanced programmable interrupt controller) driver.
 * Allows to remap the IO-APIC IRQ, set the IRQs mask and manage EoI for the
 * X86 IO-APIC.
 *
 * @warning This driver also use the LAPIC driver to function correctly.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_IO_APIC_H_
#define __X86_IO_APIC_H_

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
/** @brief IO-APIC trigger mode. */
typedef enum
{
  /** @brief IO-APIC edge trigger mode. */
  IOAPIC_TRIGGER_MODE_EDGE,
  /** @brief IO-APIC trigger mode. */
  IOAPIC_TRIGGER_MODE_LEVEL
} E_IOAPICTriggerMode;

/** @brief IO-APIC polarity. */
typedef enum
{
  /** @brief IO-APIC high polarity. */
  IOAPIC_POLARITY_HIGH,
  /** @brief IO-APIC low polarity. */
  IOAPIC_POLARITY_LOW
} E_IOAPICPolarity;

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
 * @brief Setup an IRQ in the IO-APIC.
 *
 * @details Setup an IRQ in the IO-APIC. This function will setup the IRQ
 * in the IO-APIC with the given parameters. The function will return an error
 * code if the setup failed.
 *
 * @param[in] kIRQNumber The IRQ number to setup.
 * @param[in] kInterruptLine The interrupt line to use for the IRQ.
 * @param[in] kLapicDestination The LAPIC ID of the destination LAPIC.
 * @param[in] kTriggerMode The trigger mode of the IRQ.
 * @param[in] kPolarity The polarity of the IRQ.
 */
void IOAPICSetupIRQ(const uint32_t            kIRQNumber,
                    const uint32_t            kInterruptLine,
                    const uint32_t            kLapicDestination,
                    const E_IOAPICTriggerMode kTriggerMode,
                    const E_IOAPICPolarity    kPolarity);

/**
 * @brief Disable an IRQ in the IO-APIC.
 *
 * @details Disable an IRQ in the IO-APIC. This function will disable the IRQ
 * in the IO-APIC. The function will return an error code if the disable failed.
 *
 * @param[in] kIRQNumber The IRQ number to disable.
 */
void IOAPICDisableIRQ(const uint32_t kIRQNumber);
#endif /* #ifndef __X86_IO_APIC_H_ */

/************************************ EOF *************************************/