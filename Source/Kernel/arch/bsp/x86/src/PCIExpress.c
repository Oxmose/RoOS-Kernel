/*******************************************************************************
 * @file PCIExpress.c
 *
 * @see PCIExpress.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <ACPI.h>
#include <MMIO.h>
#include <Panic.h>
#include <X64Cpu.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <Memory.h>
#include <Interrupts.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <PCIExpress.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief FDT property for acpi handle */
#define PCIE_FDT_ACPI_NODE_PROP "acpi-node"

/** @brief Maximum number of PCIe buses */
#define PCIE_MAX_BUS_COUNT 256
/** @brief Size of the PCIe bus space */
#define PCIE_BUS_SIZE 0x100000
/** @brief Maximum number of devices per PCIe bus */
#define PCIE_MAX_DEVICES_PER_BUS 32
/** @brief Maximum number of functions per PCIe device */
#define PCIE_MAX_FUNCTIONS_PER_DEVICE 8

/** @brief Size of the PCIe class code table */
#define PCIE_CLASS_TABLE_SIZE 0x14
/** @brief Class code for co-processor */
#define PCIE_CLASS_CO_PROCESSOR 0x40

/** @brief Size of the PCIe class 0 table */
#define PCIE_CLASS0_TABLE_SIZE 2
/** @brief Size of the PCIe class 1 table */
#define PCIE_CLASS1_TABLE_SIZE 9
/** @brief Size of the PCIe class 2 table */
#define PCIE_CLASS2_TABLE_SIZE 9
/** @brief Size of the PCIe class 3 table */
#define PCIE_CLASS3_TABLE_SIZE 3
/** @brief Size of the PCIe class 4 table */
#define PCIE_CLASS4_TABLE_SIZE 4
/** @brief Size of the PCIe class 5 table */
#define PCIE_CLASS5_TABLE_SIZE 2
/** @brief Size of the PCIe class 6 table */
#define PCIE_CLASS6_TABLE_SIZE 11
/** @brief Size of the PCIe class 7 table */
#define PCIE_CLASS7_TABLE_SIZE 6
/** @brief Size of the PCIe class 8 table */
#define PCIE_CLASS8_TABLE_SIZE 7
/** @brief Size of the PCIe class 9 table */
#define PCIE_CLASS9_TABLE_SIZE 5
/** @brief Size of the PCIe class A table */
#define PCIE_CLASSA_TABLE_SIZE 1
/** @brief Size of the PCIe class B table */
#define PCIE_CLASSB_TABLE_SIZE 65
/** @brief Size of the PCIe class C table */
#define PCIE_CLASSC_TABLE_SIZE 10
/** @brief Size of the PCIe class D table */
#define PCIE_CLASSD_TABLE_SIZE 34
/** @brief Size of the PCIe class E table */
#define PCIE_CLASSE_TABLE_SIZE 1
/** @brief Size of the PCIe class F table */
#define PCIE_CLASSF_TABLE_SIZE 5
/** @brief Size of the PCIe class 10 table */
#define PCIE_CLASS10_TABLE_SIZE 17
/** @brief Size of the PCIe class 11 table */
#define PCIE_CLASS11_TABLE_SIZE 33

/** @brief Bit mask for the I/O space enable bit in the PCI command */
#define PCI_COMMAND_IO_SPACE_ENABLE 0x0001
/** @brief Bit mask for the memory space enable bit in the PCI command */
#define PCI_COMMAND_MEMORY_SPACE_ENABLE 0x0002
/** @brief Bit mask for the bus master enable bit in the PCI command */
#define PCI_COMMAND_BUS_MASTER_ENABLE 0x0004
/** @brief Bit mask for the legacy interrupt disable bit in the PCI command */
#define PCI_COMMAND_LEGACY_INTERRUPT_DISABLE 0x400

/** @brief Bit mask for the capabilities list bit in the PCI status register */
#define PCI_STATUS_CAPABILITIES_LIST 0x0010
/** @brief Capability ID for MSI */
#define PCI_CAPABILITY_ID_MSI 0x05
/** @brief Capability ID for MSI-X */
#define PCI_CAPABILITY_ID_MSIX 0x11
/** @brief MSI control bit for enabling MSI */
#define PCIE_MSI_CONTROL_ENABLE 0x0001
/** @brief MSI control bit for 64-bit address support */
#define PCIE_MSI_CONTROL_64BIT 0x0080
/** @brief MSI-X control bit for enabling MSI-X */
#define PCIE_MSIX_CONTROL_ENABLE 0x8000
/** @brief MSI-X control bit for masking MSI-X */
#define PCIE_MSIX_CONTROL_MASK 0x4000

/** @brief Maximum number of PCIe BARS */
#define PCI_MAX_BARS 6

