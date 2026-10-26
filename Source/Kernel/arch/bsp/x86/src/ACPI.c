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
#include <X64Cpu.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ProcFS.h>
#include <Memory.h>
#include <SystemBSP.h>
#include <DeviceTree.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <ACPI.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for regs  */
#define ACPI_FDT_REGS_PROP "reg"

/** @brief ACPI Procfs directory path */
#define ACPI_PROCFS_DIR_PATH "acpi"
/** @brief Length of the PROCFS string */
#define PROCFS_STRING_LENGTH 1024

/** @brief Module name */
#define MODULE_NAME "X86 ACPI"

/** @brief ACPI table: Root System Description Pointer */
#define ACPI_RSDP_SIG 0x2052545020445352ULL /* "RSD PTR " (64-bit signature) */
/** @brief ACPI table: Root System Description Table */
#define ACPI_RSDT_SIG 0x54445352 /* "RSDT" */
/** @brief ACPI table: Extended System Description Table */
#define ACPI_XSDT_SIG 0x54445358 /* "XSDT" */
/** @brief ACPI table: Fixed ACPI Description Table */
#define ACPI_FADT_SIG 0x50434146 /* "FACP" */
/** @brief ACPI table: Firmware ACPI Control Structure */
#define ACPI_FACS_SIG 0x53434146 /* "FACS" */
/** @brief ACPI table: Differentiated System Description Table */
#define ACPI_DSDT_SIG 0x54445344 /* "DSDT" */
/** @brief ACPI table: Secondary System Description Table */
#define ACPI_SSDT_SIG 0x54445353 /* "SSDT" */
/** @brief ACPI table: Multiple APIC Description Table */
#define ACPI_MADT_SIG 0x43495041 /* "APIC" */
/** @brief ACPI table: Smart Battery Specification Table */
#define ACPI_SBST_SIG 0x54534253 /* "SBST" */
/** @brief ACPI table: System Resource Affinity Table */
#define ACPI_SRAT_SIG 0x54415253 /* "SRAT" */
/** @brief ACPI table: System Locality Distance Information Table */
#define ACPI_SLIT_SIG 0x54494C53 /* "SLIT" */
/** @brief ACPI table: Memory Power State Table */
#define ACPI_MPST_SIG 0x5453504D /* "MPST" */
/** @brief ACPI table: NVDIMM Firmware Interface Table */
#define ACPI_NFIT_SIG 0x5449464E /* "NFIT" */
/** @brief ACPI table: Heterogeneous Memory Attribute Table */
#define ACPI_HMAT_SIG 0x54414D48 /* "HMAT" */
/** @brief ACPI table: Processor Properties Topology Table */
#define ACPI_PPTT_SIG 0x54545050 /* "PPTT" */
/** @brief ACPI table: Platform Health Performance Monitoring Table */
#define ACPI_PHAT_SIG 0x54414850 /* "PHAT" */
/** @brief ACPI table: Cache Coherency Interconnect Topology Table */
#define ACPI_CCIT_SIG 0x54494343 /* "CCIT" */
/** @brief ACPI table: Component Measurement Log Table */
#define ACPI_CMLT_SIG 0x544C4D43 /* "CMLT" */
/** @brief ACPI table: Boot Error Record Table */
#define ACPI_BERT_SIG 0x54524542 /* "BERT" */
/** @brief ACPI table: Error Injection Table */
#define ACPI_EINJ_SIG 0x4A4E4945 /* "EINJ" */
/** @brief ACPI table: Error Record Serialization Table */
#define ACPI_ERST_SIG 0x54535245 /* "ERST" */
/** @brief ACPI table: Hardware Error Source Table */
#define ACPI_HEST_SIG 0x54534548 /* "HEST" */
/** @brief ACPI table: Platform Communications Channel Table */
#define ACPI_PCCT_SIG 0x54434350 /* "PCCT" */
/** @brief ACPI table: Boot Graphics Resource Table */
#define ACPI_BGRT_SIG 0x54524742 /* "BGRT" */
/** @brief ACPI table: Corrected Platform Error Polling Table */
#define ACPI_CPEP_SIG 0x50455043 /* "CPEP" */
/** @brief ACPI table: CSR Descriptor Table */
#define ACPI_CSRT_SIG 0x54525343 /* "CSRT" */
/** @brief ACPI table: Debug Port Table 2 */
#define ACPI_DBG2_SIG 0x32474244 /* "DBG2" */
/** @brief ACPI table: Debug Port Table 1 */
#define ACPI_DBGP_SIG 0x50474244 /* "DBGP" */
/** @brief ACPI table: Data Management Table */
#define ACPI_DMAR_SIG 0x52414D44 /* "DMAR" */
/** @brief ACPI table: Direct Memory Access Remapping Table */
#define ACPI_DRTM_SIG 0x4D545244 /* "DRTM" */
/** @brief ACPI table: Embedded Controller Boot Resources Table */
#define ACPI_ECDT_SIG 0x54444345 /* "ECDT" */
/** @brief ACPI table: Firmware Performance Data Table */
#define ACPI_FPDT_SIG 0x54445046 /* "FPDT" */
/** @brief ACPI table: Generic Timer Description Table */
#define ACPI_GTDT_SIG 0x54445447 /* "GTDT" */
/** @brief ACPI table: Memory Map Resource Allocation Table */
#define ACPI_MCFG_SIG 0x4746434D /* "MCFG" */
/** @brief ACPI table: Maximum System Characteristics Table */
#define ACPI_MSCT_SIG 0x5443534D /* "MSCT" */
/** @brief ACPI table: PCI Root Bridge Digital Properties Table */
#define ACPI_PRMT_SIG 0x544D5250 /* "PRMT" */
/** @brief ACPI table: RAS Extensibility Architecture Table */
#define ACPI_RAST_SIG 0x54534152 /* "RAST" */
/** @brief ACPI table: Serial Port Console Redirection Table */
#define ACPI_SPCR_SIG 0x52435053 /* "SPCR" */
/** @brief ACPI table: Watchdog Action Table */
#define ACPI_WDAT_SIG 0x54414457 /* "WDAT" */
/** @brief ACPI table: Windows SMM Security Mitigations Table */
#define ACPI_WSMT_SIG 0x544D5357 /* "WSMT" */
/** @brief ACPI table: High Precision Event Timer Table */
#define ACPI_HPET_SIG 0x54455048 /* "HPET" */
/** @brief ACPI table: Trusted Platform Module 2 Table */
#define ACPI_TPM2_SIG 0x324D5054 /* "TPM2" */
/** @brief ACPI table: CXL Early Discovery Table */
#define ACPI_CEDT_SIG 0x54444543 /* "CEDT" */
/** @brief ACPI table: I/O Virtualization Table (ACPI 6.6 New addition) */
#define ACPI_IOVT_SIG 0x54564F49 /* "IOVT" */
/** @brief ACPI table: Software Licensing Table (ACPI 6.6 New addition) */
#define ACPI_SWFT_SIG 0x54465753 /* "SWFT" */
/** @brief ACPI table: Keypad Configuration Table (ACPI 6.6 New addition) */
#define ACPI_KEYP_SIG 0x5059454B /* "KEYP" */

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

