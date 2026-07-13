/*******************************************************************************
 * @file ACPI.c
 *
 * @see ACPI.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <Memory.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <ACPI.h>

/* Unit test header */
/* TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for regs  */
#define ACPI_FDT_REGS_PROP "reg"

/** @brief Module name */
#define MODULE_NAME "X86 ACPI"

/** @brief ACPI memory signature: RSDP. */
#define ACPI_RSDP_SIG 0x2052545020445352
/** @brief ACPI memory signature: RSDT. */
#define ACPI_RSDT_SIG 0x54445352
/** @brief ACPI memory signature: XSDT. */
#define ACPI_XSDT_SIG 0x54445358
/** @brief ACPI memory signature: FACP. */
#define ACPI_FACP_SIG 0x50434146
/** @brief ACPI memory signature: FACS. */
#define ACPI_FACS_SIG 0x53434146
/** @brief ACPI memory signature: APIC. */
#define ACPI_APIC_SIG 0x43495041
/** @brief ACPI memory signature: DSDT. */
#define ACPI_DSDT_SIG 0x54445344
/** @brief ACPI memory signature: HPET. */
#define ACPI_HPET_SIG 0x54455048

/** @brief APIC type: local APIC. */
#define APIC_TYPE_LOCAL_APIC 0x0
/** @brief APIC type: IO APIC. */
#define APIC_TYPE_IO_APIC 0x1
/** @brief APIC type: interrupt override. */
#define APIC_TYPE_INTERRUPT_OVERRIDE 0x2
/** @brief APIC type: NMI. */
#define APIC_TYPE_NMI 0x4

/** @brief HPET Flags HW Revision mask */
#define HPET_FLAGS_HW_REV_MASK 0x00FF
/** @brief HPET Flags comparator count mask */
#define HPET_FLAGS_CC_MASK 0x1F00
/** @brief HPET Flags counter size mask */
#define HPET_FLAGS_CS_MASK 0x2000
/** @brief HPET Flags legacy replacement IRQ mask */
#define HPET_FLAGS_IRQ_MASK 0x8000

/** @brief HPET Flags HW Revision shift */
#define HPET_FLAGS_HW_REV_SHIFT 0
/** @brief HPET Flags comparator count shift */
#define HPET_FLAGS_CC_SHIFT 8
/** @brief HPET Flags counter size shift */
#define HPET_FLAGS_CS_SHIFT 13
/** @brief HPET Flags legacy replacement IRQ shift */
#define HPET_FLAGS_IRQ_SHIFT 15

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief ACPI structure header.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief Header signature */
  char pSignature[4];
  /** @brief Structure length */
  uint32_t length;
  /** @brief Structure revision version */
  uint8_t revision;
  /** @brief Structure checksum */
  uint8_t checksum;
  /** @brief OEM identifier */
  char pOEM[6];
  /** @brief OEM talbe identifier */
  char pOEMTableId[8];
  /** @brief OEM revision version */
  uint32_t oemRevision;
  /** @brief Creator identifier */
  uint32_t creatorId;
  /** @brief Creator revision version */
  uint32_t creatorRevision;
} __attribute__((__packed__)) S_ACPIHeader;

/** @brief ACPI RSDP descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief RSDP Signature */
  char pSignature[8];
  /** @brief RSDP checksum */
  uint8_t checksum;
  /** @brief RSDP OEM identifiter */
  char oemid[6];
  /** @brief RSDP ACPI resivion version */
  uint8_t revision;
  /** @brief RSDT pointer address */
  uint32_t rsdtAddress;
} __attribute__ ((packed)) S_RSDPDescriptor;

/** @brief ACPI extended RSDP descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief RSDP base part (non extended) */
  S_RSDPDescriptor rsdpBasePart;
  /** @brief RSDP extension length */
  uint32_t length;
  /** @brief XSDT pointer address */
  uint64_t xsdtAddress;
  /** @brief RSDP extended checksum */
  uint8_t extendedChecksum;
  /** @brief Reserved memory */
  uint8_t reserved[3];
} __attribute__ ((packed)) S_RSDPDescriptorExtended;

/** @brief ACPI RSDT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief RSDT header */
  S_ACPIHeader header;
  /** @brief Array of pointers to the ACPI DTs. */
  uint32_t *pDtPointers;
} __attribute__ ((packed)) S_RSDTDescriptor;

/** @brief ACPI XSDT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief XSDT header */
  S_ACPIHeader header;
  /** @brief Array of pointers to the ACPI DTs. */
  uint64_t *pDtPointers;
} __attribute__ ((packed)) S_XSDTDescriptor;

/** @brief ACPI address descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief Address space identifier */
  uint8_t addressSpace;
  /** @brief Bit width */
  uint8_t bitWidth;
  /** @brief Bit offset */
  uint8_t bitOffset;
  /** @brief Access size */
  uint8_t accessSize;
  /** @brief Plain address */
  uint64_t address;
} __attribute__((__packed__)) S_GAddress;