/** @brief Current module name */
#define MODULE_NAME "X86 PCIE"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the PCIE to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the PCIE to ensure correctness of
 * execution. Due to the critical nature of the PCIE, any error generates
 * a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define PCIE_ASSERT(COND, MSG, ERROR) {                   \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/**
 * @brief Gets the address of the PCIe configuration space for a given bus,
 * device, and function.
 *
 * @details This macro calculates the address of the PCIe configuration space
 * for a given bus, device, and function.
 * The address is calculated based on the base address of the PCIe configuration
 * space and the bus, device, and function numbers.
 *
 * @param[in] START_BUS The starting bus number.
 * @param[in] BUS The bus number.
 * @param[in] DEVICE The device number.
 * @param[in] FUNCTION The function number.
 * @param[in] BASE_ADDRESS The base address of the PCIe configuration space.
 *
 * @return The address of the PCIe configuration space for the given bus,
 * device, and function.
 */
#define GET_FUNCTIONS_SPACE(START_BUS, BUS, DEVICE, FUNCTION, BASE_ADDRESS) \
  ((uintptr_t)BASE_ADDRESS +                                                \
   ((BUS - START_BUS) << 20) +                                              \
   (DEVICE << 15) +                                                         \
   (FUNCTION << 12))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Reads a 32-bit value from the PCIe.
 *
 * @details Reads a 32-bit value from the PCIe. Thus function uses MMIO
 * functions to read the value.
 *
 * @param[in] pAddr The address to read from.
 * @param[in] kSize The size of the value to read.
 *
 * @return The function returns the 32-bit value read from the PCIe.
 */
static inline uint32_t _PCIERead32(const volatile void* pAddr);

/**
 * @brief Reads a 16-bit value from the PCIe.
 *
 * @details Reads a 16-bit value from the PCIe. Thus function uses MMIO
 * functions to read the value.
 *
 * @param[in] pAddr The address to read from.
 * @param[in] kSize The size of the value to read.
 *
 * @return The function returns the 16-bit value read from the PCIe.
 */
static inline uint16_t _PCIERead16(const volatile void* pAddr);

/**
 * @brief Reads a 8-bit value from the PCIe.
 *
 * @details Reads a 8-bit value from the PCIe. Thus function uses MMIO
 * functions to read the value.
 *
 * @param[in] pAddr The address to read from.
 * @param[in] kSize The size of the value to read.
 *
 * @return The function returns the 8-bit value read from the PCIe.
 */
static inline uint8_t _PCIERead8(const volatile void* pAddr);

/**
 * @brief Writes a 32-bit value to the PCIe.
 *
 * @details Writes a 32-bit value to the PCIe. Thus function uses MMIO
 * functions to write the value.
 *
 * @param[in] pAddr The address to write to.
 * @param[in] kValue The value to write.
 */
static inline void _PCIEWrite32(volatile void* pAddr, const uint32_t kValue);

/**
 * @brief Writes a 16-bit value to the PCIe.
 *
 * @details Writes a 16-bit value to the PCIe. Thus function uses MMIO
 * functions to write the value.
 *
 * @param[in] pAddr The address to write to.
 * @param[in] kValue The value to write.
 */
static inline void _PCIEWrite16(volatile void* pAddr, const uint16_t kValue);

/**
 * @brief Writes a 8-bit value to the PCIe.
 *
 * @details Writes a 8-bit value to the PCIe. Thus function uses MMIO
 * functions to write the value.
 *
 * @param[in] pAddr The address to write to.
 * @param[in] kValue The value to write.
 */
static inline void _PCIEWrite8(volatile void* pAddr, const uint8_t kValue);

/**
 * @brief Gets the full class code string for a given class code, subclass, and
 * programming interface.
 *
 * @details This function retrieves the full class code string for a given class
 * code, subclass, and programming interface. It uses the class code and
 * subclass to determine the appropriate string to return.
 *
 * @param[in] kClassCode The class code of the PCIe device.
 * @param[in] kSubclass The subclass of the PCIe device.
 * @param[in] kProgIF The programming interface of the PCIe device.
 * @param[out] pDeviceDesc The PCIe device descriptor to populate with the full
 * class code string.
 */
static void _GetFullClassCodeString(const uint8_t kClassCode,
                                    const uint8_t kSubclass,
                                    const uint8_t kProgIF,
                                    S_PCIDevice*  pDeviceDesc);

/**
 * @brief Gets the full programming interface string for a given class code,
 * subclass, and programming interface.
 *
 * @details This function retrieves the full programming interface string for a
 * given class code, subclass, and programming interface. It uses the class code
 * and subclass to determine the appropriate string to return.
 *
 * @param[in] kClassCode The class code of the PCIe device.
 * @param[in] kSubclass The subclass of the PCIe device.
 * @param[in] kProgIF The programming interface of the PCIe device.
 * @param[out] pDeviceDesc The PCIe device descriptor to populate with the full
 * programming interface string.
 */
static void _GetFullProgIFString(const uint8_t kClassCode,
                                 const uint8_t kSubclass,
                                 const uint8_t kProgIF,
                                 S_PCIDevice*  pDeviceDesc);

/**
 * @brief Probes the PCIe devices.
 *
 * @details This function probes the PCIe devices. It iterates through the list
 * of PCI configuration nodes and initializes the corresponding info drivers for
 * each PCI device found.
 *
 * @param[in] kpPCIConfigList The list of PCI configuration nodes.
 *
 * @return The function returns an E_Return value indicating the success or
 * failure of the operation.
 */
static E_Return _ProbeDevices(const S_PCIConfigNode* kpPCIConfigList);

/**
 * @brief Prepares the interrupts for the PCIe device.
 *
 * @details This function prepares the interrupts for the PCIe device. It will
 * check for the MSI or MSI-X capabilities of the device and configure the
 * interrupt controller accordingly.
 *
 * @param[in, out] pDevice The PCI device to prepare interrupts for.
 */
static void _PrepareDeviceInterrupt(S_PCIDevice* pDevice);

/**
 * @brief Attaches the PCIE driver to the system.
 *
 * @details Attaches the PCIE driver to the system. This function will
 * use the FDT to initialize the PCIE hardware and retreive the PCIE
 * parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the registered kernel PCI drivers table */
extern uintptr_t _START_PCI_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PCIE driver instance. */
static S_Driver sX86PCIEDriver =
{
  .pName         = "X86 PCIE Driver",
  .pDescription  = "X86 PCI Express driver for roOs.",
  .pCompatible   = "x86,x86-pcie",
  .pVersion      = "1.0",
  .pDriverAttach = _Attach
};

/** @brief Pointer to the list of PCIe devices */
static S_PCIDevice* spDevices = NULL;

/** @brief PCIe class code strings */
const char* skpPCIEClassCodeStrings[PCIE_CLASS_TABLE_SIZE] =
{
  "Unclassified",
  "Mass Storage Controller",
  "Network Controller",
  "Display Controller",
  "Multimedia Controller",
  "Memory Controller",
  "Bridge Device",
  "Simple Communication Controller",
  "Base System Peripheral",
  "Input Device Controller",
  "Docking Station",
  "Processor",
  "Serial Bus Controller",
  "Wireless Controller",
  "Intelligent I/O Controller",
  "Satellite Communication Controller",
  "Encryption/Decryption Controller",
  "Data Acquisition and Signal Processing Controller",
  "Processing Accelerator",
  "Non-Essential Instrumentation",
};

/** @brief PCIe Subclass 0 code strings */
const char* skpPCIESubClass0CodeStrings[PCIE_CLASS0_TABLE_SIZE] =
{
  "Non-VGA-Compatible Device",
  "VGA-Compatible Device"
};

/** @brief PCIe Subclass 1 code strings */
const char* skpPCIESubClass1CodeStrings[PCIE_CLASS1_TABLE_SIZE] =
{
  "SCSI Bus Controller",
  "IDE Controller",
  "Floppy Disk Controller",
  "IPI Bus Controller",
  "RAID Controller",
  "ATA Controller",
  "Serial ATA Controller",
  "Serial Attached SCSI Controller",
  "Non-Volatile Memory Controller"
};

/** @brief PCIe Subclass 2 code strings */
const char* skpPCIESubClass2CodeStrings[PCIE_CLASS2_TABLE_SIZE] =
{
  "Ethernet Controller",
  "Token Ring Controller",
  "FDDI Controller",
  "ATM Controller",
  "ISDN Controller",
  "WorldFip Controller",
  "PICMG 2.14 Multi Computing",
  "Infiniband Controller",
  "Fabric Controller"
};

/** @brief PCIe Subclass 3 code strings */
const char* skpPCIESubClass3CodeStrings[PCIE_CLASS3_TABLE_SIZE] =
{
  "VGA-Compatible Controller",
  "XGA Controller",
  "3D Controller"
};

/** @brief PCIe Subclass 4 code strings */
const char* skpPCIESubClass4CodeStrings[PCIE_CLASS4_TABLE_SIZE] =
{
  "Multimedia Video Controller",
  "Multimedia Audio Controller",
  "Computer Telephony Device",
  "Audio Device"
};

/** @brief PCIe Subclass 5 code strings */
const char* skpPCIESubClass5CodeStrings[PCIE_CLASS5_TABLE_SIZE] =
{
  "RAM Controller",
  "Flash Controller"
};

/** @brief PCIe Subclass 6 code strings */
const char* skpPCIESubClass6CodeStrings[PCIE_CLASS6_TABLE_SIZE] =
{
  "Host Bridge",
  "ISA Bridge",
  "EISA Bridge",
  "MCA Bridge",
  "PCI-to-PCI Bridge",
  "PCMCIA Bridge",
  "NuBus Bridge",
  "CardBus Bridge",
  "RACEway Bridge",
  "Semi-transparent PCI-to-PCI Bridge",
  "InfiniBand-to-PCI Host Bridge"
};

/** @brief PCIe Subclass 7 code strings */
const char* skpPCIESubClass7CodeStrings[PCIE_CLASS7_TABLE_SIZE] =
{
  "Serial Controller",
  "Parallel Controller",
  "Multiport Serial Controller",
  "Modem",
  "IEEE 488.1/2 (GPIB) Controller",
  "Smart Card Controller"
};

/** @brief PCIe Subclass 8 code strings */
const char* skpPCIESubClass8CodeStrings[PCIE_CLASS8_TABLE_SIZE] =
{
  "PIC",
  "DMA Controller",
  "Timer",
  "RTC Controller",
  "PCI Hot-Plug Controller",
  "SD Host controller",
  "IOMMU"
};

/** @brief PCIe Subclass 9 code strings */
const char* skpPCIESubClass9CodeStrings[PCIE_CLASS9_TABLE_SIZE] =
{
  "Keyboard Controller",
  "Digitizer Pen",
  "Mouse Controller",
  "Scanner Controller",
  "Gameport Controller"
};

/** @brief PCIe Subclass A code strings */
const char* skpPCIESubClassACodeStrings[PCIE_CLASSA_TABLE_SIZE] =
{
  "Generic Docking Station"
};

/** @brief PCIe Subclass B code strings */
const char* skpPCIESubClassBCodeStrings[PCIE_CLASSB_TABLE_SIZE] =
{
  "386 Processor",
  "486 Processor",
  "Pentium Processor",
  "Pentium Pro Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Alpha Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "PowerPC Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "MIPS Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Unknown Processor",
  "Co-Processor"
};

/** @brief PCIe Subclass C code strings */
const char* skpPCIESubClassCCodeStrings[PCIE_CLASSC_TABLE_SIZE] =
{
  "FireWire (IEEE 1394) Controller",
  "ACCESS Bus Controller",
  "SSA Controller",
  "USB Controller",
  "Fibre Channel Controller",
  "SMBus Controller",
  "InfiniBand Controller",
  "IPMI Interface",
  "SERCOS Interface",
  "CANbus Controller"
};

/** @brief PCIe Subclass D code strings */
const char* skpPCIESubClassDCodeStrings[PCIE_CLASSD_TABLE_SIZE] =
{
  "iRDA Compatible Controller",
  "Consumer IR Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "RF Controller",
  "Bluetooth Controller",
  "Broadband Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Ethernet Controller (802.1a)",
  "Ethernet Controller (802.1b)"
};

/** @brief PCIe Subclass E code strings */
const char* skpPCIESubClassECodeStrings[PCIE_CLASSE_TABLE_SIZE] =
{
  "I20"
};

/** @brief PCIe Subclass F code strings */
const char* skpPCIESubClassFCodeStrings[PCIE_CLASSF_TABLE_SIZE] =
{
  "Satellite TV Controller",
  "Satellite Audio Controller",
  "Satellite Voice Controller",
  "Satellite Data Controller"
};

/** @brief PCIe Subclass 10 code strings */
const char* skpPCIESubClass10CodeStrings[PCIE_CLASS10_TABLE_SIZE] =
{
  "Network and Computing Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Unknown Encrpytion/Decryption",
  "Entertainment Encryption/Decryption"
};

/** @brief PCIe Subclass 11 code strings */
const char* skpPCIESubClass11CodeStrings[PCIE_CLASS11_TABLE_SIZE] =
{
  "DPIO Modules",
  "Performance Counters",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Communication Synchronizer",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Unknown Controller",
  "Signal Processing Management"
};

/** @brief Array of PCIe Subclass code strings */
static const char** const skpPCIESubClassCodeStrings[PCIE_CLASS_TABLE_SIZE] =
{
  skpPCIESubClass0CodeStrings,
  skpPCIESubClass1CodeStrings,
  skpPCIESubClass2CodeStrings,
  skpPCIESubClass3CodeStrings,
  skpPCIESubClass4CodeStrings,
  skpPCIESubClass5CodeStrings,
  skpPCIESubClass6CodeStrings,
  skpPCIESubClass7CodeStrings,
  skpPCIESubClass8CodeStrings,
  skpPCIESubClass9CodeStrings,
  skpPCIESubClassACodeStrings,
  skpPCIESubClassBCodeStrings,
  skpPCIESubClassCCodeStrings,
  skpPCIESubClassDCodeStrings,
  skpPCIESubClassECodeStrings,
  skpPCIESubClassFCodeStrings,
  skpPCIESubClass10CodeStrings,
  skpPCIESubClass11CodeStrings,
  NULL,
  NULL
};

/** @brief Array of PCIe Subclass code sizes */
static const uint32_t skpPCIESubClassCodeSizes[PCIE_CLASS_TABLE_SIZE] =
{
  PCIE_CLASS0_TABLE_SIZE,
  PCIE_CLASS1_TABLE_SIZE,
  PCIE_CLASS2_TABLE_SIZE,
  PCIE_CLASS3_TABLE_SIZE,
  PCIE_CLASS4_TABLE_SIZE,
  PCIE_CLASS5_TABLE_SIZE,
  PCIE_CLASS6_TABLE_SIZE,
  PCIE_CLASS7_TABLE_SIZE,
  PCIE_CLASS8_TABLE_SIZE,
  PCIE_CLASS9_TABLE_SIZE,
  PCIE_CLASSA_TABLE_SIZE,
  PCIE_CLASSB_TABLE_SIZE,
  PCIE_CLASSC_TABLE_SIZE,
  PCIE_CLASSD_TABLE_SIZE,
  PCIE_CLASSE_TABLE_SIZE,
  PCIE_CLASSF_TABLE_SIZE,
  PCIE_CLASS10_TABLE_SIZE,
  PCIE_CLASS11_TABLE_SIZE,
  0,
  0
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static inline uint32_t _PCIERead32(const volatile void* pAddr)
{
  return _MMIORead32(pAddr);
}

static inline void _PCIEWrite32(volatile void* pAddr, const uint32_t kValue)
{
  _MMIOWrite32(pAddr, kValue);
}

static inline uint16_t _PCIERead16(const volatile void* pAddr)
{
  uint32_t  value;
  uintptr_t aligned;
  uint32_t  shift;

  aligned = ((uintptr_t)pAddr & ~0x3);
  shift   = ((uintptr_t)pAddr & 0x3) << 3;

  value = _MMIORead32((void*)aligned);

  return (value >> shift) & 0xFFFF;
}

static inline void _PCIEWrite16(volatile void* pAddr, const uint16_t kValue)
{
  uint32_t  value;
  uintptr_t aligned;
  uint32_t  shift;

  aligned = ((uintptr_t)pAddr & ~0x3);
  shift   = ((uintptr_t)pAddr & 0x3) << 3;

  value = _MMIORead32((void*)aligned);
  value &= ~(0xFFFF << shift);
  value |= ((uint32_t)kValue << shift);

  _MMIOWrite32((void*)aligned, value);
}

static inline uint8_t _PCIERead8(const volatile void* pAddr)
{
  uint32_t  value;
  uintptr_t aligned;
  uint32_t  shift;

  aligned = ((uintptr_t)pAddr & ~0x3);
  shift   = ((uintptr_t)pAddr & 0x3) << 3;

  value = _MMIORead32((void*)aligned);

  return (value >> shift) & 0xFF;
}

static inline void _PCIEWrite8(volatile void* pAddr, const uint8_t kValue)
{
  uint32_t  value;
  uintptr_t aligned;
  uint32_t  shift;

  aligned = ((uintptr_t)pAddr & ~0x3);
  shift   = ((uintptr_t)pAddr & 0x3) << 3;

  value = _MMIORead32((void*)aligned);
  value &= ~(0xFF << shift);
  value |= ((uint32_t)kValue << shift);

  _MMIOWrite32((void*)aligned, value);
}

static void _GetFullProgIFString(const uint8_t kClassCode,
                                 const uint8_t kSubclass,
                                 const uint8_t kProgIF,
                                 S_PCIDevice*  pDeviceDesc)
{
  pDeviceDesc->pProgIFName = "Unknown";
  switch (kClassCode)
  {
    case 1: /* Mass Storage Controller */
      switch (kSubclass)
      {
        case 1: /* IDE Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "ISA Compatibility Mode 0";
              break;
            case 0x05:
              pDeviceDesc->pProgIFName = "PCI Native Mode 5";
              break;
            case 0xA:
              pDeviceDesc->pProgIFName = "ISA Compatibility Mode A";
              break;
            case 0xF:
              pDeviceDesc->pProgIFName = "PCI Native Mode F";
              break;
            case 0x80:
              pDeviceDesc->pProgIFName = "ISA Compatibility Mode 80";
              break;
            case 0x85:
              pDeviceDesc->pProgIFName = "PCI Native Mode 85";
              break;
            case 0x8A:
              pDeviceDesc->pProgIFName = "ISA Compatibility Mode 8A";
              break;
            case 0x8F:
              pDeviceDesc->pProgIFName = "PCI Native Mode 8F";
              break;
            default:
              break;
          }
          break;
        case 5: /* ATA Disk Controller */
          switch (kProgIF)
          {
            case 0x20:
              pDeviceDesc->pProgIFName = "Single DMA";
              break;
            case 0x30:
              pDeviceDesc->pProgIFName = "Chained DMA";
              break;
            default:
              break;
          }
          break;
        case 6: /* Serial ATA Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Vendor Specific Interface";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "AHCI 1.0 Interface";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "Serial Storage Bus Interface";
              break;
            default:
              break;
          }
          break;
        case 7: /* Serial Attached SCSI Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "SAS";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "Serial Storage Bus Interface";
              break;
            default:
              break;
          }
          break;
        case 8: /* Non-Volatile Memory Controller */
          switch (kProgIF)
          {
            case 0x01:
              pDeviceDesc->pProgIFName = "NVMHCI";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "NVM Express";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case 3: /* Display Controller */
      switch (kSubclass)
      {
        case 0: /* VGA-Compatible Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "VGA Controller";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "8514-Compatible Controller";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case 6: /* Bridge Device */
      switch (kSubclass)
      {
        case 4: /* PCI-to-PCI Bridge */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Normal Decode";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "Subtractive Decode";
              break;
            default:
              break;
          }
          break;
        case 8: /* InfiniBand-to-PCI Host Bridge */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Transparent Mode";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "Endpoint Mode";
              break;
            default:
              break;
          }
          break;
        case 9: /* PCI-to-PCI Bridge (Semi-transparent) */
          switch (kProgIF)
          {
            case 0x40:
              pDeviceDesc->pProgIFName = "Semi-Transparent, Primary";
              break;
            case 0x80:
              pDeviceDesc->pProgIFName = "Semi-Transparent, Secondary";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case 7: /* Simple Communication Controller */
      switch (kSubclass)
      {
        case 0: /* Serial Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "8250-Compatible";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "16450-Compatible";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "16550-Compatible";
              break;
            case 0x03:
              pDeviceDesc->pProgIFName = "16650-Compatible";
              break;
            case 0x04:
              pDeviceDesc->pProgIFName = "16750-Compatible";
              break;
            case 0x05:
              pDeviceDesc->pProgIFName = "16850-Compatible";
              break;
            case 0x06:
              pDeviceDesc->pProgIFName = "16950-Compatible";
              break;
            default:
              break;
          }
          break;
        case 1: /* Parallel Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Standard Parallel Port";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "Bi-Directional Parallel Port";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "ECP 1.X Compliant Parallel Port";
              break;
            case 0x03:
              pDeviceDesc->pProgIFName = "IEEE 1284 Controller";
              break;
            case 0xFE:
              pDeviceDesc->pProgIFName = "IEEE 1284 Target Device";
              break;
            default:
              break;
          }
          break;
        case 3: /* Multiport Serial Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Generic Modem";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "Hayes 16450-Compatible Interface";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "Hayes 16550-Compatible Interface";
              break;
            case 0x03:
              pDeviceDesc->pProgIFName = "Hayes 16650-Compatible Interface";
              break;
            case 0x04:
              pDeviceDesc->pProgIFName = "Hayes 16750-Compatible Interface";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case 8: /* Base System Peripheral */
      switch (kSubclass)
      {
        case 0: /* PIC */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "8259-Compatible PIC";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "ISA-Compatible PIC";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "EISA-Compatible PIC";
              break;
            case 0x10:
              pDeviceDesc->pProgIFName = "I/O APIC Interrupt Controller";
              break;
            case 0x20:
              pDeviceDesc->pProgIFName = "I/O xAPIC Interrupt Controller";
              break;
            default:
              break;
          }
          break;
        case 1: /* DMA Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "8237-Compatible DMA Controller";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "ISA-Compatible DMA Controller";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "EISA-Compatible DMA Controller";
              break;
            default:
              break;
          }
          break;
        case 2: /* Timer */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "8254-Compatible Timer";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "ISA-Compatible Timer";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "EISA-Compatible Timer";
              break;
            case 0x03:
              pDeviceDesc->pProgIFName = "HPET";
              break;
            default:
              break;
          }
          break;
        case 3: /* RTC Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "RTC Controller";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "ISA Compatible RTC Controller";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case 9: /* Input Device Controller */
      switch (kSubclass)
      {
        case 4: /* Gameport Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Generic Gameport Controller";
              break;
            case 0x10:
              pDeviceDesc->pProgIFName = "Extended Gameport Controller";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    case 0xC: /* Serial Bus Controller */
      switch (kSubclass)
      {
        case 0: /* FireWire (IEEE 1394) Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "Generic";
              break;
            case 0x10:
              pDeviceDesc->pProgIFName = "OHCI";
              break;
            default:
              break;
          }
          break;
        case 3: /* USB Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "UHCI";
              break;
            case 0x10:
              pDeviceDesc->pProgIFName = "OHCI";
              break;
            case 0x20:
              pDeviceDesc->pProgIFName = "EHCI";
              break;
            case 0x30:
              pDeviceDesc->pProgIFName = "XHCI";
              break;
            case 0x80:
              pDeviceDesc->pProgIFName = "Unspecified";
              break;
            case 0xFE:
              pDeviceDesc->pProgIFName = "USB Device (Not a Host Controller)";
              break;
            default:
              break;
          }
          break;
        case 7: /* IPMI Controller */
          switch (kProgIF)
          {
            case 0x00:
              pDeviceDesc->pProgIFName = "SMIC";
              break;
            case 0x01:
              pDeviceDesc->pProgIFName = "Keyboard Controller Style";
              break;
            case 0x02:
              pDeviceDesc->pProgIFName = "Block Transfer";
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "        Programming Interface: %s",
               pDeviceDesc->pProgIFName);
}

static void _GetFullClassCodeString(const uint8_t kClassCode,
                                    const uint8_t kSubclass,
                                    const uint8_t kProgIF,
                                    S_PCIDevice*  pDeviceDesc)
{
  if (kClassCode < PCIE_CLASS_TABLE_SIZE)
  {
    KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "        Class: %s",
                skpPCIEClassCodeStrings[kClassCode]);

    pDeviceDesc->pClassName = skpPCIEClassCodeStrings[kClassCode];
    if (skpPCIESubClassCodeStrings[kClassCode] != NULL)
    {
      if (kSubclass < skpPCIESubClassCodeSizes[kClassCode])
      {
        KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "        Subclass: %s",
                     skpPCIESubClassCodeStrings[kClassCode][kSubclass]);

        pDeviceDesc->pSubClassName =
          skpPCIESubClassCodeStrings[kClassCode][kSubclass];
        _GetFullProgIFString(kClassCode, kSubclass, kProgIF, pDeviceDesc);
      }
      else
      {

        KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "        Subclass: Unknown");

        pDeviceDesc->pSubClassName = "Unknown";
        pDeviceDesc->pProgIFName = "Unknown";
      }
    }

  }
  else if (kClassCode == PCIE_CLASS_CO_PROCESSOR)
  {
    KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "        Class: Co-Processor");

    pDeviceDesc->pClassName = "Co-Processor";
  }
  else
  {
    KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "        Class: Unknown");

   pDeviceDesc->pClassName = "Unknown";
  }
}

static E_Return _ProbeDevices(const S_PCIConfigNode* kpPCIConfigList)
{
  E_Return               retCode;
  const S_PCIConfigNode* kpNode;
  void*                  mapped;
  size_t                 toMap;
  uint32_t               i;
  uint32_t               j;
  uint32_t               k;
  S_PCIFunction*         pFunction;
  S_PCIDevice*           pDevice;
  uint16_t               vendorID;
  uint16_t               deviceID;
  uint16_t               subsystemID;
  uint16_t               subsystemVendorID;
  uint8_t                classCode;
  uint8_t                subclass;
  uint8_t                progIF;
  uint8_t                headerType;

  if (kpPCIConfigList != NULL)
  {

    kpNode = kpPCIConfigList;
    while (kpNode != NULL)
    {
      KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "    PCIe Base Address: 0x%llx\n"
                    "    PCIe Segment Group Number: %d\n"
                    "    PCIe Start Bus Number: %d\n"
                    "    PCIe End Bus Number: %d",
                    kpNode->pciConfig.baseAddress,
                    kpNode->pciConfig.pciSegmentGroupNumber,
                    kpNode->pciConfig.startBusNumber,
                    kpNode->pciConfig.endBusNumber);

      /* Map the bus */
      toMap = (kpNode->pciConfig.endBusNumber -
               kpNode->pciConfig.startBusNumber + 1) * PCIE_BUS_SIZE;
      mapped = MemoryKernelMap((void*)kpNode->pciConfig.baseAddress,
                               toMap,
                               MEMMGR_MAP_HARDWARE |
                               MEMMGR_MAP_KERNEL   |
                               MEMMGR_MAP_RW,
                               &retCode);
      if (mapped != NULL && retCode == NO_ERROR)
      {
        for (i = kpNode->pciConfig.startBusNumber;
             i <= kpNode->pciConfig.endBusNumber;
             ++i)
        {
          for (j = 0; j < PCIE_MAX_DEVICES_PER_BUS; ++j)
          {
            for (k = 0; k < PCIE_MAX_FUNCTIONS_PER_DEVICE; ++k)
            {
              pFunction = (S_PCIFunction*)GET_FUNCTIONS_SPACE(
                                          kpNode->pciConfig.startBusNumber,
                                          i,
                                          j,
                                          k,
                                          mapped);
              /* Read the vendor ID */
              vendorID = _PCIERead16(&pFunction->vendorID);
              if (vendorID != 0xFFFF)
              {
                deviceID = _PCIERead16(&pFunction->deviceID);
                subsystemID = _PCIERead16(&pFunction->subsystemID);
                subsystemVendorID = _PCIERead16(&pFunction->subsystemVendorID);
                pDevice = KMalloc(sizeof(S_PCIDevice), KMALLOC_NO_FREE_POOL);
                memset(pDevice, 0, sizeof(S_PCIDevice));
                KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                             MODULE_NAME,
                             "    PCIe Device: Bus/Dev/Fun: %d/%d/%d",
                             i,
                             j,
                             k);
                KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                             MODULE_NAME,
                             "        \\VEN_%04x&\\DEV_%04x&\\SUBSYS_%04x&\\"
                             "SUBSYS_VEN_%04x",
                             vendorID,
                             deviceID,
                             subsystemID,
                             subsystemVendorID);

                /* Initialize the PCIe device descriptor */
                classCode = _PCIERead8(&pFunction->classCode);
                subclass = _PCIERead8(&pFunction->subclass);
                progIF = _PCIERead8(&pFunction->progIF);

                pDevice->pFunction     = pFunction;
                pDevice->msiBaseAddr   = 0;
                pDevice->msiXBaseAddr  = 0;
                pDevice->isMSIEnabled  = false;
                pDevice->isMSIXEnabled = false;
                pDevice->pDriver       = NULL;
                _GetFullClassCodeString(classCode, subclass, progIF, pDevice);

                pDevice->pNext = spDevices;
                spDevices = pDevice;
              }

              if (k == 0)
              {
                /* Check if this is a multi-function device */
                headerType = _PCIERead8(&pFunction->headerType);
                if ((headerType & 0x80) == 0)
                {
                  /* Not a multi-function device */
                  break;
                }
              }
            }
          }
        }
      }

      kpNode = kpNode->pNext;
    }
    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_VALUE;
  }

  return retCode;
}