/** @brief S5 package name */
#define S5_PACKAGE_NAME 0x5F35535F
/** @brief _PRT package name */
#define PRT_PACKAGE_NAME 0x5452505F

/** @brief Address space: System Memory */
#define ADDR_SPACE_SYSTEM_MEMORY 0

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
  /** @brief DSDT AML code */
  uint8_t aml[];
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

typedef struct
{
  /** @brief Base address of the enhanced configuration space */
  uint64_t baseAddress;
  /** @brief PCI segment group number */
  uint16_t pciSegmentGroupNumber;
  /** @brief Start bus number */
  uint8_t startBusNumber;
  /** @brief End bus number */
  uint8_t endBusNumber;
  /** @brief Reserved field */
  uint32_t reserved;
} __attribute__((__packed__)) S_MCFGDescriptorConfigSpace;

/** @brief ACPI MCFG descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
  /** @brief MCFG header */
  S_ACPIHeader header;
  /** @brief Reserved field */
  uint64_t reserved;
  /** @brief Array of configuration spaces */
  S_MCFGDescriptorConfigSpace configSpaces[];
} __attribute__((__packed__)) S_MCFGDescriptor;

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

/** @brief x86 ACPI driver controller. */
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
  /** @brief List of the detected PCI configuration spaces. */
  S_PCIConfigNode* pPCIConfigList;
  /** @brief ACPI version */
  uint32_t version;
  /** @brief ACPI base address */
  uintptr_t baseAddress;
  /** @brief Copy of the FADT structure. */
  S_FADT fadt;
  /** @brief Shutdown register values */
  uint16_t shutdownRegValue[2];
} S_ACPIControler;