/** @brief ACPI FADT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief FADT header */
  S_ACPIHeader header;
  /** @brief Firmware control */
  uint32_t firmwareControl;
  /** @brief DSDT pointer address*/
  uint32_t dsdt;
  /** @brief Reserved */
  uint8_t reserved0;
  /** @brief Prefered PM profile */
  uint8_t preferredPMProfile;
  /** @brief SCI interrupt */
  uint16_t sciInterrupt;
  /** @brief SMI command port */
  uint32_t smiCommandPort;
  /** @brief ACPI enable */
  uint8_t acpiEnable;
  /** @brief ACPI disable */
  uint8_t acpiDisable;
  /** @brief S4 BIOS request */
  uint8_t s4BIOSReq;
  /** @brief PSTATE control */
  uint8_t pstateControl;
  /** @brief PM1 A Event block */
  uint32_t pm1aEventBlock;
  /** @brief PM1 B Event block */
  uint32_t pm1bEventBlock;
  /** @brief PM1 A Control block */
  uint32_t pm1aControlBlock;
  /** @brief PM1 B Control block */
  uint32_t pm1bControlBlock;
  /** @brief PM2 Control block */
  uint32_t pm2ControlBlock;
  /** @brief PM Timer block */
  uint32_t pmTimerBlock;
  /** @brief GPE0 Block */
  uint32_t gpe0Block;
  /** @brief GPE1 Block */
  uint32_t gpe1Block;
  /** @brief PM1 Event length */
  uint8_t pm1EventLength;
  /** @brief PM1 Control length */
  uint8_t pm1ControlLength;
  /** @brief PM1 Control length */
  uint8_t pm2ControlLength;
  /** @brief PM Timer length*/
  uint8_t pmTimerLength;
  /** @brief GPE0 length */
  uint8_t gpe0Length;
  /** @brief GPE1 length */
  uint8_t gpe1Length;
  /** @brief GPE1 balse */
  uint8_t gpe1Base;
  /** @brief CState control */
  uint8_t cStateControl;
  /** @brief Worst C2 latency */
  uint16_t worstC2Latency;
  /** @brief Worst C3 latency */
  uint16_t worstC3Latency;
  /** @brief Flush size */
  uint16_t flushSize;
  /** @brief Flush stride */
  uint16_t flush_stride;
  /** @brief Duty offset */
  uint8_t dutyOffset;
  /** @brief Duty width */
  uint8_t dutyWidth;
  /** @brief Day alarm */
  uint8_t dayAlarm;
  /** @brief Month alarm */
  uint8_t monthAlarm;
  /** @brief Century */
  uint8_t century;
  /** @brief Boot architecture flags */
  uint16_t bootArchitectureFlags;
  /** @brief Reserved */
  uint8_t reserved1;
  /** @brief Flags */
  uint32_t flags;
  /** @brief Reset registers */
  S_GAddress resetReg;
  /** @brief Reset value */
  uint8_t resetValue;
  /** @brief Reserved */
  uint8_t reserved2[3];
  /** @brief Extended firmware control */
  uint64_t xFirmwareControl;
  /** @brief Extended DSDT */
  uint64_t xDsdt;
  /** @brief Extended PM1 A Event block  */
  S_GAddress xPM1aEventBlock;
  /** @brief Extended PM1 B Event block */
  S_GAddress xPM1bEventBlock;
  /** @brief Extended PM1 A Control block */
  S_GAddress xPM1aControlBlock;
  /** @brief Extended PM1 B Control block */
  S_GAddress xPM1bControlBlock;
  /** @brief Extended PM2 Control block */
  S_GAddress xPM2ControlBlock;
  /** @brief Extended PM Timer block */
  S_GAddress xPMTimerBlock;
  /** @brief Extended GPE0 block  */
  S_GAddress xGPE0Block;
  /** @brief Extended GPE1 block */
  S_GAddress xGPE1Block;
} __attribute__((__packed__)) S_FADT;

/** @brief ACPI DSDT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief DSDT header */
  S_ACPIHeader header;
}  __attribute__((__packed__)) S_DSDT;

/** @brief ACPI MADT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief DSDT header */
  S_ACPIHeader header;
  /** @brief Local APIC ACPI address */
  uint32_t localApicAddr;
  /** @brief Local APIC flags */
  uint32_t flags;
} __attribute__((__packed__)) S_MADT;

/** @brief ACPI HPET descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief The APIC header field. */
  S_ACPIHeader header;
  /** @brief HPET flags
   * @details [0:7] hardware revision ID
   *          [8:12] Comparator count
   *          [13] Counter size
   *          [14] Reserved
   *          [15] Legacy Replacement IRQ routine table
   */
  uint16_t flags;
  /** @brief PCI vendor ID */
  uint16_t pciVendorId;
  /** @brief HPET base adderss */
  S_GAddress address;
  /** @brief HPET sequence number */
  uint8_t hpetNumber;
  /** @brief Minimum number of ticks supported in periodic mode */
  uint16_t minimumTick;
  /** @brief Page protection attribute */
  uint8_t pageProtection;
} __attribute__((__packed__)) S_ACPIHPETDescriptor;

/**
 * @brief ACPI APIC descriptor header.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief The APIC descriptor type. */
  uint8_t type;
  /** @brief The APIC descriptor length. */
  uint8_t length;
} __attribute__((__packed__)) S_APICHeader;

/**
 * @brief ACPI IO APIC descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief The APIC header field. */
  S_APICHeader header;
  /** @brief Stores the IO-APIC identifier. */
  uint8_t ioApicId;
  /** @brief Reserved field. */
  uint8_t reserved;
  /** @brief Stores the IO-APIC MMIO address. */
  uint32_t ioApicAddr;
  /** @brief Stores the IO-APIC GSI base address. */
  uint32_t globalSystemInterruptBase;
} __attribute__((__packed__)) S_IOAPIC;

