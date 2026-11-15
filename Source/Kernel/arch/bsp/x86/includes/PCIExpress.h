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
#include <MMIO.h>
#include <stdint.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Capability ID for Power Management */
#define PCI_CAPABILITY_ID_POWERMGNT 0x01
/** @brief Capability ID for AGP Support */
#define PCI_CAPABILITY_ID_AGP 0x02
/** @brief Capability ID for VPD */
#define PCI_CAPABILITY_ID_VPD 0x03
/** @brief Capability ID for Slot Identification */
#define PCI_CAPABILITY_ID_SLOTID 0x04
/** @brief Capability ID for MSI */
#define PCI_CAPABILITY_ID_MSI 0x05
/** @brief Capability ID for CompactPCI Hot Swap */
#define PCI_CAPABILITY_ID_CPCI_HOTSWAP 0x06
/** @brief Capability ID for PCI-X */
#define PCI_CAPABILITY_ID_PCIX 0x07
/** @brief Capability ID for HyperTransport */
#define PCI_CAPABILITY_ID_HT 0x08
/** @brief Capability ID for Vendor-Specific */
#define PCI_CAPABILITY_ID_VENDOR 0x09
/** @brief Capability ID for Debug Port */
#define PCI_CAPABILITY_ID_DEBUGPORT 0x0A
/** @brief Capability ID for CompactPCI Central Resource Control */
#define PCI_CAPABILITY_ID_CPCI_CRC 0x0B
/** @brief Capability ID for PCI Hot-Plug */
#define PCI_CAPABILITY_ID_HOTPLUG 0x0C
/** @brief Capability ID for Bridge Subsystem Vendor ID */
#define PCI_CAPABILITY_ID_BRIDGE_SUBSYS 0x0D
/** @brief Capability ID for AGP 8x */
#define PCI_CAPABILITY_ID_AGP8X 0x0E
/** @brief Capability ID for Secure Device */
#define PCI_CAPABILITY_ID_SECUREDEV 0x0F
/** @brief Capability ID for PCI-Express */
#define PCI_CAPABILITY_ID_PCIE 0x10
/** @brief Capability ID for MSI-X */
#define PCI_CAPABILITY_ID_MSIX 0x11
/** @brief Capability ID for SATA Data/Index Configuration */
#define PCI_CAPABILITY_ID_SATA 0x12
/** @brief Capability ID for Advanced Features */
#define PCI_CAPABILITY_ID_AF 0x13
/** @brief Capability ID for Enhanced Allocation */
#define PCI_CAPABILITY_ID_ENHANCED_ALLOC 0x14


/* --- PCI Power Management Capability Register Offsets & Masks --- */
/** @brief Power Management Control/Status register */
#define PCI_PM_CTRL_STATUS_OFFSET   0x04
/** @brief Power state mask */
#define PCI_PM_CTRL_STATE_MASK      0x0003
/** @brief Power state D0 (Active) */
#define PCI_PM_CTRL_STATE_D0        0x0000
/** @brief Power state D1 (Low-Power) */
#define PCI_PM_CTRL_STATE_D1        0x0001
/** @brief Power state D2 (Low-Power) */
#define PCI_PM_CTRL_STATE_D2        0x0002
/** @brief Power state D3hot (Low-Power) */
#define PCI_PM_CTRL_STATE_D3HOT     0x0003

/* --- PCI Express Capability Register Offsets & Masks --- */
/** @brief Device Control register */
#define PCI_PCIE_DEV_CTRL_OFFSET    0x08
/** @brief Device Status register */
#define PCI_PCIE_DEV_STATUS_OFFSET  0x0A
/** @brief Link Control register */
#define PCI_PCIE_LINK_CTRL_OFFSET   0x10
/** @brief Link Status register */
#define PCI_PCIE_LINK_STATUS_OFFSET 0x12

/* --- PCIe Link Control ASPM Bits --- */
/** @brief ASPM Control bits mask */
#define PCI_PCIE_LCTL_ASPM_MASK     0x0003
/** @brief L0s Entry Enabled */
#define PCI_PCIE_LCTL_ASPM_L0S      0x0001
/** @brief L1 Entry Enabled */
#define PCI_PCIE_LCTL_ASPM_L1       0x0002

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
  /** @brief The reserved bytes of the PCI function. */
  uint8_t  reserved[7];
  /** @brief The interrupt line of the PCI function. */
  uint8_t  interruptLine;
  /** @brief The interrupt pin of the PCI function. */
  uint8_t  interruptPin;
  /** @brief The minimum grant of the PCI function. */
  uint8_t  minimumGrant;
  /** @brief The maximum latency of the PCI function. */
  uint8_t  maximumLatency;
} __attribute__((packed)) S_PCIFunction;

