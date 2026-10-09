/*******************************************************************************
 * @file PCIExpress.h
 *
 * @see PCIExpress.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 08/10/2026
 *
 * @version 1.0
 *
 * @brief Peripheral Component Interconnect Express (PCIe) driver.
 *
 * @details PCIe (Peripheral Component Interconnect Express) driver. This driver
 * provides basic access to the PCIe bus and its features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_PCIEXPRESS_H_
#define __X86_PCIEXPRESS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief PCI function descriptor.
 */
typedef struct
{
  /** @brief The vendor ID of the PCI function. */
  uint16_t vendorID;
  /** @brief The device ID of the PCI function. */
  uint16_t deviceID;
  /** @brief The command register of the PCI function. */
  uint16_t command;
  /** @brief The status register of the PCI function. */
  uint16_t status;
  /** @brief The revision ID of the PCI function. */
  uint8_t  revisionID;
  /** @brief The programming interface of the PCI function. */
  uint8_t  progIF;
  /** @brief The subclass of the PCI function. */
  uint8_t  subclass;
  /** @brief The class code of the PCI function. */
  uint8_t  classCode;
  /** @brief The cache line size of the PCI function. */
  uint8_t  cacheLineSize;
  /** @brief The latency timer of the PCI function. */
  uint8_t  latencyTimer;
  /** @brief The header type of the PCI function. */
  uint8_t  headerType;
  /** @brief The bist of the PCI function. */
  uint8_t  bist;
  /** @brief The base address registers of the PCI function. */
  uint32_t bar0;
  /** @brief The base address registers of the PCI function. */
  uint32_t bar1;
  /** @brief The base address registers of the PCI function. */
  uint32_t bar2;
  /** @brief The base address registers of the PCI function. */
  uint32_t bar3;
  /** @brief The base address registers of the PCI function. */
  uint32_t bar4;
  /** @brief The base address registers of the PCI function. */
  uint32_t bar5;
  /** @brief The cardbus CIS pointer of the PCI function. */
  uint32_t cardbusCISPointer;
  /** @brief The subsystem vendor ID of the PCI function. */
  uint16_t subsystemVendorID;
  /** @brief The subsystem ID of the PCI function. */
  uint16_t subsystemID;
  /** @brief The expansion ROM base address of the PCI function. */
  uint32_t expansionROMBaseAddress;
  /** @brief The capabilities pointer of the PCI function. */
  uint8_t  capabilitiesPointer;
} __attribute__((packed)) S_PCIFunction;

/** @brief Structure representing an individual entry in the MSI-X Table */
typedef struct
{
  /** @brief The lower 32 bits of the message address. */
  uint32_t msgAddrLower;
  /** @brief The upper 32 bits of the message address. */
  uint32_t msgAddrUpper;
  /** @brief The message data. */
  uint32_t msgData;
  /** @brief The vector control register. */
  uint32_t vectorControl;
} __attribute__((packed)) S_MSIXTableEntry;

/**
 * @brief PCI device descriptor.
 */
typedef struct S_PCIDevice
{
  /** @brief The name of the PCI device class. */
  const char* pClassName;
  /** @brief The name of the PCI device subclass. */
  const char* pSubClassName;
  /** @brief The name of the PCI device programming interface. */
  const char* pProgIFName;
  /** @brief The PCI function descriptor. */
  S_PCIFunction* pFunction;
  /** @brief The PCI driver attached to the device. */
  void* pDriver;
  /** @brief The base address of the MSI interrupt for the device. */
  uintptr_t msiBaseAddr;
  /** @brief The base address of the MSI X interrupt for the device. */
  uintptr_t msiXBaseAddr;
  /** @brief Pointer to the MSI-X table for the device. */
  S_MSIXTableEntry* pMSIXTable;
  /** @brief Indicates if MSI is enabled for the device. */
  bool isMSIEnabled;
  /** @brief Indicates if MSIX is enabled for the device. */
  bool isMSIXEnabled;
  /** @brief Next device in the list */
  struct S_PCIDevice* pNext;
} S_PCIDevice;