/**
 * @brief ACPI LAPIC descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief The APIC header field. */
  S_APICHeader header;
  /** @brief Stores the LAPIC CPU identifier. */
  uint8_t cpuId;
  /** @brief Stores the LAPIC identifier.  */
  uint8_t lapicId;
  /** @brief Stores the LAPIC configuration flags. */
  uint32_t flags;
} __attribute__((__packed__)) S_LAPIC;

/** @brief ACPI Interrupt override descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief The APIC header field. */
  S_APICHeader header;
  /** @brief Interrupt override bus */
  uint8_t bus;
  /** @brief Interrupt override source */
  uint8_t source;
  /** @brief Interrupt override destination */
  uint32_t interrupt;
  /** @brief Interrupt override flags */
  uint16_t flags;
} __attribute__((__packed__)) S_APICInterruptOverride;

/** @brief x86 ACPI driver controler. */
typedef struct
{
  /** @brief Detected CPU count. */
  uint8_t detectedCPUCount;
  /** @brief Detected IO APIC count. */
  uint8_t detectedIOAPICCount;
  /** @brief Detected interrupt override count */
  uint8_t detectedIntOverrideCount;
  /** @brief Detected HPET count */
  uint8_t detectedHpetCount;
  /** @brief Parsed local APIC address. */
  uintptr_t localApicAddress;
  /** @brief List of detected LAPIC. */
  S_LAPICNode* pLAPICList;
  /** @brief List of detected IO APIC. */
  S_IOAPICNode* pIoApicList;
  /** @brief List of detected interrupt override */
  S_InterruptOverrideNode* pIntOverrideList;
  /** @brief List of detected HPET devices */
  S_HPETNode* pHpetList;
} S_ACPIControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the ACPI to ensure correctness of execution.
 *
 * @details Assert macro used by the ACPI to ensure correctness of execution.
 * Due to the critical nature of the ACPI, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define ACPI_ASSERT(COND, MSG, ERROR) {                 \
  if ((COND) == false)                                   \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false);              \
  }                                                     \
}

/**
 * @brief Adds a node to a list using a cursor.
 *
 * @param[out] LIST The list to update.
 * @param[out] CURSOR The cursor to use.
 * @param[out] NODE The node to add.
*/
#define ADD_TO_LIST(LIST, CURSOR, NODE) {     \
  NODE->pNext = NULL;                         \
  CURSOR = LIST;                              \
  if (CURSOR != NULL)                          \
  {                                           \
    while (CURSOR->pNext != NULL)              \
    {                                         \
      CURSOR = CURSOR->pNext;                 \
    }                                         \
    CURSOR->pNext = NODE;                     \
  }                                           \
  else                                        \
  {                                           \
    LIST = NODE;                              \
  }                                           \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the ACPI driver to the system.
 *
 * @details Attaches the ACPI driver to the system. This function will use the
 * FDT to initialize the ACPI hardware and retreive the ACPI parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Use the APIC RSDP to parse the ACPI infomation.
 *
 * @details Use the APIC RSDP to parse the ACPI infomation. The function will
 * detect the RSDT or XSDT pointed and parse them.
 *
 * @param[in] kpRsdpDesc The RSDP to walk.
 */
static void  _ParseRSDP(const S_RSDPDescriptor* kpRsdpDesc);

/**
 * @brief Use the APIC RSDP to parse the ACPI infomation.
 *
 * @details Use the APIC RSDP to parse the ACPI infomation. The function will
 * detect the RSDT or XSDT pointed and parse them.
 *
 * @param[in] kpRsdpDesc The RSDP to walk.
 */
static void _ParseRSDPRevision0(const S_RSDPDescriptor* kpRsdpDesc);

/**
 * @brief Use the APIC RSDP to parse the ACPI infomation.
 *
 * @details Use the APIC RSDP to parse the ACPI infomation. The function will
 * detect the RSDT or XSDT pointed and parse them.
 *
 * @param[in] kpRsdpDesc The RSDP to walk.
 */
static void _ParseRSDPRevision2(const S_RSDPDescriptor* kpRsdpDesc);

/**
 * @brief Parse the APIC RSDT table.
 *
 * @details Parse the APIC RSDT table. The function will detect the read each
 * entries of the RSDT and call the corresponding functions to parse the entries
 * correctly.
 *
 * @param[in] kpRsdtPtr The address of the RSDT entry to parse.
 */
static void _ParseRSDT(const S_RSDTDescriptor* kpRsdtPtr);

#ifdef ARCH_64_BITS
/**
 * @brief Parse the APIC XSDT table.
 *
 * @details The function will detect the read each entries of the XSDT and call
 * the corresponding functions to parse the entries correctly.
 *
 * @param[in] kpXsdtPtr The address of the XSDT entry to parse.
 */
static void _ParseXSDT(const S_XSDTDescriptor* kpXsdtPtr);
#endif

/**
 * @brief Parse the APIC SDT table.
 *
 * @details Parse the APIC SDT table. The function will detect the SDT given as
 * parameter thanks to the information contained in the header. Then, if the
 * entry is correctly detected and supported, the parsing function corresponding
 * will be called.
 *
 * @param[in] kpHeader The virtual address of the SDT entry to parse.
 * @param[in] kPhysAddr The physical address of the SDT entry.
 */
static void _ParseDT(const S_ACPIHeader* kpHeader,
                         const uintptr_t     kPhysAddr);

/**
 * @brief Parse the APIC FADT table.
 *
 * @details Parse the APIC FADT table. The function will save the FADT table
 * address in for further use.
 *
 * @param[in] kpFadtPtr The address of the FADT entry to parse.
 */