/** @brief Structure representing an individual entry in the MSI-X Table */
typedef struct
{
  /** @brief The lower 32 bits of the message address. */
  uint32_t msgAddrLow;
  /** @brief The upper 32 bits of the message address. */
  uint32_t msgAddrHigh;
  /** @brief The message data. */
  uint32_t msgData;
  /** @brief The vector control register. */
  uint32_t vectorControl;
} __attribute__((packed)) S_MSIXTableEntry;

/**
 * @brief PCI device descriptor.
 */
struct S_PCIDriver;
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
  struct S_PCIDriver* pDriver;
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
  /** @brief The bus number of the device. */
  uint8_t bus;
  /** @brief The slot number of the device. */
  uint8_t slot;
  /** @brief The function number of the device. */
  uint8_t function;
  /** @brief Indicates if the device is a multi-function device. */
  bool isMultiFunction;
  /** @brief The IRQ pin for the device. */
  uint8_t irqLine;
  /** @brief Next device in the list */
  struct S_PCIDevice* pNext;
} S_PCIDevice;

/** @brief Defines the generic definition for a PCI driver used in the kernel */
typedef struct S_PCIDriver
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
  /** @brief Driver's device class. */
  uint8_t devClass;
  /** @brief Driver's device subclass. */
  uint8_t devSubClass;
  /** @brief Driver's device programming interface. */
  uint8_t devProgIf;

  /** @brief Driver's internal data. */
  void* pDriverData;

  /**
   * @brief Driver's attatch function.
   *
   * @details Driver's attatch function. This function is called when a device
   * is detected and is compatible with the driver.
   * It should initialize the driver and / or device.
   *
   * @param[in] pPCIDevice The PCI node that matches the driver.
   *
   * @return The functon should return the error state of the driver.
   */
  E_Return (*pDriverAttach)(S_PCIDevice* pPCIDevice);

  /**
   * @brief Driver's dettach function.
   *
   * @details Driver's dettach function. This function is called when a device
   * is detached or shutdown after being attached.
   * It should deinitialize the driver and / or device.
   *
   * @param[in] pPCIDevice The PCI node that matches the driver.
   *
   * @return The functon should return the error state of the driver.
   */
  E_Return (*pDriverDettach)(S_PCIDevice* pPCIDevice);
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
 * @param[in] kCoreId The core ID to set up for the device.
 * @param[in] kIsEnabled Whether the interrupt is enabled.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