static void _PrepareDeviceInterrupt(S_PCIDevice* pDevice)
{
  E_Return  retCode;
  uint16_t  status;
  uint8_t   capabilitiesPtr;
  uint32_t  capabilityHeader;
  uint8_t   capabilityId;
  uint16_t  commandReg;
  void*     pCommandReg;
  uint8_t   bir;
  uint32_t  tableInfo;
  uintptr_t barPhysAddr;
  size_t    barSize;

  pDevice->isMSIEnabled  = false;
  pDevice->isMSIXEnabled = false;

  /* Update the Command register */
  pCommandReg = (void*)&pDevice->pFunction->command;
  commandReg  = _PCIERead16(pCommandReg);
  commandReg |= PCI_COMMAND_BUS_MASTER_ENABLE |
                PCI_COMMAND_MEMORY_SPACE_ENABLE |
                PCI_COMMAND_LEGACY_INTERRUPT_DISABLE;
  _PCIEWrite16(pCommandReg, commandReg);

  /* Check for MSI and MSI-X support */
  pDevice->msiBaseAddr  = 0;
  pDevice->msiXBaseAddr = 0;
  status = _PCIERead16(&pDevice->pFunction->status);
  if ((status & PCI_STATUS_CAPABILITIES_LIST) != 0)
  {
    capabilitiesPtr = _PCIERead8(&pDevice->pFunction->capabilitiesPointer);
    /* Process the capabilities pointer */
    while (capabilitiesPtr != 0)
    {
      /* Read the capability ID */
      capabilityHeader = _PCIERead32((void*)((uintptr_t)pDevice->pFunction +
                                            capabilitiesPtr));
      capabilityId = (uint8_t)(capabilityHeader & 0xFF);
      if (capabilityId == PCI_CAPABILITY_ID_MSI)
      {
        pDevice->msiBaseAddr =(uintptr_t)pDevice->pFunction + capabilitiesPtr;
      }
      else if (capabilityId == PCI_CAPABILITY_ID_MSIX)
      {
        pDevice->msiXBaseAddr = (uintptr_t)pDevice->pFunction +
                                capabilitiesPtr;
      }
      if (pDevice->msiBaseAddr != 0 && pDevice->msiXBaseAddr != 0)
      {
        break;
      }
      capabilitiesPtr = (uint8_t)((capabilityHeader >> 8) & 0xFF);
    }
  }

  /* Now get the BAR MSI-X Table*/
  if (pDevice->msiXBaseAddr != 0)
  {
    /* Get the BAR that the MSI-X is associated with */
    tableInfo = _PCIERead32((void*)(pDevice->msiXBaseAddr + 4));
    bir       = tableInfo & 0x7;
    tableInfo = tableInfo & 0xFFFFFFF8;

    barPhysAddr = PCIGetBARAddress(pDevice->pFunction, bir);
    barSize     = PCIGetBARSize(pDevice->pFunction, bir);

    if (barPhysAddr != 0 && barSize != 0)
    {
      /* Map the MSI-X table */
      pDevice->pMSIXTable = MemoryKernelMap((void*)barPhysAddr,
                                            barSize,
                                            MEMMGR_MAP_HARDWARE |
                                            MEMMGR_MAP_KERNEL   |
                                            MEMMGR_MAP_RW,
                                            &retCode);
      if (retCode == NO_ERROR)
      {
        /* Add the offset of the table */
        pDevice->pMSIXTable = (S_MSIXTableEntry*)
                              ((uintptr_t)pDevice->pMSIXTable + tableInfo);
      }
      else
      {
        pDevice->msiXBaseAddr = 0;
      }
    }
    else
    {
      pDevice->msiXBaseAddr = 0;
    }
  }
}

