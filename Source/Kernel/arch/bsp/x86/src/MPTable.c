/*******************************************************************************
 * @file MPTable.c
 *
 * @see MPTable.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 09/10/2026
 *
 * @version 1.0
 *
 * @brief Kernel's MP (Multi-Processor) table driver.
 *
 * @details Kernel's MP (Multi-Processor) table driver. Defines
 * the functions and definitions to interact with the MP table and its devices.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <Panic.h>
#include <X64Cpu.h>
#include <Memory.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <MPTable.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Module name */
#define MODULE_NAME "X86 MPTABLE"


/** @brief BIOS base address */
#define BIOS_BASE_ADDRESS  0x000F0000
/** @brief BIOS base memory size */
#define BIOS_BASE_MEM_SIZE 0x10000
/** @brief SYS base address */
#define SYS_BASE_ADDRESS  0x0009FC00
/** @brief SYS base memory size */
#define SYS_BASE_MEM_SIZE 0x400
/** @brief BIOS EBDA pointer address */
#define BIOS_EBDA_ADDRESS  0x0000040E
/** @brief BIOS EBDA memory size */
#define BIOS_EBDA_MEM_SIZE 0x00001000

/** @brief IMCR chip present flag */
#define IMCR_CHIP_PRESENT (1 << 7)

/** @brief MPTable pointer signature */
#define MP_TABLE_POINTER_SIGNATURE "_MP_"
/** @brief MPTable signature */
#define MP_TABLE_SIGNATURE "PCMP"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief MPTable pointer structure. */
typedef struct
{
  /** @brief MPTable pointer signature */
  char pSignature[4];
  /** @brief MPTable pointer address */
  uint32_t mpTablePointerAddress;
  /** @brief MPTable pointer length */
  uint8_t length;
  /** @brief MPTable pointer version */
  uint8_t version;
  /** @brief MPTable pointer checksum */
  uint8_t checksum;
  /** @brief MPTable pointer default configuration type */
  uint8_t defaultConfigType;
  /** @brief MPTable pointer feature bytes */
  uint32_t featureBytes;
} S_MPTablePointer;

/** @brief MPTable header structure. */
typedef struct
{
  /** @brief MPTable signature */
  char pSignature[4];
  /** @brief MPTable length */
  uint16_t length;
  /** @brief MPTable version */
  uint8_t version;
  /** @brief MPTable checksum */
  uint8_t checksum;
  /** @brief MPTable OEM ID */
  char pOEM[8];
  /** @brief MPTable product ID */
  char pProductId[12];
  /** @brief MPTable OEM table pointer */
  uint32_t oemTablePointer;
  /** @brief MPTable OEM table size */
  uint16_t oemTableSize;
  /** @brief MPTable entry count */
  uint16_t entryCount;
  /** @brief MPTable local APIC address */
  uint32_t localApicAddress;
  /** @brief MPTable extended table length */
  uint16_t extendedTableLength;
  /** @brief MPTable extended table checksum */
  uint8_t extendedTableChecksum;
  /** @brief MPTable reserved */
  uint8_t reserved;
} S_MPTableHeader;

/** @brief MPTable Processor entry structure. */
typedef struct
{
  /** @brief Processor entry type */
  uint8_t entryType;
  /** @brief Processor local APIC ID */
  uint8_t localApicId;
  /** @brief Processor local APIC version */
  uint8_t localApicVersion;
  /** @brief Processor flags */
  uint8_t flags;
  /** @brief Processor signature */
  uint32_t signature;
  /** @brief Processor feature flags */
  uint32_t featureFlags;
  /** @brief Processor reserved */
  uint32_t reserved[2];
} S_ProcessorEntry;

/** @brief MPTable Bus entry structure. */
typedef struct
{
  /** @brief Bus entry type */
  uint8_t entryType;
  /** @brief Bus ID */
  uint8_t busId;
  /** @brief Bus type string */
  char pBusType[6];
} S_BusEntry;

/** @brief MPTable IO APIC entry structure. */
typedef struct
{
  /** @brief IO APIC entry type */
  uint8_t entryType;
  /** @brief IO APIC ID */
  uint8_t ioApicId;
  /** @brief IO APIC version */
  uint8_t ioApicVersion;
  /** @brief IO APIC flags */
  uint8_t flags;
  /** @brief IO APIC address */
  uint32_t ioApicAddress;
} S_IOAPICEntry;