/** @brief AML package structure. */
typedef struct
{
  /** @brief Package name operation */
  uint8_t nameOp;
  /** @brief Package signature */
  uint32_t signature;
  /** @brief Package type */
  uint8_t type;
  /** @brief Package length */
  uint8_t length;
  /** @brief Package elements count */
  uint8_t elementsCount;
  /** @brief Package elements */
  uint8_t elements[];
} __attribute__((__packed__)) S_Package;

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
  if ((COND) == false)                                  \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);       \
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
  if (CURSOR != NULL)                         \
  {                                           \
    while (CURSOR->pNext != NULL)             \
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
 * @param[in] kpFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* kpFdtNode);

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
 * @brief Parses the APIC entries of the MCFG table.
 *
 * @details Parse the APIC entries of the MCFG table.The function will parse
 * each entry and add the MCFG node to the list of detected MCFG entries.
 *
 * @param[in] kpMcfgPtr The address of the MCFG entry to parse.
 */
static void _ParseMCFG(const S_MCFGDescriptor* kpMcfgPtr);

/**
 * @brief Parses the APIC entries of the DSDT table.
 *
 * @details Parse the APIC entries of the DSDT table.The function will parse
 * each entry and add the DSDT node to the list of detected DSDT entries.
 *
 * @param[in] pDsdtPtr The address of the DSDT entry to parse.
 */
static void _ParseDSDT(S_DSDT* pDsdtPtr);

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

/**
 * @brief Returns the list of detected PCI configuration spaces.
 *
 * @details Returns the list of detected PCI configuration spaces. This list
 * should not be modified and is generated during the attach of the ACPI while
 * parsing its tables.
 *
 * @return The list of detected PCI configuration spaces is returned.
 */
static const S_PCIConfigNode* _GetPCIConfigList(void);

/**
 * @brief Opens the ACPI ProcFS main entry.
 *
 * @details Opens the ACPI ProcFS main entry. This function will open the ACPI
 * ProcFS main entry and return a pointer to the file handle. The file handle
 * will be used to read the ACPI information from the kernel.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] kpPath The path to the ProcFS entry.
 * @param[in] flags The flags for opening the file.
 * @param[in] mode The mode for opening the file.
 *
 * @return The pointer to the file handle or -1 in case of error.
 */
static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode);

/**
 * @brief Closes the ACPI ProcFS main entry.
 *
 * @details Closes the ACPI ProcFS main entry. This function will close the ACPI
 * ProcFS main entry and free the resources used by the entry.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle);

/**
 * @brief Read the ACPI ProcFS main entry.
 *
 * @details Read the ACPI ProcFS main entry. This function will read the ACPI
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pDriverData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the ACPI information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count);

/**
 * @brief Creates the ProcFS entry for the ACPI driver.
 *
 * @details Creates the ProcFS entry for the ACPI driver. This function will
 * create the ProcFS entry for the ACPI driver and register it in the ProcFS.
 * The entry will be used to read the ACPI information from the kernel. The
 * entry will be created in the /proc/acpi directory.
 *
 * @return The success state or the error code.
 */
static E_Return _CreateProcFSEntry(void);

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

/** @brief ACPI driver controller instance */
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
  .pPCIConfigList           = NULL,
  .version                  = 0,
  .baseAddress              = 0,
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
  .pGetPCIConfigList    = _GetPCIConfigList,
};

/** @brief PROCFS main entry */
static S_ProcFSDirEntry* spProcFSMainEntry = NULL;