static void _ParseFADT(const S_FADT* kpFadtPtr);

/**
 * @brief Parses the APIC entries of the MADT table.
 *
 * @details Parse the APIC entries of the MADT table.The function will parse
 * each entry and detect three of the possible entry kind: the LAPIC entries,
 * which also determine the cpu count, the IO-APIC entries will detect the
 * different available IO-APIC of the system, the interrupt override will also
 * be detected.
 *
 * @param[in] kpApicPtr The address of the MADT entry to parse.
 */
static void _ParseMADT(const S_MADT* kpApicPtr);

/**
 * @brief Parses the APIC entries of the HPET table.
 *
 * @details Parse the APIC entries of the HPET table.The function will parse
 * each entry and add the HPET node to the list of detected HPET entries.
 *
 * @param[in] kpHpetPtr The address of the HPET entry to parse.
 */
static void _ParseHPET(const S_ACPIHPETDescriptor* kpHpetPtr);

/**
 * @brief Returns the number of LAPIC detected in the system.
 *
 * @details Returns the number of LAPIC detected in the system.
 *
 * @return The number of LAPIC detected in the system is returned.
    */
static uint8_t _GetLAPICCount(void);

/**
 * @brief Returns the list of detected LAPICs.
 *
 * @details Returns the list of detected LAPICs. This list should not be
 * modified and is generated during the attach of the ACPI while parsing
 * its tables.
 *
 * @return The list of detected LAPICs descritors is returned.
 */
static const S_LAPICNode* _GetLAPICList(void);

/**
 * @brief Returns the detected LAPIC base address.
 *
 * @details Returns the detected LAPIC base address.
 *
 * @return The detected LAPIC base address is returned.
 */
static uintptr_t _GetLAPICBaseAddress(void);

/**
 * @brief Returns the number of IO-APIC detected in the system.
 *
 * @details Returns the number of IO-APIC detected in the system.
 *
 * @return The number of IO-APIC detected in the system is returned.
    */
static uint8_t _GetIOAPICCount(void);

/**
 * @brief Returns the list of detected IO-APICs.
 *
 * @details Returns the list of detected IO-APICs. This list should not be
 * modified and is generated during the attach of the ACPI while parsing
 * its tables.
 *
 * @return The list of detected IO-APICs descritors is returned.
 */
static const S_IOAPICNode* _GetIOAPICList(void);

/**
 * @brief Returns the list of detected HPETs.
 *
 * @details Returns the list of detected HPETs. This list should not be
 * modified and is generated during the attach of the ACPI while parsing
 * its tables.
 *
 * @return The list of detected HPETs descritors is returned.
 */
static const S_HPETNode* _GetHPETList(void);

/**
 * @brief Checks if the IRQ has been remaped in the IO-APIC structure.
 *
 * @details Checks if the IRQ has been remaped in the IO-APIC structure. This
 * function must be called after the Init function.
 *
 * @param[in] kIRQNumber The initial IRQ number to check.
 *
 * @return The remapped IRQ number corresponding to the irq number given as
 * parameter.
 */