static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t*         kpUintProp;
  const S_ACPIDriver*     kpACPIDriver;
  size_t                  propLen;
  E_Return                retCode;
  uintptr_t               driverTableCursor;
  S_PCIDriver*            pDriver;
  S_PCIDevice*            pDevice;
  uint16_t                vendorID;
  uint16_t                deviceID;

  /* Get the ACPI pHandle */
  kpUintProp = FDTGetProp(pkFdtNode, PCIE_FDT_ACPI_NODE_PROP, &propLen);
  if (kpUintProp != NULL && propLen == sizeof(uint32_t))
  {
    /* Get the ACPI driver */
    kpACPIDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if (kpACPIDriver != NULL)
    {
      /* Get all devices*/
      retCode = _ProbeDevices(kpACPIDriver->pGetPCIConfigList());

      if (retCode == NO_ERROR)
      {
        /* Attach the drivers */
        pDevice = spDevices;
        while (pDevice != NULL)
        {
          /* Get the head of the registered drivers section */
          driverTableCursor = (uintptr_t)&_START_PCI_TABLE_ADDR;
          pDriver = *(S_PCIDriver**)driverTableCursor;

          /* Compare with the list of registered drivers */
          while (pDriver != NULL)
          {
            vendorID = _PCIERead16(&pDevice->pFunction->vendorID);
            deviceID = _PCIERead16(&pDevice->pFunction->deviceID);
            if (pDriver->deviceID == deviceID &&
                pDriver->vendorID == vendorID)
            {
              _PrepareDeviceInterrupt(pDevice);
              retCode = pDriver->pDriverAttach(pDevice);
              if (retCode != NO_ERROR)
              {
                KERNEL_ERROR("%s failed to attach to device %04x%04x with"
                            " error %d\n",
                              pDriver->pName,
                              deviceID,
                              vendorID,
                              retCode);
              }
              else
              {
                pDevice->pDriver = pDriver;
                KERNEL_INFO("%s attached to device %04x%04x\n",
                            pDriver->pName,
                            deviceID,
                            vendorID);
              }
            }
            driverTableCursor += sizeof(uintptr_t);
            pDriver = *(S_PCIDriver**)driverTableCursor;
          }
          pDevice = pDevice->pNext;
        }
      }
    }
    else
    {
      retCode = ERR_NOT_FOUND;
    }
  }
  else
  {
    retCode = ERR_INVALID_VALUE;
  }

  return retCode;
}