/** @brief PROCFS operations */
static S_ProcFSFileOperations sProcFSOps =
{
  .pOpen    = _ProcFSOpen,
  .pClose   = _ProcFSClose,
  .pRead    = _ProcFSRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief PROCFS string */
static char sProcFSString[PROCFS_STRING_LENGTH] = {0};
/** @brief Length of the PROCFS string */
static size_t sProcFSStringLength = 0;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* kpFdtNode)
{
  const uintptr_t*         kpUintptrProp;
  size_t                   propLen;
  E_Return                 retCode;
  uintptr_t                searchRangeStart;
  uintptr_t                searchRangeEnd;
  size_t                   mapSize;
  uintptr_t                mapBase;
  uintptr_t                mapPhys;
  size_t                   initSize;
  uint64_t                 signature;

  /* Get the reg: the range to search for the ACPI structure */
  kpUintptrProp = FDTGetProp(kpFdtNode, ACPI_FDT_REGS_PROP, &propLen);

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
      sDrvCtrl.baseAddress = searchRangeStart;
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
  retCode = DriverManagerSetDeviceData(kpFdtNode, &sAPIDriver);
  ACPI_ASSERT(retCode == NO_ERROR, "Failed to register ACPI", retCode);

  /* Update the CPU count */
  CPUSetCount(sAPIDriver.pGetLAPICCount());

  /* Register the PROCFS entries */
  retCode = _CreateProcFSEntry();
  ACPI_ASSERT(retCode == NO_ERROR, "Failed to create ProcFS entries", retCode);

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
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "ACPI Revision 0 Detected");
    _ParseRSDPRevision0(kpRsdpDesc);

    sDrvCtrl.version = 0;
  }
  else if (kpRsdpDesc->revision == 2)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "ACPI Revision 2 Detected");
    _ParseRSDPRevision2(kpRsdpDesc);

    sDrvCtrl.version = 2;
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
  if (*((uint32_t*)kpHeader->pSignature) == ACPI_FADT_SIG)
  {
    _ParseFADT((S_FADT*)descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_FACS_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "FACS found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_SSDT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SSDT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_MADT_SIG)
  {
    _ParseMADT((S_MADT*)descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_SBST_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SBST found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_SRAT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SRAT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_SLIT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SLIT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_MPST_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "MPST found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_NFIT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "NFIT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_HMAT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "HMAT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_PPTT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PPTT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_PHAT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PHAT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_CCIT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "CCIT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_CMLT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "CMLT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_BERT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "BERT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_EINJ_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "EINJ found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_ERST_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "ERST found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_HEST_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "HEST found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_PCCT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PCCT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_BGRT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "BGRT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_CPEP_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "CPEP found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_CSRT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "CSRT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_DBG2_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DBG2 found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_DBGP_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DBGP found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_DMAR_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DMAR found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_DRTM_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "DRTM found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_ECDT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "ECDT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_FPDT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "FPDT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_GTDT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "GTDT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_MCFG_SIG)
  {
    _ParseMCFG((S_MCFGDescriptor*)descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_MSCT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "MSCT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_PRMT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "PRMT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_RAST_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "RAST found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_SPCR_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SPCR found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_WDAT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "WDAT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_WSMT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "WSMT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_HPET_SIG)
  {
    _ParseHPET((S_ACPIHPETDescriptor*)descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_TPM2_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "TPM2 found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_CEDT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "CEDT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_IOVT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IOVT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_SWFT_SIG)
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "SWFT found at 0x%p",
                 descPtr);
  }
  else if (*((uint32_t*)kpHeader->pSignature) == ACPI_KEYP_SIG)
  {

  }
  else
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Unknown table detected at 0x%p: 0x%08x",
                 descPtr,
                 *((uint32_t*)kpHeader->pSignature));
  }

  /* Unmap memory */
  errCode = MemoryKernelUnmap((void*)descAddr, toMap);
  ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap DT", errCode);
}

static void _ParseFADT(const S_FADT* kpFadtPtr)
{
  int32_t     sum;
  uint32_t    i;
  uintptr_t   dsdtAddr;
  S_GAddress* pDsdtGAddress;
  S_DSDT*     pDsdt;
  void*       pDsdtMap;
  size_t      toMap;
  size_t      initSize;
  E_Return    errCode;
  uintptr_t   offset;

  ACPI_ASSERT(kpFadtPtr != NULL,
              "Tried to parse a NULL FADT",
              ERR_INVALID_PARAMETER);

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "ACPI FADT at 0x%p",
               kpFadtPtr);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpFadtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpFadtPtr)[i];
  }

  ACPI_ASSERT((sum & 0xFF) == 0, "FADT Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpFadtPtr->header.pSignature) == ACPI_FADT_SIG,
              "FADT Signature comparison failed",
              ERR_INVALID_VALUE);

  /* Save the FADT pointer */
  memcpy(&sDrvCtrl.fadt, kpFadtPtr, sizeof(S_FADT));


  /* Parse the DSDT */
  dsdtAddr = 0;
  if (kpFadtPtr->header.length >= 148)
  {
    pDsdtGAddress = (S_GAddress*)&kpFadtPtr->xDsdt;
    if (pDsdtGAddress->addressSpace == ADDR_SPACE_SYSTEM_MEMORY)
    {
      if (pDsdtGAddress->address != 0)
      {
        KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "    FADT DSDT address is 0x%p",
                     pDsdtGAddress->address);
        dsdtAddr = (uintptr_t)pDsdtGAddress->address;

      }
      else
      {
        KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                     MODULE_NAME,
                     "    FADT DSDT address is 32 Bits");
      }
    }
    else
    {
      KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "    FADT DSDT address is 32 Bits");
    }
  }
  else
  {
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "    FADT DSDT address is 32 Bits");
  }

  /* Fallback to the 32-bit DSDT address in the FADT */
  if (dsdtAddr == 0)
  {
    dsdtAddr = (uintptr_t)kpFadtPtr->dsdt;
  }

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "    FADT DSDT address is 0x%p",
               dsdtAddr);
  initSize = KERNEL_PAGE_SIZE;
  offset = dsdtAddr & PAGE_SIZE_MASK;
  if (offset < sizeof(S_ACPIHeader))
  {
    initSize += KERNEL_PAGE_SIZE;
  }
  pDsdtMap = MemoryKernelMap((void*)((uintptr_t)dsdtAddr & ~PAGE_SIZE_MASK),
                          initSize,
                          MEMMGR_MAP_HARDWARE |
                          MEMMGR_MAP_KERNEL   |
                          MEMMGR_MAP_RO,
                          &errCode);
  ACPI_ASSERT(errCode == NO_ERROR, "Failed to map DSDT", errCode);

  pDsdt = (S_DSDT*)((uintptr_t)pDsdtMap + offset);
  toMap = ((uintptr_t)pDsdt & PAGE_SIZE_MASK) + pDsdt->header.length;
  if (toMap % KERNEL_PAGE_SIZE)
  {
    toMap = (toMap + KERNEL_PAGE_SIZE) & ~PAGE_SIZE_MASK;
  }
  errCode = MemoryKernelUnmap(pDsdtMap, initSize);
  ACPI_ASSERT(errCode == NO_ERROR, "Failed to unmap DSDT", errCode);

  pDsdtMap = MemoryKernelMap((void*)((uintptr_t)dsdtAddr & ~PAGE_SIZE_MASK),
                             toMap,
                             MEMMGR_MAP_HARDWARE |
                             MEMMGR_MAP_KERNEL   |
                             MEMMGR_MAP_RW,
                             &errCode);
  pDsdt = (S_DSDT*)((uintptr_t)pDsdtMap + offset);
  ACPI_ASSERT(errCode == NO_ERROR, "Failed to map DSDT", errCode);

  _ParseDSDT(pDsdt);
}