static uint32_t _GetRemapedIRQ(const uint32_t kIRQNumber);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief ACPI driver instance. */
static S_Driver sX86ACPIDriver =
{
  .pName         = "X86 ACPI Driver",
  .pDescription  = "X86 ACPI Driver for roOs",
  .pCompatible   = "x86,x86-acpi",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/** @brief ACPI driver controler instance */
static S_ACPIControler sDrvCtrl =
{
  .detectedCPUCount         = 0,
  .detectedIOAPICCount      = 0,
  .detectedIntOverrideCount = 0,
  .detectedHpetCount        = 0,
  .pIoApicList              = NULL,
  .pLAPICList               = NULL,
  .pIntOverrideList         = NULL,
  .pHpetList                = NULL,
};

/** @brief ACPI external driver instance */
static S_ACPIDriver sAPIDriver =
{
  .pGetLAPICCount       = _GetLAPICCount,
  .pGetLAPICList        = _GetLAPICList,
  .pGetLAPICBaseAddress = _GetLAPICBaseAddress,
  .pGetIOAPICCount      = _GetIOAPICCount,
  .pGetIOAPICList       = _GetIOAPICList,
  .pGetHPETList         = _GetHPETList,
  .pGetRemapedIRQ       = _GetRemapedIRQ,
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uintptr_t* kpUintptrProp;
  size_t           propLen;
  E_Return         retCode;
  uintptr_t        searchRangeStart;
  uintptr_t        searchRangeEnd;
  size_t           mapSize;
  uintptr_t        mapBase;
  uintptr_t        mapPhys;
  size_t           initSize;
  uint64_t         signature;

  /* Get the reg: the range to search for the ACPI structure */
  kpUintptrProp = FDTGetProp(pkFdtNode, ACPI_FDT_REGS_PROP, &propLen);

  ACPI_ASSERT(kpUintptrProp != NULL && propLen == sizeof(uintptr_t) * 2,
              "Invalid ACPI node in FDT.",
              retCode);

#ifdef ARCH_32_BITS
  searchRangeStart = FDTTOCPU32(*kpUintptrProp);
  initSize = FDTTOCPU32(*(kpUintptrProp + 1));
  searchRangeEnd   = searchRangeStart + initSize;
#elif defined(ARCH_64_BITS)
  searchRangeStart = FDTTOCPU64(*kpUintptrProp);
  initSize = FDTTOCPU64(*(kpUintptrProp + 1));
  searchRangeEnd   = searchRangeStart + initSize;
#else
  #error "Invalid architecture"
#endif

  /* Map the memory */
  mapPhys = searchRangeStart;
  mapBase = searchRangeStart & ~PAGE_SIZE_MASK;
  mapSize = ((searchRangeEnd - mapBase) + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

  searchRangeStart = (uintptr_t)MemoryKernelMap((void*)mapBase,
                                                mapSize,
                                                MEMMGR_MAP_HARDWARE |
                                                MEMMGR_MAP_KERNEL   |
                                                MEMMGR_MAP_RO,
                                                &retCode);
  ACPI_ASSERT(retCode == NO_ERROR, "Failed to map ACPI", retCode);

  /* Search for the ACPI table */
  mapBase          = searchRangeStart;
  searchRangeStart = mapBase + (mapPhys & PAGE_SIZE_MASK);
  searchRangeEnd   = searchRangeStart + initSize;
  while (searchRangeStart < searchRangeEnd)
  {
    signature = *(uint64_t*)searchRangeStart;

    /* Checking the RSDP signature */
    if (signature == ACPI_RSDP_SIG)
    {
      KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "ACPI RSDP found at 0x%p",
                   searchRangeStart);
      /* Parse RSDP */
      _ParseRSDP((S_RSDPDescriptor*)searchRangeStart);
      break;
    }

    searchRangeStart += sizeof(uintptr_t);
  }

  /* Unmap the memory */
  retCode = MemoryKernelUnmap((void*)mapBase, mapSize);
  ACPI_ASSERT(retCode == NO_ERROR, "Failed to unmap ACPI", retCode);

  ACPI_ASSERT(searchRangeStart < searchRangeEnd,
              "ACPI not found", ERR_NOT_SUPPORTED);

  /* Set the API driver */
  retCode = DriverManagerSetDeviceData(pkFdtNode, &sAPIDriver);
  ACPI_ASSERT(retCode == NO_ERROR, "Failed to register ACPI", retCode);

  return NO_ERROR;
}

static void _ParseRSDPRevision0(const S_RSDPDescriptor* kpRsdpDesc)
{
  uintptr_t                 descAddr;
  S_RSDTDescriptor*         pDesc;
  size_t                    toMap;
  E_Return                  errCode;

  /* Map pages for RSDT */
  toMap = ((uintptr_t)kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK) +
            sizeof(S_RSDTDescriptor);
  toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

  descAddr = (uintptr_t)kpRsdpDesc->rsdtAddress & ~PAGE_SIZE_MASK;
  descAddr = (uintptr_t)MemoryKernelMap((void*)descAddr,
                                        toMap,
                                        MEMMGR_MAP_HARDWARE |
                                        MEMMGR_MAP_KERNEL   |
                                        MEMMGR_MAP_RO,
                                        &errCode);
  ACPI_ASSERT(errCode == NO_ERROR && descAddr != (uintptr_t)NULL,
              "Failed to map RSDT",
              errCode);
  pDesc = (S_RSDTDescriptor*)(descAddr |
                              (kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK));
  _ParseRSDT(pDesc);

  /* Unmap */
  errCode = MemoryKernelUnmap((void*)descAddr, toMap);
  ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap RSDT", errCode);
}

static void _ParseRSDPRevision2(const S_RSDPDescriptor* kpRsdpDesc)
{
  uintptr_t                 descAddr;
  S_RSDTDescriptor*         pDesc;
  size_t                    toMap;
  S_RSDPDescriptorExtended* pExtendedRsdp;
  E_Return                  errCode;
  uint8_t                   sum;
  uint8_t                   i;

  pExtendedRsdp = (S_RSDPDescriptorExtended*)kpRsdpDesc;
  sum = 0;

  for (i = 0; i < sizeof(S_RSDPDescriptorExtended); ++i)
  {
    sum += ((uint8_t*)pExtendedRsdp)[i];
  }
  if (sum == 0)
  {
#ifdef ARCH_64_BITS
    if (pExtendedRsdp->xsdtAddress)
    {
      /* Map pages for XSDT */
      toMap = ((uintptr_t)pExtendedRsdp->xsdtAddress & PAGE_SIZE_MASK) +
                sizeof(S_XSDTDescriptor);
      toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

      descAddr = (uintptr_t)pExtendedRsdp->xsdtAddress &
                  ~PAGE_SIZE_MASK;
      descAddr = (uintptr_t)MemoryKernelMap((void*)descAddr,
                                          toMap,
                                          MEMMGR_MAP_HARDWARE |
                                          MEMMGR_MAP_KERNEL   |
                                          MEMMGR_MAP_RO,
                                          &errCode);
      ACPI_ASSERT(errCode == NO_ERROR && descAddr != (uintptr_t)NULL,
                  "Failed to map XSDT",
                  errCode);
      _ParseXSDT((S_XSDTDescriptor*)(descAddr |
                                      ((uintptr_t)pExtendedRsdp->xsdtAddress &
                                      PAGE_SIZE_MASK)));
    }
    else
#endif
    {
      /* Map pages for RSDT */
      toMap = ((uintptr_t)kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK) +
                sizeof(S_RSDTDescriptor);
      toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

      descAddr = (uintptr_t)kpRsdpDesc->rsdtAddress &
                  ~PAGE_SIZE_MASK;
      descAddr = (uintptr_t)MemoryKernelMap((void*)descAddr,
                                          KERNEL_PAGE_SIZE * 2,
                                          MEMMGR_MAP_HARDWARE |
                                          MEMMGR_MAP_KERNEL   |
                                          MEMMGR_MAP_RO,
                                          &errCode);
      ACPI_ASSERT(errCode == NO_ERROR && descAddr != (uintptr_t)NULL,
                  "Failed to map RSDT",
                  errCode);

      pDesc = (S_RSDTDescriptor*)(descAddr |
                                  (kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK));
      _ParseRSDT(pDesc);
    }

    /* Unmap */
    errCode = MemoryKernelUnmap((void*)descAddr, toMap);
    ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap RSDT", errCode);
  }
}

static void _ParseRSDP(const S_RSDPDescriptor* kpRsdpDesc)
{
  uint8_t sum;
  uint8_t i;

  ACPI_ASSERT(kpRsdpDesc != NULL,
              "Tried to parse a NULL RSDP",
              ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < sizeof(S_RSDPDescriptor); ++i)
  {
    sum += ((uint8_t*)kpRsdpDesc)[i];
  }
  ACPI_ASSERT((sum & 0xFF) == 0, "RSDP Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(kpRsdpDesc->revision == 0 || kpRsdpDesc->revision == 2,
              "Unsupported ACPI version",
              ERR_NOT_SUPPORTED);

  /* ACPI version check */
  if (kpRsdpDesc->revision == 0)
  {
    _ParseRSDPRevision0(kpRsdpDesc);
  }
  else if (kpRsdpDesc->revision == 2)
  {
    _ParseRSDPRevision2(kpRsdpDesc);
  }
}

static void _ParseRSDT(const S_RSDTDescriptor* kpRrsdtPtr)
{
  uintptr_t     rangeBegin;
  uintptr_t     rangeEnd;
  uintptr_t     descAddr;
  size_t        toMap;
  E_Return      errCode;
  uint8_t       i;
  int8_t        sum;
  S_ACPIHeader* pAddress;

  ACPI_ASSERT(kpRrsdtPtr != NULL,
              "Tried to parse a NULL RSDT",
              ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpRrsdtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpRrsdtPtr)[i];
  }

  ACPI_ASSERT((sum & 0xFF) == 0, "RSDT Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpRrsdtPtr->header.pSignature) == ACPI_RSDT_SIG,
              "Wrong RSDT Signature",
              ERR_INVALID_VALUE);

  rangeBegin = (uintptr_t)(&kpRrsdtPtr->pDtPointers);
  rangeEnd   = ((uintptr_t)kpRrsdtPtr + kpRrsdtPtr->header.length);

  /* Parse each SDT of the RSDT */
  while (rangeBegin < rangeEnd)
  {
    pAddress = (S_ACPIHeader*)(uintptr_t)(*(uint32_t*)rangeBegin);

    /* Map pages */
    toMap = ((uintptr_t)pAddress & PAGE_SIZE_MASK) + sizeof(S_ACPIHeader);
    toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

    descAddr = (uintptr_t)pAddress & ~PAGE_SIZE_MASK;
    descAddr = (uintptr_t)MemoryKernelMap((void*)descAddr,
                                          toMap,
                                          MEMMGR_MAP_HARDWARE |
                                          MEMMGR_MAP_KERNEL   |
                                          MEMMGR_MAP_RO,
                                          &errCode);
    ACPI_ASSERT(errCode == NO_ERROR && descAddr != (uintptr_t)NULL,
                "Failed to map DT",
                errCode);

    _ParseDT((S_ACPIHeader*)
              (descAddr | ((uintptr_t)pAddress & PAGE_SIZE_MASK)),
              (uintptr_t)pAddress);

    /* Unmap */
    errCode = MemoryKernelUnmap((void*)descAddr, toMap);
    ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap DT", errCode);

    rangeBegin += sizeof(uint32_t);
  }
}