/** @brief MPTable IO Interrupt Assignment entry structure. */
typedef struct
{
  /** @brief IO Interrupt Assignment entry type */
  uint8_t entryType;
  /** @brief IO Interrupt Assignment interrupt type */
  uint8_t interruptType;
  /** @brief IO Interrupt Assignment flags */
  uint16_t flags;
  /** @brief IO Interrupt Assignment source bus ID */
  uint8_t sourceBusId;
  /** @brief IO Interrupt Assignment source bus IRQ */
  uint8_t sourceBusIrq;
  /** @brief IO Interrupt Assignment destination IO APIC ID */
  uint8_t destIoApicId;
  /** @brief IO Interrupt Assignment destination IO APIC INTIN */
  uint8_t destIoApicIntin;
} S_IOInterruptAssignment;

/** @brief MPTable Local Interrupt Assignment entry structure. */
typedef struct
{
  /** @brief Local Interrupt Assignment entry type */
  uint8_t entryType;
  /** @brief Local Interrupt Assignment interrupt type */
  uint8_t interruptType;
  /** @brief Local Interrupt Assignment flags */
  uint16_t flags;
  /** @brief Local Interrupt Assignment source bus ID */
  uint8_t sourceBusId;
  /** @brief Local Interrupt Assignment source bus IRQ */
  uint8_t sourceBusIrq;
  /** @brief Local Interrupt Assignment destination local APIC ID */
  uint8_t destLocalApicId;
  /** @brief Local Interrupt Assignment destination local APIC LINTIN */
  uint8_t destLocalApicLintin;
} S_LocalInterruptAssignment;

/** @brief IO APIC Assignment structure. */
typedef struct S_IOAPICAssignement
{
  /** @brief Source bus ID */
  uint8_t sourceBusId;
  /** @brief Source bus IRQ */
  uint8_t sourceBusIrq;
  /** @brief Destination IO APIC ID */
  uint8_t destIoApicId;
  /** @brief Destination IO APIC INTIN */
  uint8_t destIoApicIntin;
  /** @brief Next assignment */
  struct S_IOAPICAssignement* pNext;
} S_IOAPICAssignement;

 /*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the MPTABLE to ensure correctness of execution.
 *
 * @details Assert macro used by the MPTABLE to ensure correctness of execution.
 * Due to the critical nature of the MPTABLE, any error generates a kernel
 * panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define MPTABLE_ASSERT(COND, MSG, ERROR) {              \
  if ((COND) == false)                                  \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);       \
  }                                                     \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Finds the MPTable pointer in the given memory range.
 *
 * @details Finds the MPTable pointer in the given memory range. The function
 * will search for the MPTable pointer signature in the given memory range.
 *
 * @param[in] startAddress The start address of the memory range to search.
 * @param[in] kSize The size of the memory range to search.
 *
 * @return The MPTable pointer if found, NULL otherwise.
 */
static S_MPTablePointer* _FindMPTablePointer(uint8_t*     startAddress,
                                             const size_t kSize);

/**
 * @brief Parses the MPTable pointer and its entries.
 *
 * @details Parses the MPTable pointer and its entries. The function will
 * validate the MPTable pointer and parse its entries.
 *
 * @param[in] pMPTablePointer The MPTable pointer to parse.
 */
static void _ParseMPTable(S_MPTablePointer* pMPTablePointer);

/**
 * @brief Parses the MPTable entries.
 *
 * @details Parses the MPTable entries. The function will parse the MPTable
 * entries and initialize the MPTable hardware.
 *
 * @param[in] pMPTableHeader The MPTable header to parse.
 */
static void _ParseMPTableEntries(S_MPTableHeader* pMPTableHeader);

/**
 * @brief Validates the MPTable checksum.
 *
 * @details Validates the MPTable checksum. The function will calculate the
 * checksum of the MPTable and compare it with the checksum in the MPTable
 * header.
 *
 * @param[in] pMPTableHeader The MPTable header to validate.
 */
static void _ValidateChecksum(S_MPTableHeader* pMPTableHeader);

/**
 * @brief Parses the MPTable Processor entry.
 *
 * @details Parses the MPTable Processor entry. The function will parse the
 * Processor entry and initialize the Processor hardware.
 *
 * @param[in] pEntry The Processor entry to parse.
 */
static void _ParseProcessorEntry(S_ProcessorEntry* pEntry);

/**
 * @brief Parses the MPTable Bus entry.
 *
 * @details Parses the MPTable Bus entry. The function will parse the
 * Bus entry and initialize the Bus hardware.
 *
 * @param[in] pEntry The Bus entry to parse.
 */
static void _ParseBusEntry(S_BusEntry* pEntry);