static void _ParseMADT(const S_MADT* kpMadtPtr)
{
  int32_t                  sum;
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
              "Tried to parse a NULL MADT",
              ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpMadtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpMadtPtr)[i];
  }

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "ACPI MADT at 0x%p",
               kpMadtPtr);

  ACPI_ASSERT((sum & 0xFF) == 0, "MADT checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpMadtPtr->header.pSignature) == ACPI_MADT_SIG,
              "Invalid MADT signature",
              ERR_INVALID_VALUE);

  madtEntry = (uintptr_t)(kpMadtPtr + 1);
  madtLimit = ((uintptr_t)kpMadtPtr) + kpMadtPtr->header.length;

  /* Get the LAPIC address */
  sDrvCtrl.localApicAddress = kpMadtPtr->localApicAddr;

  while (madtEntry < madtLimit)
  {
    /* Get entry header */
    pHeader = (S_APICHeader*)madtEntry;

    /* Check entry type */
    if (pHeader->type == APIC_TYPE_LOCAL_APIC)
    {
      KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "    Local APIC CPU %d",
                   sDrvCtrl.detectedCPUCount);

      /* Create new LAPIC node */
      pLAPICNode = KMalloc(sizeof(S_LAPICNode), KMALLOC_NO_FREE_POOL);

      /* Fill the descriptor */
      pLAPICNode->lapic.lapicId = ((S_LAPIC*)madtEntry)->lapicId;
      pLAPICNode->lapic.cpuId   = ((S_LAPIC*)madtEntry)->cpuId;
      pLAPICNode->lapic.flags   = ((S_LAPIC*)madtEntry)->flags;

      /* Link the node */
      ADD_TO_LIST(sDrvCtrl.pLAPICList, pLAPICListCursor, pLAPICNode);
      ++sDrvCtrl.detectedCPUCount;
    }
    else if (pHeader->type == APIC_TYPE_IO_APIC)
    {
      KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "    IO-APIC %d Detected",
                   sDrvCtrl.detectedIOAPICCount);

      /* Create new IO APIC node */
      pIOApicNode = KMalloc(sizeof(S_IOAPICNode), KMALLOC_NO_FREE_POOL);

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

      KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "    Interrupt Override %d => %d",
                   pIntOverrideNode->intOverride.source,
                   pIntOverrideNode->intOverride.interrupt);
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

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "ACPI HPET at 0x%p",
               kpHpetPtr);

  ACPI_ASSERT((sum & 0xFF) == 0, "HPET Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpHpetPtr->header.pSignature) == ACPI_HPET_SIG,
              "HPET Signature comparison failed",
              ERR_INVALID_VALUE);

  /* Create the new HPET node */
  pHpetNode = KMalloc(sizeof(S_HPETNode), KMALLOC_NO_FREE_POOL);

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