size_t PCIGetBARSize(const S_PCIFunction* kpFunction, const uint8_t kBARIndex)
{
  size_t       size;
  uint32_t*    pBarValue;
  uint32_t     valueSave;
  uint32_t     valueSaveSecond;
  E_PCIBARType type;

  if (kBARIndex < PCI_MAX_BARS)
  {
    pBarValue = (uint32_t*)&kpFunction->bar0 + kBARIndex;
    valueSave = _PCIERead32(pBarValue);
    type = PCIGetBARType(valueSave);

    if (type == PCI_BAR_TYPE_IO)
    {
      _PCIEWrite32(pBarValue, 0xFFFFFFFF);
      size = (size_t)(_PCIERead32(pBarValue));
      _PCIEWrite32(pBarValue, valueSave);

      size = size & 0xFFFFFFFC;
      if (size == 0)
      {
        size = 0;
      }
      else
      {
        size = ~size + 1;
      }
    }
    else if (type == PCI_BAR_TYPE_MEM32)
    {
      _PCIEWrite32(pBarValue, 0xFFFFFFFF);
      size = (size_t)(_PCIERead32(pBarValue));
      _PCIEWrite32(pBarValue, valueSave);

      size = size & 0xFFFFFFF0;
      if (size == 0)
      {
        size = 0;
      }
      else
      {
        size = ~size + 1;
      }
    }
    else if (type == PCI_BAR_TYPE_MEM64 && kBARIndex < PCI_MAX_BARS - 1)
    {
      valueSaveSecond = _PCIERead32(pBarValue + 1);

      _PCIEWrite32(pBarValue, 0xFFFFFFFF);
      _PCIEWrite32(pBarValue + 1, 0xFFFFFFFF);

      size = (size_t)(_PCIERead32(pBarValue));
      size |= ((size_t)(_PCIERead32(pBarValue + 1))) << 32;

      _PCIEWrite32(pBarValue, valueSave);
      _PCIEWrite32(pBarValue + 1, valueSaveSecond);

      size = size & 0xFFFFFFFFFFFFFFF0ULL;
      if (size == 0)
      {
        size = 0;
      }
      else
      {
        size = ~size + 1;
      }
    }
    else
    {
      size = 0;
    }
  }
  else
  {
    size = 0;
  }
  return size;
}