/**
 * @brief Parses the MPTable IO APIC entry.
 *
 * @details Parses the MPTable IO APIC entry. The function will parse the
 * IO APIC entry and initialize the IO APIC hardware.
 *
 * @param[in] pEntry The IO APIC entry to parse.
 */
static void _ParseIOAPICEntry(S_IOAPICEntry* pEntry);

/**
 * @brief Parses the MPTable IO Interrupt Assignment entry.
 *
 * @details Parses the MPTable IO Interrupt Assignment entry. The function will
 * parse the IO Interrupt Assignment entry and initialize the IO Interrupt
 * Assignment hardware.
 *
 * @param[in] pEntry The IO Interrupt Assignment entry to parse.
 */
static void _ParseIOInterruptAssignment(S_IOInterruptAssignment* pEntry);

/**
 * @brief Parses the MPTable Local Interrupt Assignment entry.
 *
 * @details Parses the MPTable Local Interrupt Assignment entry. The function
 * will parse the Local Interrupt Assignment entry and initialize the Local
 * Interrupt Assignment hardware.
 *
 * @param[in] pEntry The Local Interrupt Assignment entry to parse.
 */
static void _ParseLocalInterruptAssignment(S_LocalInterruptAssignment* pEntry);

/**
 * @brief Attaches the MPTable driver to the system.
 *
 * @details Attaches the MPTable driver to the system. This function will parse
 * the MPTable tables and initialize the MPTable hardware.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief MPTable driver instance. */
static S_LayerDriver sX86MPTableDriver =
{
  .pName         = "X86 MPTable Driver",
  .pDescription  = "X86 MPTable Driver for roOs",
  .pVersion      = "1.0",
  .layer         = 0,
  .pDriverAttach = _Attach
};


/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static S_MPTablePointer* _FindMPTablePointer(uint8_t*     startAddress,
                                             const size_t kSize)
{
  S_MPTablePointer* pMPTablePointer = NULL;
  uint8_t*          pSearchStart    = NULL;
  uint8_t*          pSearchEnd      = NULL;

  pSearchStart    = startAddress;
  pSearchEnd      = pSearchStart + kSize;
  pMPTablePointer = NULL;
  for (uint8_t* pPtr = pSearchStart; pPtr < pSearchEnd; pPtr += 16)
  {
    if (*(uint32_t*)pPtr == *(uint32_t*)MP_TABLE_POINTER_SIGNATURE)
    {
      pMPTablePointer = (S_MPTablePointer*)pPtr;
      break;
    }
  }

  return pMPTablePointer;
}

static void _ParseMPTable(S_MPTablePointer* pMPTablePointer)
{
  uintptr_t        mpTableAddress;
  uintptr_t        alignedAddress;
  uintptr_t        offset;
  size_t           alignedSize;
  E_Return         retCode;
  size_t           actualSize;
  S_MPTableHeader* pMPTableHeader;

  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Parsing MPTable Pointer at: 0x%p",
               pMPTablePointer);

  /* Get the table header */
  mpTableAddress = (uintptr_t)pMPTablePointer->mpTablePointerAddress;
  alignedAddress = ALIGN_DOWN(mpTableAddress, KERNEL_PAGE_SIZE);
  offset         = mpTableAddress & PAGE_SIZE_MASK;
  alignedSize    = sizeof(S_MPTableHeader) + offset;
  alignedSize    = ALIGN_UP(alignedSize, KERNEL_PAGE_SIZE);

  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Mapping MPTable Header at: 0x%p with size: %zu",
               alignedAddress,
               alignedSize);

  pMPTableHeader = MemoryKernelMap((void*)alignedAddress,
                                   alignedSize,
                                   MEMMGR_MAP_HARDWARE |
                                   MEMMGR_MAP_KERNEL   |
                                   MEMMGR_MAP_RO,
                                   &retCode);
  MPTABLE_ASSERT(retCode == NO_ERROR,
                 "Failed to map MPTable header",
                 retCode);
  pMPTableHeader = (S_MPTableHeader*)((uintptr_t)pMPTableHeader + offset);

  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "MPTable Header at: 0x%p with signature: %c%c%c%c",
               pMPTableHeader,
               pMPTableHeader->pSignature[0],
               pMPTableHeader->pSignature[1],
               pMPTableHeader->pSignature[2],
               pMPTableHeader->pSignature[3]);

  MPTABLE_ASSERT(*(uint32_t*)pMPTableHeader->pSignature ==
                 *(uint32_t*)MP_TABLE_SIGNATURE,
                 "Invalid MPTable signature",
                 ERR_INVALID_VALUE);

  actualSize = pMPTableHeader->length + offset;
  actualSize = ALIGN_UP(actualSize, KERNEL_PAGE_SIZE);

  /* Unmap the table header */
  pMPTableHeader = (S_MPTableHeader*)((uintptr_t)pMPTableHeader - offset);
  retCode = MemoryKernelUnmap(pMPTableHeader, alignedSize);
  MPTABLE_ASSERT(retCode == NO_ERROR,
                 "Failed to unmap MPTable header",
                 retCode);

  /* Map the table */
  pMPTableHeader = MemoryKernelMap((void*)alignedAddress,
                                   actualSize,
                                   MEMMGR_MAP_HARDWARE |
                                   MEMMGR_MAP_KERNEL   |
                                   MEMMGR_MAP_RO,
                                   &retCode);
  MPTABLE_ASSERT(retCode == NO_ERROR,
                 "Failed to map MPTable",
                 retCode);
  pMPTableHeader = (S_MPTableHeader*)((uintptr_t)pMPTableHeader + offset);

  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "Parsing MPTable at: 0x%p",
               pMPTableHeader);

  _ParseMPTableEntries(pMPTableHeader);

  /* Unmap the table */
  pMPTableHeader = (S_MPTableHeader*)((uintptr_t)pMPTableHeader - offset);
  retCode = MemoryKernelUnmap(pMPTableHeader, actualSize);
  MPTABLE_ASSERT(retCode == NO_ERROR,
                 "Failed to unmap MPTable",
                 retCode);
}