/** @brief Defines the generic definition for a PCI driver used in the kernel */
typedef struct
{
  /** @brief Driver's name. */
  const char* pName;
  /** @brief Driver's description. */
  const char* pDescription;
  /** @brief Driver's version. */
  const char* pVersion;
  /** @brief Driver's device ID. */
  uint16_t deviceID;
  /** @brief Driver's vendor ID. */
  uint16_t vendorID;
  /**
   * @brief Driver's attatch function.
   *
   * @details Driver's attatch function. This function is called when a device
   * is detected and is compatible with the driver.
   * It should initialize the driver and / or device.
   *
   * @param[in] kpPCIDevice The PCI node that matches the driver.
   *
   * @return The functon should return the error state of the driver.
   */
  E_Return (*pDriverAttach)(const S_PCIDevice* kpPCIDevice);

  /**
   * @brief Driver's dettach function.
   *
   * @details Driver's dettach function. This function is called when a device
   * is detached or shutdown after being attached.
   * It should deinitialize the driver and / or device.
   *
   * @param[in] kpPCIDevice The PCI node that matches the driver.
   *
   * @return The functon should return the error state of the driver.
   */
  E_Return (*pDriverDettach)(const S_PCIDevice* kpPCIDevice);

  /**
   * @brief Driver's interrupt notification function.
   *
   * @details Driver's interrupt notification function. This function is called
   * when a device generates an interrupt and is compatible with the driver.
   * It should handle the interrupt and / or device.
   *
   * @param[in] kpPCIDevice The PCI node that matches the driver.
   */
  void (*pNotifyInterrupt)(void);
} S_PCIDriver;

/** @brief PCI BAR type. */
typedef enum
{
  /** @brief PCIe BAR is of type I/O. */
  PCI_BAR_TYPE_IO,
  /** @brief PCIe BAR is of type memory (32-bit). */
  PCI_BAR_TYPE_MEM32,
  /** @brief PCIe BAR is of type memory (64-bit). */
  PCI_BAR_TYPE_MEM64,
  /** @brief PCIe BAR is of unknown type. */
  PCI_BAR_TYPE_UNKNOWN
} E_PCIBARType;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Registers a new PCI driver.
 *
 * @details Registers a new driver in the kernel's PCI driver table.
 *
 * @param[in] DRIVER The driver to add to the PCI driver table.
 */
#define DRIVERMGR_REG_PCI(DRIVER)                                              \
    S_PCIDriver* DRVENT_##DRIVER __attribute__ ((section (".roos_pci_tbl"))) = \
        &DRIVER

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
 * @brief Gets the size of a PCI BAR.
 *
 * @details Gets the size of a PCI BAR. This function will read the BAR register
 * of the PCI function and determine the size of the BAR.
 *
 * @param[in] kpFunction The PCI function to get the BAR size for.
 * @param[in] kBARIndex The index of the BAR to get the size for.
 *
 * @return The function returns the size of the BAR.
 */
size_t PCIGetBARSize(const S_PCIFunction* kpFunction, const uint8_t kBARIndex);

/**
 * @brief Gets the address of a PCI BAR.
 *
 * @details Gets the address of a PCI BAR. This function will read the BAR
 * register of the PCI function and return the address of the BAR.
 *
 * @param[in] kpFunction The PCI function to get the BAR address for.
 * @param[in] kBARIndex The index of the BAR to get the address for.
 *
 * @return The functions returns the address of the BAR.
 */
uintptr_t PCIGetBARAddress(const S_PCIFunction* kpFunction,
                           const uint8_t        kBARIndex);

/**
 * @brief Gets the type of a PCI BAR.
 *
 * @details Gets the type of a PCI BAR. This function will read the BAR register
 * of the PCI function and return the type of the BAR.
 *
 * @param[in] kBarValue The value of the BAR to get the type for.
 *
 * @return The function returns the type of the BAR.
 */
E_PCIBARType PCIGetBARType(const uint32_t kBarValue);

/**
 * @brief Sets up the interrupts for the PCIe device.
 *
 * @details This function sets up the interrupts for the PCIe device.
 * It will read the interrupt line and interrupt pin of the PCI function and
 * configure the interrupt controller to handle the interrupts for the device.
 *
 * @param[in, out] pDevice The PCI device to set up interrupts for.
 * @param[in] kInterruptLine The interrupt line to set up for the device.
 * @param[in] kIsEnabled Whether the interrupt is enabled.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
E_Return PCISetupInterruptMSI(S_PCIDevice*   pDevice,
                              const uint32_t kInterruptLine,
                              const bool     kIsEnabled);

/**
 * @brief Sets up the interrupts for the PCIe device.
 *
 * @details This function sets up the interrupts for the PCIe device.
 * It will read the interrupt line and interrupt pin of the PCI function and
 * configure the interrupt controller to handle the interrupts for the device.
 *
 * @param[in, out] pDevice The PCI device to set up interrupts for.
 * @param[in] kInterruptLine The interrupt line to set up for the device.
 * @param[in] kVector The interrupt vector to set up for the device.
 * @param[in] kIsEnabled Whether the interrupt is enabled.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
E_Return PCISetupInterruptMSIX(S_PCIDevice*   pDevice,
                               const uint32_t kInterruptLine,
                               const uint8_t  kVector,
                               const bool     kIsEnabled);

#endif /* #ifndef __X86_PCIEXPRESS_H_ */

/************************************ EOF *************************************/