uintptr_t PCIGetBARAddress(const S_PCIFunction* kpFunction,
                           const uint8_t        kBARIndex)
{
  uintptr_t    address;
  uint32_t     barValue[2];
  uint32_t*    pBarValue;
  E_PCIBARType type;

  if (kBARIndex < PCI_MAX_BARS)
  {
    pBarValue = (uint32_t*)&kpFunction->bar0 + kBARIndex;
    type = PCIGetBARType(_PCIERead32(pBarValue));

    if (type == PCI_BAR_TYPE_MEM64 && kBARIndex < PCI_MAX_BARS - 1)
    {
      barValue[0] = _PCIERead32(pBarValue);
      barValue[1] = _PCIERead32(pBarValue + 1);
      address = ((uintptr_t)barValue[1] << 32) | (barValue[0] & 0xFFFFFFF0);
    }
    else if (type == PCI_BAR_TYPE_MEM32)
    {
      address = (uintptr_t)_PCIERead32(pBarValue) & 0xFFFFFFF0;
    }
    else if (type == PCI_BAR_TYPE_IO)
    {
      address = (uintptr_t)(_PCIERead32(pBarValue) & 0xFFFFFFFC);
    }
    else
    {
      address = 0;
    }
  }
  else
  {
    address = 0;
  }

  return address;
}