static void _ParseMPTableEntries(S_MPTableHeader* pMPTableHeader)
{
  uint8_t* pEntry;
  uint8_t  entryType;
  size_t   remainingEntries;

  _ValidateChecksum(pMPTableHeader);

  /* Parse the MPTable entries */
  pEntry           = (uint8_t*)pMPTableHeader + sizeof(S_MPTableHeader);
  remainingEntries = pMPTableHeader->length - sizeof(S_MPTableHeader);
  while (remainingEntries > 0)
  {
    entryType = *pEntry;
    switch (entryType)
    {
      case 0:
        _ParseProcessorEntry((S_ProcessorEntry*)pEntry);
        pEntry           += sizeof(S_ProcessorEntry);
        remainingEntries -= sizeof(S_ProcessorEntry);
        break;
      case 1:
        _ParseBusEntry((S_BusEntry*)pEntry);
        pEntry           += sizeof(S_BusEntry);
        remainingEntries -= sizeof(S_BusEntry);
        break;
      case 2:
        _ParseIOAPICEntry((S_IOAPICEntry*)pEntry);
        pEntry           += sizeof(S_IOAPICEntry);
        remainingEntries -= sizeof(S_IOAPICEntry);
        break;
      case 3:
        _ParseIOInterruptAssignment((S_IOInterruptAssignment*)pEntry);
        pEntry           += sizeof(S_IOInterruptAssignment);
        remainingEntries -= sizeof(S_IOInterruptAssignment);
        break;
      case 4:
        _ParseLocalInterruptAssignment((S_LocalInterruptAssignment*)pEntry);
        pEntry           += sizeof(S_LocalInterruptAssignment);
        remainingEntries -= sizeof(S_LocalInterruptAssignment);
        break;
      default:
        KERNEL_ERROR("Unknown MPTable entry type: %d. Skipping...", entryType);
    }
  }
}

static void _ValidateChecksum(S_MPTableHeader* pMPTableHeader)
{
  uint8_t sum = 0;
  for (size_t i = 0; i < pMPTableHeader->length; ++i)
  {
    sum += ((uint8_t*)pMPTableHeader)[i];
  }
  MPTABLE_ASSERT(sum == 0, "Invalid MPTable checksum", ERR_INVALID_VALUE);
}

static void _ParseProcessorEntry(S_ProcessorEntry* pEntry)
{
  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "    Processor Entry: Local APIC ID: %d, Local APIC Version: %d,"
               " Flags: 0x%02X, Signature: 0x%08X, Feature Flags: 0x%08X",
               pEntry->localApicId,
               pEntry->localApicVersion,
               pEntry->flags,
               pEntry->signature,
               pEntry->featureFlags);
}

static void _ParseBusEntry(S_BusEntry* pEntry)
{
  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "    Bus Entry: Bus ID: %d, Bus Type: %c%c%c%c%c%c",
               pEntry->busId,
               pEntry->pBusType[0],
               pEntry->pBusType[1],
               pEntry->pBusType[2],
               pEntry->pBusType[3],
               pEntry->pBusType[4],
               pEntry->pBusType[5]);
}