static void _ParseMCFG(const S_MCFGDescriptor* kpMcfgPtr)
{
  int32_t          sum;
  uint32_t         i;
  uint32_t         toParse;
  S_PCIConfigNode* pNode;
  S_PCIConfigNode* pCursor;

  ACPI_ASSERT(kpMcfgPtr != NULL, "Parse a NULL MCFG", ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < kpMcfgPtr->header.length; ++i)
  {
    sum += ((uint8_t*)kpMcfgPtr)[i];
  }

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "ACPI MCFG at 0x%p",
               kpMcfgPtr);

  ACPI_ASSERT((sum & 0xFF) == 0, "MCFG Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)kpMcfgPtr->header.pSignature) == ACPI_MCFG_SIG,
              "MCFG Signature comparison failed",
              ERR_INVALID_VALUE);

  toParse = kpMcfgPtr->header.length - sizeof(S_ACPIHeader) - sizeof(uint64_t);
  toParse /= sizeof(S_MCFGDescriptorConfigSpace);
  for (i = 0; i < toParse; ++i)
  {
    /* Create the new node */
    pNode = KMalloc(sizeof(S_PCIConfigNode), KMALLOC_NO_FREE_POOL);
    pNode->pciConfig.baseAddress = kpMcfgPtr->configSpaces[i].baseAddress;
    pNode->pciConfig.pciSegmentGroupNumber = kpMcfgPtr->configSpaces[i].pciSegmentGroupNumber;
    pNode->pciConfig.startBusNumber = kpMcfgPtr->configSpaces[i].startBusNumber;
    pNode->pciConfig.endBusNumber = kpMcfgPtr->configSpaces[i].endBusNumber;

    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "    PCI Config Space %d: Base Address: 0x%p, "
                 "Segment Group: %d, Start Bus: %d, End Bus: %d",
                 i,
                 pNode->pciConfig.baseAddress,
                 pNode->pciConfig.pciSegmentGroupNumber,
                 pNode->pciConfig.startBusNumber,
                 pNode->pciConfig.endBusNumber);

    ADD_TO_LIST(sDrvCtrl.pPCIConfigList, pCursor, pNode);
  }
}