E_PCIBARType PCIGetBARType(const uint32_t kBarValue)
{
  E_PCIBARType type;

  if (kBarValue & 0x1)
  {
    type = PCI_BAR_TYPE_IO;
  }
  else if ((kBarValue & 0x6) == 0)
  {
    type = PCI_BAR_TYPE_MEM32;
  }
  else if ((kBarValue & 0x6) == 0x4)
  {
    type = PCI_BAR_TYPE_MEM64;
  }
  else
  {
    type = PCI_BAR_TYPE_UNKNOWN;
  }

  return type;
}

E_Return PCISetupInterruptMSI(S_PCIDevice*   pDevice,
                              const uint32_t kInterruptLine,
                              const bool     kIsEnabled)
{
  E_Return retCode;
  uint32_t cpuMainLapicId;
  uint16_t msgCtrl;
  uint32_t msgAddr;
  uint32_t msgData;

  if (pDevice->isMSIXEnabled != true)
  {
    /* Check if MSI is supported */
    if (pDevice->msiBaseAddr != 0)
    {
      msgCtrl = _PCIERead16((void*)(pDevice->msiBaseAddr + 2));

      if (kIsEnabled == true)
      {
        pDevice->isMSIEnabled = true;

        /* Enable MSI */
        cpuMainLapicId = CPUGetLAPICId();

        msgCtrl |= PCIE_MSI_CONTROL_ENABLE;
        msgAddr = 0xFEE00000 | (cpuMainLapicId << 12);
        msgData = kInterruptLine;
      }
      else
      {
        pDevice->isMSIEnabled = false;

        /* Disable MSI */
        msgCtrl &= ~PCIE_MSI_CONTROL_ENABLE;
        msgAddr = 0;
        msgData = 0;
      }

      _PCIEWrite32((void*)(pDevice->msiBaseAddr + 4), msgAddr);
      if ((msgCtrl & PCIE_MSI_CONTROL_64BIT) != 0)
      {
        _PCIEWrite32((void*)(pDevice->msiBaseAddr + 8), 0);
        _PCIEWrite32((void*)(pDevice->msiBaseAddr + 12), msgData);
      }
      else
      {
        _PCIEWrite32((void*)(pDevice->msiBaseAddr + 8), msgData);
      }
      _PCIEWrite16((void*)(pDevice->msiBaseAddr + 2), msgCtrl);
      KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "MSI %s for device at %p",
                    kIsEnabled ? "enabled" : "disabled",
                    pDevice->pFunction);

      retCode = NO_ERROR;
    }
    else
    {
      retCode = ERR_NOT_SUPPORTED;
    }
  }
  else
  {
    retCode = ERR_UNAUTHORIZED_ACTION;
  }

  return retCode;
}