static void _ParseIOAPICEntry(S_IOAPICEntry* pEntry)
{
  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "    IO APIC Entry: IO APIC ID: %d, IO APIC Version: %d, "
               "Flags: 0x%02X, IO APIC Address: 0x%08X",
               pEntry->ioApicId,
               pEntry->ioApicVersion,
               pEntry->flags,
               pEntry->ioApicAddress);
}

static void _ParseIOInterruptAssignment(S_IOInterruptAssignment* pEntry)
{
  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "    IO Interrupt Assignment Entry: Interrupt Type: %d, "
               "Interrupt Flags: 0x%04X, Source Bus ID: %d, Source Bus IRQ: %d,"
               " Destination IO APIC ID: %d, Destination IO APIC INTIN: %d",
               pEntry->interruptType,
               pEntry->flags,
               pEntry->sourceBusId,
               pEntry->sourceBusIrq,
               pEntry->destIoApicId,
               pEntry->destIoApicIntin);
}

static void _ParseLocalInterruptAssignment(S_LocalInterruptAssignment* pEntry)
{
  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "    Local Interrupt Assignment Entry: Interrupt Type: %d, "
               "Interrupt Flags: 0x%04X, Source Bus ID: %d, Source Bus IRQ: %d,"
               " Destination LAPIC ID: %d, Destination LAPIC LINTIN: %d",
               pEntry->interruptType,
               pEntry->flags,
               pEntry->sourceBusId,
               pEntry->sourceBusIrq,
               pEntry->destLocalApicId,
               pEntry->destLocalApicLintin);
}

static E_Return _Attach(void)
{
  S_MPTablePointer* pMPTablePointer;
  void*             pMap;
  E_Return          retCode;
  uintptr_t         ebdaAddress;
  S_MPTablePointer  mpTablePointer;

  /* Map the low memory range */
  pMap = MemoryKernelMap(0,
                         1024 * KERNEL_PAGE_SIZE,
                         MEMMGR_MAP_HARDWARE |
                         MEMMGR_MAP_KERNEL   |
                         MEMMGR_MAP_RO,
                         &retCode);
  MPTABLE_ASSERT(retCode == NO_ERROR && pMap != NULL,
                 "Failed to map low memory range",
                 retCode);



  /* Search for the MPTable in the BIOS base memory */
  pMPTablePointer = _FindMPTablePointer((uint8_t*)pMap + BIOS_BASE_ADDRESS,
                                        BIOS_BASE_MEM_SIZE);
  if (pMPTablePointer == NULL)
  {
    pMPTablePointer = _FindMPTablePointer((uint8_t*)pMap + SYS_BASE_ADDRESS,
                                          SYS_BASE_MEM_SIZE);
    if (pMPTablePointer == NULL)
    {
      /* Search for the MPTable in the EBDA */
      ebdaAddress = *(uint16_t*)((uintptr_t)pMap + BIOS_EBDA_ADDRESS);
      ebdaAddress = ebdaAddress << 4;
      pMPTablePointer = _FindMPTablePointer((uint8_t*)ebdaAddress,
                                        BIOS_EBDA_MEM_SIZE);
      MPTABLE_ASSERT(pMPTablePointer != NULL,
                     "Failed to find MPTable pointer in EBDA",
                     ERR_NOT_FOUND);
    }
  }

  KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
               MODULE_NAME,
               "MPTable pointer found at: 0x%p",
               pMPTablePointer);

  memcpy(&mpTablePointer, pMPTablePointer, sizeof(S_MPTablePointer));

  /* Unmap the memory */
  retCode = MemoryKernelUnmap(pMap, 1024 * KERNEL_PAGE_SIZE);
  MPTABLE_ASSERT(retCode == NO_ERROR,
                 "Failed to unmap low memory range",
                 retCode);

  _ParseMPTable(&mpTablePointer);

  /* If needed set the IO APIC mode */
  if (mpTablePointer.featureBytes & IMCR_CHIP_PRESENT)
  {
    /* Enable IO APIC mode */
    CPUPortWriteByte(0x22, 0x70);
    CPUPortWriteByte(0x23, 0x01);

    KERNEL_DEBUG(MPTABLE_DRIVER_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IO APIC mode enabled");
  }

  return NO_ERROR;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_LAYER(sX86MPTableDriver);

/************************************ EOF *************************************/