uint32_t parse_aml_pkg_length(const uint8_t* stream, uint32_t* offset);
uint32_t parse_aml_pkg_length(const uint8_t* stream, uint32_t* offset) {
    uint8_t lead_byte = stream[*offset];
    (*offset)++;

    // Rule 1: Standard 1-byte length
    // If bits 6 and 7 are 0, the length is just the lower 6 bits (max value 63)
    if ((lead_byte & 0xC0) == 0) {
        return lead_byte & 0x3F;
    }

    // Rule 2: Multi-byte length (2, 3, or 4 bytes follow)
    // Bits 6-7 tell us how many extra bytes represent this integer
    uint8_t extra_bytes = (lead_byte >> 6) & 0x03;
    uint32_t length = lead_byte & 0x0F; // Initialize with the lower 4 bits

    for (uint8_t i = 0; i < extra_bytes; i++) {
        length |= (stream[*offset] << (4 + (i * 8)));
        (*offset)++; // Advance the stream index past this length byte
    }

    return length;
}

static void _ParseDSDT(S_DSDT* pDsdtPtr)
{
  int32_t          sum;
  uint32_t         i;
  uint8_t*         memCursor;
  S_Package*       pPackage;

  ACPI_ASSERT(pDsdtPtr != NULL, "Parse a NULL DSDT", ERR_INVALID_PARAMETER);

  /* Verify checksum */
  sum = 0;
  for (i = 0; i < pDsdtPtr->header.length; ++i)
  {
    sum += ((uint8_t*)pDsdtPtr)[i];
  }

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "ACPI DSDT at 0x%p",
               pDsdtPtr);

  ACPI_ASSERT((sum & 0xFF) == 0, "DSDT Checksum failed", ERR_INVALID_VALUE);
  ACPI_ASSERT(*((uint32_t*)pDsdtPtr->header.pSignature) == ACPI_DSDT_SIG,
              "DSDT Signature comparison failed",
              ERR_INVALID_VALUE);

  /* Search for the S5 sequence to get the register values */
  memCursor = pDsdtPtr->aml;
  for (i = 0;
       i < pDsdtPtr->header.length - sizeof(S_ACPIHeader) - sizeof(uint32_t);
       ++i)
  {
    /* Check if we found the S5 package */
    if (*((uint32_t*)&memCursor[i]) == S5_PACKAGE_NAME)
    {
      pPackage = (S_Package*)&memCursor[i-1];
      sDrvCtrl.shutdownRegValue[0] = (uint16_t)pPackage->elements[1];
      sDrvCtrl.shutdownRegValue[1] = (uint16_t)pPackage->elements[3];

      KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                   MODULE_NAME,
                   "    Found S5 Package Shutdown Values: 0x%04X, 0x%04X",
                   sDrvCtrl.shutdownRegValue[0],
                   sDrvCtrl.shutdownRegValue[1]);
    }
  }
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

static const S_PCIConfigNode* _GetPCIConfigList(void)
{
  return sDrvCtrl.pPCIConfigList;
}

static void* _ProcFSOpen(void*       pDriverData,
                         const char* kpPath,
                         int32_t     flags,
                         int32_t     mode)
{
  size_t* pEntryOffset;

  (void)pDriverData;
  (void)mode;

  if (flags == O_RDONLY && *kpPath == 0)
  {
    pEntryOffset = KMallocUser(sizeof(size_t), NULL);
    if (pEntryOffset == NULL)
    {
      pEntryOffset = (void*)-1;
    }
    else
    {
      *pEntryOffset = 0;
    }
  }
  else
  {
    pEntryOffset = (void*)-1;
  }

  return pEntryOffset;
}