#ifdef ARCH_64_BITS
static void _ParseXSDT(const S_XSDTDescriptor* kpXsdtPtr)
{
  uintptr_t     rangeBegin;
  uintptr_t     rangeEnd;
  uintptr_t     descAddr;
  size_t        toMap;
  E_Return      errCode;
  uint8_t       i;
  int8_t        sum;
  S_ACPIHeader* pAddress;

  ACPI_ASSERT(kpXsdtPtr != NULL,
              "Tried to parse a NULL XSDT",
              ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpXsdtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpXsdtPtr)[i];
  }

  ACPI_ASSERT((sum & 0xFF) == 0, "XSDT Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpXsdtPtr->header.pSignature) == ACPI_XSDT_SIG,
              "Wrong XSDT Signature",
              ERR_INVALID_VALUE);

  rangeBegin = (uintptr_t)(&kpXsdtPtr->pDtPointers);
  rangeEnd   = ((uintptr_t)kpXsdtPtr + kpXsdtPtr->header.length);

  /* Parse each SDT of the RSDT */
  while (rangeBegin < rangeEnd)
  {
    pAddress = (S_ACPIHeader*)(*(uint64_t*)rangeBegin);

    /* Map pages */
    toMap = ((uintptr_t)pAddress & PAGE_SIZE_MASK) + sizeof(S_ACPIHeader);
    toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

    descAddr = (uintptr_t)pAddress & ~PAGE_SIZE_MASK;
    descAddr = (uintptr_t)MemoryKernelMap((void*)descAddr,
                                          toMap,
                                          MEMMGR_MAP_HARDWARE |
                                          MEMMGR_MAP_KERNEL   |
                                          MEMMGR_MAP_RO,
                                          &errCode);
    ACPI_ASSERT(errCode == NO_ERROR && descAddr != (uintptr_t)NULL,
                "Failed to map DT x64",
                errCode);

    _ParseDT((S_ACPIHeader*)
             (descAddr | ((uintptr_t)pAddress & PAGE_SIZE_MASK)),
             (uintptr_t)pAddress);

    /* Unmap */
    errCode = MemoryKernelUnmap((void*)descAddr, toMap);
    ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap DT", errCode);

    rangeBegin += sizeof(uint64_t);
  }

}
#endif