E_Return PCISetupInterruptMSI(S_PCIDevice*   pDevice,
                              const uint32_t kInterruptLine,
                              const uint32_t kCoreId,
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
 * @param[in] kCoreId The core ID to set up for the device.
 * @param[in] kIsEnabled Whether the interrupt is enabled.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
E_Return PCISetupInterruptMSIX(S_PCIDevice*   pDevice,
                               const uint32_t kInterruptLine,
                               const uint8_t  kVector,
                               const uint32_t kCoreId,
                               const bool     kIsEnabled);

/**
 * @brief Prepares the interrupts for the PCIe device.
 *
 * @details This function prepares the interrupts for the PCIe device. It will
 * check for the MSI or MSI-X capabilities of the device and configure the
 * interrupt controller accordingly.
 *
 * @param[in, out] pDevice The PCI device to prepare interrupts for.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
E_Return PrepareDeviceMSIInterrupt(S_PCIDevice* pDevice);

/**
 * @brief Prepares the interrupts for the legacy PCI device.
 *
 * @details This function prepares the interrupts for the legacy PCI device.
 * It will enable the interrupt in the IOAPIC and configure the interrupt
 * controller accordingly.
 *
 * @param[in, out] pDevice The PCI device to prepare interrupts for.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
E_Return PrepareDeviceLegacyInterrupt(S_PCIDevice* pDevice);

/**
 * @brief Reads a 32-bit value from the PCI configuration space.
 *
 * @details This function reads a 32-bit value from the PCI configuration space
 * at the specified offset.
 *
 * @param[in] kpDevice The PCI device to read from.
 * @param[in] kOffset The offset to read from.
 *
 * @return The function returns the value read from the PCI configuration space.
 */
uint32_t PCIReadConfig32(const S_PCIDevice* kpDevice, const uint16_t kOffset);

/**
 * @brief Writes a 32-bit value to the PCI configuration space.
 *
 * @details This function writes a 32-bit value to the PCI configuration space
 * at the specified offset.
 *
 * @param[in] kpDevice The PCI device to write to.
 * @param[in] kOffset The offset to write to.
 * @param[in] kValue The value to write.
 */
void PCIWriteConfig32(const S_PCIDevice* kpDevice,
                      const uint16_t     kOffset,
                      const uint32_t     kValue);

/**
 * @brief Reads a 16-bit value from the PCI configuration space.
 *
 * @details This function reads a 16-bit value from the PCI configuration space
 * at the specified offset.
 *
 * @param[in] kpDevice The PCI device to read from.
 * @param[in] kOffset The offset to read from.
 *
 * @return The function returns the value read from the PCI configuration space.
 */
uint16_t PCIReadConfig16(const S_PCIDevice* kpDevice, const uint16_t kOffset);

/**
 * @brief Writes a 16-bit value to the PCI configuration space.
 *
 * @details This function writes a 16-bit value to the PCI configuration space
 * at the specified offset.
 *
 * @param[in] kpDevice The PCI device to write to.
 * @param[in] kOffset The offset to write to.
 * @param[in] kValue The value to write.
 */
void PCIWriteConfig16(const S_PCIDevice* kpDevice,
                      const uint16_t     kOffset,
                      const uint16_t     kValue);

/**
 * @brief Reads an 8-bit value from the PCI configuration space.
 *
 * @details This function reads an 8-bit value from the PCI configuration space
 * at the specified offset.
 *
 * @param[in] kpDevice The PCI device to read from.
 * @param[in] kOffset The offset to read from.
 *
 * @return The function returns the value read from the PCI configuration space.
 */
uint8_t PCIReadConfig8(const S_PCIDevice* kpDevice, const uint16_t kOffset);

/**
 * @brief Writes an 8-bit value to the PCI configuration space.
 *
 * @details This function writes an 8-bit value to the PCI configuration space
 * at the specified offset.
 *
 * @param[in] kpDevice The PCI device to write to.
 * @param[in] kOffset The offset to write to.
 * @param[in] kValue The value to write.
 */
void PCIWriteConfig8(const S_PCIDevice* kpDevice,
                     const uint16_t     kOffset,
                     const uint8_t      kValue);

/**
 * @brief Locates a standard PCI capability structure in configuration space
 * (0x40 - 0xFC).
 *
 * @details This function searches for a specific standard PCI capability in the
 * configuration space of a given PCI device. It returns the byte offset of the
 * capability if found, or 0 if not found. The search is performed in the range
 * of 0x40 to 0xFC, which is the valid range for standard PCI capabilities in
 * PCI devices.
 *
 * @param[in] kpDevice The target PCI/PCIe device descriptor.
 * @param[in] kCapId The 8-bit Capability ID to search for.
 *
 * @return The byte offset of the capability header, or 0 if unsupported or not
 * found.
 */
uint8_t PCIFindCapability(const S_PCIDevice* kpDevice, const uint8_t kCapId);

/**
 * @brief Finds the offset of a PCIe Extended Capability in ECAM space
 * (0x100 - 0xFFF).
 *
 * @details This function searches for a specific Extended Capability in the
 * PCIe configuration space of a given PCIe device. It returns the byte offset
 * of the capability if found, or 0 if not found. The search is performed in the
 * range of 0x100 to 0xFFC, which is the valid range for Extended Capabilities
 * in PCIe devices.
 *
 * @param[in] kpDevice The target PCIe device.
 * @param[in] kExtCapId The 16-bit Extended Capability ID to locate.
 *
 * @return The 12-bit byte offset of the capability (0x100 - 0xFFC), or 0 if not
 * found.
 */
uint16_t PCIFindExtendedCapability(const S_PCIDevice* kpDevice,
                                   const uint16_t     kExtCapId);

#endif /* #ifndef __X86_PCIEXPRESS_H_ */

/************************************ EOF *************************************/