E_Return PCISetupInterruptMSIX(S_PCIDevice*   pDevice,
                               const uint32_t kInterruptLine,
                               const uint8_t  kVector,
                               const bool     kIsEnabled)
{
  E_Return  retCode;
  uint32_t  cpuMainLapicId;
  uint16_t  msgCtrl;
  uint32_t  msgAddr;
  uint32_t  msgData;
  uint32_t  vector;

  if (pDevice->isMSIEnabled != true)
  {
    if (pDevice->msiXBaseAddr != 0)
    {
      msgCtrl = _PCIERead16((void*)(pDevice->msiXBaseAddr + 2));
      if (kIsEnabled == true)
      {
        pDevice->isMSIXEnabled = true;

        /* Enable MSI */
        cpuMainLapicId = CPUGetLAPICId();

        msgCtrl |= PCIE_MSIX_CONTROL_ENABLE;
        msgAddr = 0xFEE00000 | (cpuMainLapicId << 12);
        msgData = kInterruptLine;
        vector  = 0;
      }
      else
      {
        pDevice->isMSIXEnabled = false;

        /* Disable MSI */
        msgCtrl &= ~PCIE_MSIX_CONTROL_ENABLE;
        msgAddr = 0;
        msgData = 0;
        vector  = 1;
      }

      _PCIEWrite32(&pDevice->pMSIXTable[kVector].msgAddrLower, msgAddr);
      _PCIEWrite32(&pDevice->pMSIXTable[kVector].msgAddrUpper, 0);
      _PCIEWrite32(&pDevice->pMSIXTable[kVector].msgData, msgData);
      _PCIEWrite32(&pDevice->pMSIXTable[kVector].vectorControl, vector);

      _PCIEWrite16((void*)(pDevice->msiXBaseAddr + 2), msgCtrl);
      KERNEL_DEBUG(PCIE_DRIVER_DEBUG_ENABLED,
                    MODULE_NAME,
                    "MSIX %s for device at %p",
                    kIsEnabled ? "enabled" : "disabled",
                    pDevice->pFunction);
      retCode = NO_ERROR;
    }
    else
    {
      retCode = ERR_NOT_SUPPORTED;
    }
  }
  else
  {
    retCode = ERR_UNAUTHORIZED_ACTION;
  }

  return retCode;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86PCIEDriver);

/************************************ EOF *************************************/