static void _ParseDT(const S_ACPIHeader* kpHeader, const uintptr_t kPhysAddr)
{
  uintptr_t descAddr;
  uintptr_t descPtr;
  size_t    toMap;
  E_Return  errCode;

  ACPI_ASSERT(kpHeader != NULL,
              "Tried to parse a NULL DT",
              ERR_INVALID_PARAMETER);

  /* Map memory */
  toMap = ((uintptr_t)kpHeader & PAGE_SIZE_MASK) + kpHeader->length;
  toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

  descAddr = (uintptr_t)kPhysAddr & ~PAGE_SIZE_MASK;
  descAddr = (uintptr_t)MemoryKernelMap((void*)descAddr,
                                        toMap,
                                        MEMMGR_MAP_HARDWARE |
                                        MEMMGR_MAP_KERNEL   |
                                        MEMMGR_MAP_RO,
                                        &errCode);
  ACPI_ASSERT(errCode == NO_ERROR && descAddr != (uintptr_t)NULL,
              "Failed to remap DT",
              errCode);

  descPtr = descAddr | (kPhysAddr & PAGE_SIZE_MASK);
  if (*((uint32_t*)kpHeader->pSignature) == ACPI_FACP_SIG)
  {
    _ParseFADT((S_FADT*)descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_APIC_SIG)
  {
    _ParseMADT((S_MADT*)descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_HPET_SIG)
  {
    _ParseHPET((S_ACPIHPETDescriptor*)descPtr);
  }

  /* Unmap memory */
  errCode = MemoryKernelUnmap((void*)descAddr, toMap);
  ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap DT", errCode);
}

static void _ParseFADT(const S_FADT* kpFadtPtr)
{
  int32_t  sum;
  uint32_t i;

  ACPI_ASSERT(kpFadtPtr != NULL,
              "Tried to parse a NULL FADT",
              ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpFadtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpFadtPtr)[i];
  }

  ACPI_ASSERT((sum & 0xFF) == 0, "FADT Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpFadtPtr->header.pSignature) == ACPI_FACP_SIG,
              "FADT Signature comparison failed",
              ERR_INVALID_VALUE);
}

static void _ParseMADT(const S_MADT* kpMadtPtr)
{
  int32_t                  sum;
  uint32_t                 maxCpuCount;
  uint32_t                 i;
  uintptr_t                madtEntry;
  uintptr_t                madtLimit;
  S_APICHeader*            pHeader;
  S_LAPICNode*             pLAPICNode;
  S_LAPICNode*             pLAPICListCursor;
  S_IOAPICNode*            pIOApicNode;
  S_IOAPICNode*            pIOApicListCursor;
  S_InterruptOverrideNode* pIntOverrideNode;
  S_InterruptOverrideNode* pIntOverrideListCursor;

  ACPI_ASSERT(kpMadtPtr != NULL,
              "Tried to parse a NULL APIC",
              ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpMadtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpMadtPtr)[i];
  }

  ACPI_ASSERT((sum & 0xFF) == 0, "APIC checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpMadtPtr->header.pSignature) == ACPI_APIC_SIG,
              "Invalid APIC signature",
              ERR_INVALID_VALUE);

  madtEntry = (uintptr_t)(kpMadtPtr + 1);
  madtLimit = ((uintptr_t)kpMadtPtr) + kpMadtPtr->header.length;

  maxCpuCount = CPUGetCount();

  /* Get the LAPIC address */
  sDrvCtrl.localApicAddress = kpMadtPtr->localApicAddr;

  while (madtEntry < madtLimit)
  {
    /* Get entry header */
    pHeader = (S_APICHeader*)madtEntry;

    /* Check entry type */
    if (pHeader->type == APIC_TYPE_LOCAL_APIC)
    {
      if (sDrvCtrl.detectedCPUCount < maxCpuCount)
      {
        /* Create new LAPIC node */
        pLAPICNode = KMalloc(sizeof(S_LAPICNode),
                             ALIGN_ADDRESS,
                             KMALLOC_NO_FREE_POOL);

        /* Fill the descriptor */
        pLAPICNode->lapic.lapicId = ((S_LAPIC*)madtEntry)->lapicId;
        pLAPICNode->lapic.cpuId   = ((S_LAPIC*)madtEntry)->cpuId;
        pLAPICNode->lapic.flags   = ((S_LAPIC*)madtEntry)->flags;

        /* Link the node */
        ADD_TO_LIST(sDrvCtrl.pLAPICList, pLAPICListCursor, pLAPICNode);
        ++sDrvCtrl.detectedCPUCount;
      }
    }
    else if (pHeader->type == APIC_TYPE_IO_APIC)
    {
      /* Create new IO APIC node */
      pIOApicNode = KMalloc(sizeof(S_IOAPICNode),
                            ALIGN_ADDRESS,
                            KMALLOC_NO_FREE_POOL);

      /* Fill the descriptor */
      pIOApicNode->ioApic.ioApicId = ((S_IOAPIC*)madtEntry)->ioApicId;
      pIOApicNode->ioApic.ioApicAddr = ((S_IOAPIC*)madtEntry)->ioApicAddr;
      pIOApicNode->ioApic.globalSystemInterruptBase =
        ((S_IOAPIC*)madtEntry)->globalSystemInterruptBase;

      /* Link the node */
      ADD_TO_LIST(sDrvCtrl.pIoApicList, pIOApicListCursor, pIOApicNode);
      ++sDrvCtrl.detectedIOAPICCount;
    }
    else if (pHeader->type == APIC_TYPE_INTERRUPT_OVERRIDE)
    {
      /* Create new IO APIC node */
      pIntOverrideNode = KMalloc(sizeof(S_InterruptOverrideNode),
                                 ALIGN_ADDRESS,
                                 KMALLOC_NO_FREE_POOL);

      /* Fill the descriptor */
      pIntOverrideNode->intOverride.bus =
        ((S_APICInterruptOverride*)madtEntry)->bus;
      pIntOverrideNode->intOverride.source =
        ((S_APICInterruptOverride*)madtEntry)->source;
      pIntOverrideNode->intOverride.interrupt =
        ((S_APICInterruptOverride*)madtEntry)->interrupt;
      pIntOverrideNode->intOverride.flags =
        ((S_APICInterruptOverride*)madtEntry)->flags;

      /* Link the node */
      ADD_TO_LIST(sDrvCtrl.pIntOverrideList,
                  pIntOverrideListCursor,
                  pIntOverrideNode);
      ++sDrvCtrl.detectedIntOverrideCount;
    }
    madtEntry += pHeader->length;
  }
}

