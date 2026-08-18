/*******************************************************************************
 * @file ACPI.h
 *
 * @see ACPI.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 04/06/2024
 *
 * @version 2.0
 *
 * @brief Kernel ACPI driver.
 *
 * @details Kernel ACPI driver, detects and parse the ACPI for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_ACPI_H_
#define __X86_ACPI_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief APIC IO APIC custom descriptor */
typedef struct
{
  /** @brief Stores the IO-APIC identifier. */
  uint8_t ioApicId;
  /** @brief Stores the IO-APIC MMIO address. */
  uint32_t ioApicAddr;
  /** @brief Stores the IO-APIC GSI base address. */
  uint32_t globalSystemInterruptBase;
} S_IOACPIDescriptor;

/** @brief APIC LAPIC custom descriptor */
typedef struct
{
  /** @brief Stores the LAPIC CPU identifier. */
  uint8_t cpuId;
  /** @brief Stores the LAPIC identifier.  */
  uint8_t lapicId;
  /** @brief Stores the LAPIC configuration flags. */
  uint32_t flags;
} S_LAPICDescriptor;

/** @brief APIC interrupt override custom descriptor */
typedef struct {
  /** @brief Interrupt override bus */
  uint8_t bus;
  /** @brief Interrupt override source */
  uint8_t source;
  /** @brief Interrupt override destination */
  uint32_t interrupt;
  /** @brief Interrupt override flags */
  uint16_t flags;
} S_InterruptOverrideDescriptor;

/** @brief HPET custom descriptor. */
typedef struct
{
  /** @brief Hardware revision ID */
  uint8_t hwRev;
  /** @brief Comparator count */
  uint8_t comparatorCount;
  /** @brief Counter size */
  uint8_t counterSize;
  /** @brief Legacy replacement IRQ routine table */
  uint8_t legacyRepIRQ;
  /** @brief PCI vendor ID */
  uint16_t pciVendorId;
  /** @brief HPET sequence number */
  uint8_t hpetNumber;
  /** @brief Minimum number of ticks supported in periodic mode */
  uint16_t minimumTick;
  /** @brief Page protection attribute */
  uint8_t pageProtection;
  /** @brief HPET base address */
  uintptr_t address;
  /** @brief Address space identifier */
  uint8_t addressSpace;
  /** @brief Bit width */
  uint8_t bitWidth;
  /** @brief Bit offset */
  uint8_t bitOffset;
  /** @brief Access size */
  uint8_t accessSize;
} S_HPETDescriptor;

/** @brief IO APIC node. */
typedef struct S_IOAPICNode
{
  /** @brief IO APIC descriptor */
  S_IOACPIDescriptor ioApic;
  /** @brief Pointer to the next IO APIC node */
  struct S_IOAPICNode* pNext;
} S_IOAPICNode;

/** @brief CPU LAPIC node. */
typedef struct S_LAPICNode
{
  /** @brief LAPIC descriptor */
  S_LAPICDescriptor lapic;
  /** @brief Pointer to the next LAPIC node */
  struct S_LAPICNode* pNext;
} S_LAPICNode;

/** @brief Interrupt override node. */
typedef struct S_InterruptOverrideNode
{
  /** @brief Interrupt override descriptor */
  S_InterruptOverrideDescriptor intOverride;
  /** @brief Pointer to the next override node */
  struct S_InterruptOverrideNode* pNext;
} S_InterruptOverrideNode;

/** @brief HPET node. */
typedef struct S_HPETNode
{
  /** @brief HPET descriptor */
  S_HPETDescriptor hpet;
  /** @brief Pointer to the next HPET node */
  struct S_HPETNode* pNext;
} S_HPETNode;


/** @brief x86 ACPI driver. */
typedef struct
{
  /**
   * @brief Returns the number of LAPIC detected in the system.
   *
   * @details Returns the number of LAPIC detected in the system.
   *
   * @return The number of LAPIC detected in the system is returned.
   */
  uint8_t (*pGetLAPICCount)(void);

  /**
   * @brief Returns the list of detected LAPICs.
   *
   * @details Returns the list of detected LAPICs. This list should not be
   * modified and is generated during the attach of the ACPI while parsing
   * its tables.
   *
   * @return The list of detected LAPICs descriptors is returned.
   */
  const S_LAPICNode* (*pGetLAPICList)(void);

  /**
   * @brief Returns the detected LAPIC base address.
   *
   * @details Returns the detected LAPIC base address.
   *
   * @return The detected LAPIC base address is returned.
   */
  uintptr_t (*pGetLAPICBaseAddress)(void);

  /**
   * @brief Returns the number of IO-APIC detected in the system.
   *
   * @details Returns the number of IO-APIC detected in the system.
   *
   * @return The number of IO-APIC detected in the system is returned.
   */
  uint8_t (*pGetIOAPICCount)(void);

  /**
   * @brief Returns the list of detected IO-APICs.
   *
   * @details Returns the list of detected IO-APICs. This list should not be
   * modified and is generated during the attach of the ACPI while parsing
   * its tables.
   *
   * @return The list of detected IO-APICs descriptors is returned.
   */
  const S_IOAPICNode* (*pGetIOAPICList)(void);

  /**
   * @brief Returns the list of detected HPETs.
   *
   * @details Returns the list of detected HPETs. This list should not be
   * modified and is generated during the attach of the ACPI while parsing
   * its tables.
   *
   * @return The list of detected HPETs descriptors is returned.
   */
  const S_HPETNode* (*pGetHPETList)(void);

  /**
   * @brief Checks if the IRQ has been remaped in the IO-APIC structure.
   *
   * @details Checks if the IRQ has been remaped in the IO-APIC structure. This
   * function must be called after the init_acpi function.
   *
   * @param[in] kIRQNumber The initial IRQ number to check.
   *
   * @return The remapped IRQ number corresponding to the irq number given as
   * parameter.
   */
  uint32_t (*pGetRemapedIRQ)(const uint32_t kIRQNumber);
} S_ACPIDriver;

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

#endif /* #ifndef __X86_ACPI_H_ */

/************************************ EOF *************************************/