static int32_t _ProcFSClose(void* pDriverData, void* pFileHandle)
{
  int32_t retCode;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    KFreeUser(pFileHandle, NULL);
    retCode = 0;
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSRead(void*  pDriverData,
                           void*  pFileHandle,
                           void*  pBuffer,
                           size_t count)
{
  int32_t retCode;
  size_t* pEntryOffset;

  (void)pDriverData;

  if (pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    pEntryOffset = (size_t*)pFileHandle;

    if (*pEntryOffset < sProcFSStringLength)
    {
      retCode = (int32_t)MIN(count, sProcFSStringLength - *pEntryOffset);
      memcpy(pBuffer, sProcFSString + *pEntryOffset, retCode);
      *pEntryOffset += retCode;
    }
    else
    {
      retCode = 0;
    }
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static E_Return _CreateProcFSEntry(void)
{
  E_Return                 retCode;
  uint32_t                 i;
  S_InterruptOverrideNode* pIntOverrideNode;

  retCode = ProcFSCreateEntry(ACPI_PROCFS_DIR_PATH,
                              0,
                              NULL,
                              &sProcFSOps,
                              NULL,
                              &spProcFSMainEntry);
  if (retCode == NO_ERROR)
  {
    sProcFSStringLength = snprintf(sProcFSString,
                                  PROCFS_STRING_LENGTH,
                                  "Version: %d\n"
                                  "Base Address: 0x%p\n"
                                  "LAPIC Count: %d\n"
                                  "IO-APIC Count: %d\n"
                                  "HPET Count: %d\n",
                                  sDrvCtrl.version,
                                  sDrvCtrl.baseAddress,
                                  sDrvCtrl.detectedCPUCount,
                                  sDrvCtrl.detectedIOAPICCount,
                                  sDrvCtrl.detectedHpetCount);
    sProcFSStringLength += snprintf(sProcFSString + sProcFSStringLength,
                                    PROCFS_STRING_LENGTH - sProcFSStringLength,
                                  "Interrupt Overrides: %d\n",
                                    sDrvCtrl.detectedIntOverrideCount);
    pIntOverrideNode = sDrvCtrl.pIntOverrideList;
    for (i = 0; i < sDrvCtrl.detectedIntOverrideCount; ++i)
    {
      sProcFSStringLength += snprintf(sProcFSString + sProcFSStringLength,
                                      PROCFS_STRING_LENGTH - sProcFSStringLength,
                                      "  Bus: %d, Source: %d, Destination: %d, "
                                      "Flags: 0x%X\n",
                                      pIntOverrideNode->intOverride.bus,
                                      pIntOverrideNode->intOverride.source,
                                      pIntOverrideNode->intOverride.interrupt,
                                      pIntOverrideNode->intOverride.flags);
      pIntOverrideNode = pIntOverrideNode->pNext;
    }
  }

  return retCode;
}

void SystemShutdown(void)
{
  uint16_t value;

  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Shutting down the system via ACPI");

  value = 0x2000 | (sDrvCtrl.shutdownRegValue[0] << 10);
  KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Writing 0x%04X to PM1a Control Block at 0x%p",
               value,
               sDrvCtrl.fadt.pm1aControlBlock);


  CPUPortWriteWord(value, sDrvCtrl.fadt.pm1aControlBlock);

  /* Second word if needed */
  if (sDrvCtrl.shutdownRegValue[1] != 0 &&
      sDrvCtrl.fadt.pm1bControlBlock != 0)
  {
    value = 0x2000 | (sDrvCtrl.shutdownRegValue[1] << 10);
    KERNEL_DEBUG(ACPI_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Writing 0x%04X to PM1b Control Block at 0x%p",
                 value,
                 sDrvCtrl.fadt.pm1bControlBlock);

    CPUPortWriteWord(value, sDrvCtrl.fadt.pm1bControlBlock);
  }

  while (1)
  {
    CPUInterruptDisable();
    CPUHalt();
  }
}


/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86ACPIDriver);

/************************************ EOF *************************************/