static void _ParseHPET(const S_ACPIHPETDescriptor* kpHpetPtr)
{
  int32_t     sum;
  uint32_t    i;
  S_HPETNode* pHpetNode;
  S_HPETNode* pHpetListCursor;

  ACPI_ASSERT(kpHpetPtr != NULL, "Parse a NULL HPET", ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpHpetPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpHpetPtr)[i];
  }

  ACPI_ASSERT((sum & 0xFF) == 0, "HPET Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpHpetPtr->header.pSignature) == ACPI_HPET_SIG,
              "HPET Signature comparison failed",
              ERR_INVALID_VALUE);

  /* Create the new HPET node */
  pHpetNode = KMalloc(sizeof(S_HPETNode), ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);

  /* Fill the descriptor */
  pHpetNode->hpet.hwRev         = (kpHpetPtr->flags & HPET_FLAGS_HW_REV_MASK) >>
                                   HPET_FLAGS_HW_REV_SHIFT;
  pHpetNode->hpet.comparatorCount = (kpHpetPtr->flags & HPET_FLAGS_CC_MASK) >>
                                      HPET_FLAGS_CC_SHIFT;
  pHpetNode->hpet.counterSize     = (kpHpetPtr->flags & HPET_FLAGS_CS_MASK) >>
                                      HPET_FLAGS_CS_SHIFT;
  pHpetNode->hpet.legacyRepIRQ    = (kpHpetPtr->flags & HPET_FLAGS_IRQ_MASK) >>
                                     HPET_FLAGS_IRQ_SHIFT;
  pHpetNode->hpet.pciVendorId     = kpHpetPtr->pciVendorId;
  pHpetNode->hpet.hpetNumber      = kpHpetPtr->hpetNumber;
  pHpetNode->hpet.minimumTick     = kpHpetPtr->minimumTick;
  pHpetNode->hpet.pageProtection  = kpHpetPtr->pageProtection;
  pHpetNode->hpet.address         = (uintptr_t)kpHpetPtr->address.address;
  pHpetNode->hpet.addressSpace    = kpHpetPtr->address.addressSpace;
  pHpetNode->hpet.bitWidth        = kpHpetPtr->address.bitWidth;
  pHpetNode->hpet.bitOffset       = kpHpetPtr->address.bitOffset;
  pHpetNode->hpet.accessSize      = kpHpetPtr->address.accessSize;

  /* Link the node */
  ADD_TO_LIST(sDrvCtrl.pHpetList, pHpetListCursor, pHpetNode);
  ++sDrvCtrl.detectedHpetCount;
}

static uint8_t _GetLAPICCount(void)
{
  return sDrvCtrl.detectedCPUCount;
}

const S_LAPICNode* _GetLAPICList(void)
{
  return sDrvCtrl.pLAPICList;
}

static uintptr_t _GetLAPICBaseAddress(void)
{
  return sDrvCtrl.localApicAddress;
}

static uint8_t _GetIOAPICCount(void)
{
  return sDrvCtrl.detectedIOAPICCount;
}

static const S_IOAPICNode* _GetIOAPICList(void)
{
  return sDrvCtrl.pIoApicList;
}

static const S_HPETNode* _GetHPETList(void)
{
  return sDrvCtrl.pHpetList;
}

static uint32_t _GetRemapedIRQ(const uint32_t kIRQNumber)
{
  const S_InterruptOverrideNode* kpOverride;
  uint32_t                   retValue;

  /* Search for the override */
  retValue   = kIRQNumber;
  kpOverride = sDrvCtrl.pIntOverrideList;
  while (kpOverride != NULL)
  {
    if (kpOverride->intOverride.source == kIRQNumber)
    {
      retValue = kpOverride->intOverride.interrupt;
      break;
    }
    kpOverride = kpOverride->pNext;
  }

  /* If we did not find the interrupt, there is no redirection. */
  return retValue;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86ACPIDriver);

/************************************ EOF *************************************/