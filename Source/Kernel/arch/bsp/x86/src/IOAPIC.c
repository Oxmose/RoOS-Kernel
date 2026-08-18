/*******************************************************************************
 * @file IOAPIC.c
 *
 * @see IOAPIC.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 05/06/2024
 *
 * @version 2.0
 *
 * @brief IO-APIC (IO advanced programmable interrupt controler) driver.
 *
 * @details IO-APIC (IO advanced programmable interrupt controler) driver.
 * Allows to remap the IO-APIC IRQ, set the IRQs mask and manage EoI for the
 * X86 IO-APIC.
 *
 * @warning This driver also use the LAPIC driver to function correctly.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <MMIO.h>
#include <ACPI.h>
#include <LAPIC.h>
#include <Panic.h>
#include <Memory.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <Critical.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <DeviceTree.h>
#include <Interrupts.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <IOAPIC.h>

/* Unit test header */
/* TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief FDT property for interrupt offset */
#define IOAPIC_FDT_INTOFF_PROP "int-offset"
/** @brief FDT property for acpi handle */
#define IOAPIC_FDT_ACPI_NODE_PROP "acpi-node"
/** @brief FDT property for lapic handle */
#define IOAPIC_FDT_LAPIC_NODE_PROP "lapic-node"
/** @brief FDT property for is-interrupt-driver */
#define IOAPIC_FDT_INT_DRIVER_PROP "interrupt-controller"

/** @brief IO-APIC register selection. */
#define IOREGSEL 0x00
/** @brief IO-APIC data access register. */
#define IOWIN 0x10

/** @brief IO-ACPI memory size */
#define IOAPIC_MEM_SIZE 0x10

/** @brief IO-APIC ID register. */
#define IOAPICID  0x00
/** @brief IO-APIC version register. */
#define IOAPICVER 0x01
/** @brief IO-APIC arbitration id register. */
#define IOAPICARB 0x02
/** @brief IO-APIC redirection register base . */
#define IOREDTBLBASE  0x10
/** @brief IO-APIC indexed redirection low register. */
#define IOREDTBLXL(IRQ) ((IRQ) * 2 + IOREDTBLBASE)
/** @brief IO-APIC indexed redirection high register. */
#define IOREDTBLXH(IRQ) ((IRQ) * 2 + 1 + IOREDTBLBASE)

/** @brief IOAPIC Version register version value mask */
#define IOAPIC_VERSION_MASK 0x000000FF
/** @brief IOAPIC Version register redirection value mask */
#define IOAPIC_REDIR_MASK 0x00FF0000
/** @brief IOAPIC Version register version value shift */
#define IOAPIC_VERSION_SHIFT 0
/** @brief IOAPIC Version register redirection value shift */
#define IOAPIC_REDIR_SHIFT 16

/** @brief Current module name */
#define MODULE_NAME "X86 IO-APIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 IO-APIC driver controler. */
typedef struct S_IOAPICControler
{
  /** @brief IO-APIC base physical address */
  uintptr_t baseAddr;
  /** @brief IO-APIC mapping size */
  size_t mappingSize;
  /** @brief IO-APIC identifier. */
  uint8_t identifier;
  /** @brief IO-APIC version. */
  uint8_t version;
  /** @brief First IRQ handled by the current IO-APIC. */
  uint32_t gsib;
  /** @brief Last IRQ handled by the current IO-APIC. */
  uint8_t gsil;
  /** @brief The controler lock to avoid concurrent accesses */
  S_KernelSpinlock lock;
  /** @brief On system with multiple IO-APICs link to the next */
  struct S_IOAPICControler* pNext;
} S_IOAPICControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the IO-APIC to ensure correctness of execution.
 *
 * @details Assert macro used by the IO-APIC to ensure correctness of execution.
 * Due to the critical nature of the IO-APIC, any error generates a kernel
 * panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define IOAPIC_ASSERT(COND, MSG, ERROR) {                 \
  if ((COND) == false)                                     \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the IO-APIC driver to the system.
 *
 * @details Attaches the IO-APIC driver to the system. This function will use
 * the FDT to initialize the IO-APIC hardware and retreive the IO-APIC
 * parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief Sets the IRQ mask for the desired IRQ number.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @param[in] kIRQNumber The IRQ number to enable/disable.
 * @param[in] kEnabled Must be set to true to enable the IRQ or false
 * to disable the IRQ.
 */
static void _SetIRQMask(const uint32_t kIRQNumber, const bool kEnabled);

/**
 * @brief Sets the IRQ mask for the desired IRQ number on a given controller.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter on a given
 * controller.
 *
 * @param[in, out] pCtrl The controller to use.
 * @param[in] kIRQNumber The IRQ number to enable/disable.
 * @param[in] kEnabled Must be set to true to enable the IRQ or false
 * to disable the IRQ.
 */
static inline void _SetIRQMaskFor (S_IOAPICControler* pCtrl,
                                  const uint32_t     kIRQNumber,
                                  const bool         kEnabled);

/**
 * @brief Checks if the serviced interrupt is a spurious
 * interrupt. The function also handles the spurious interrupt.
 *
 * @details Checks if the serviced interrupt is a spurious
 * interrupt. The function also handles the spurious interrupt.
 *
 * @param[in] kIntNumber The interrupt number of the interrupt to
 * test.
 *
 * @return The function will return the interrupt type.
 * - INTERRUPT_TYPE_SPURIOUS if the current interrupt is a spurious one.
 * - INTERRUPT_TYPE_REGULAR if the current interrupt is a regular one.
 */
static E_InterruptType _HandleSpurious(const uint32_t kIntNumber);

/**
 * @brief Returns the interrupt line attached to an IRQ.
 *
 * @details Returns the interrupt line attached to an IRQ. -1 is returned
 * if the IRQ number is not supported by the driver.
 *
 * @param[in] kIRQNumber The IRQ number of which to get the interrupt
 * line.
 *
 * @return The interrupt line attached to an IRQ. -1 is returned if the IRQ
 * number is not supported by the driver.
 */
static uint32_t _GetInterruptLine(const uint32_t kIRQNumber);

/**
 * @brief Reads into the IO APIC controller memory.
 *
 * @details Reads into the IO APIC controller memory.
 *
 * @param[in] pCtrl The controller to use.
 * @param[in] kRegister The register to read.
 *
 * @return The value contained in the register.
 */
static inline uint32_t _Read(S_IOAPICControler* pCtrl,
                             const uint32_t     kRegister);

/**
 * @brief Writes to the IO APIC controller memory.
 *
 * @details Writes to the IO APIC controller memory.
 *
 * @param[in] pCtrl The controller to use.
 * @param[in] kRegister The register to write.
 * @param[in] kVal The value to write to the register.
 */
static inline void _Write(S_IOAPICControler* pCtrl,
                          const uint32_t     kRegister,
                          const uint32_t     kVal);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PIC driver instance. */
static S_Driver sX86IOAPICDriver =
{
  .pName         = "X86 IO-APIC Driver",
  .pDescription  = "X86 IO-APIC Driver for roOs",
  .pCompatible   = "x86,x86-io-apic",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/** @brief IO-APIC interrupt driver instance. */
static S_InterruptDriver sIOAPICDriver =
{
  .pSetIRQMask          = _SetIRQMask,
  .pSetIRQEOI           = NULL,
  .pHandleSpurious      = _HandleSpurious,
  .pGetIRQInterruptLine = _GetInterruptLine
};

/** @brief IO-APIC driver controler instance */
static S_IOAPICControler* spDrvCtrl = NULL;

/** @brief IOAPIC ACPI driver handle */
static const S_ACPIDriver* skpACPIDriver;

/** @brief IRQ interrupt offset */
static uint8_t sIntOffset;

/** @brief CPU's spurious interrupt line */
static uint32_t sSpuriousIntLine;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t*     kpACPINodeProp;
  const uint32_t*     kpLAPICNodeProp;
  const uint32_t*     kpUintProp;
  S_IOAPICControler*  pNewDrvCtrl;
  S_LAPICDriver*      pLAPICDriver;
  const S_IOAPICNode* kpIOAPICNode;
  E_Return            retCode;
  size_t              propLen;
  size_t              acpiPropLen;
  size_t              lapicPropLen;
  uint8_t             i;
  uint32_t            ioapicVerRegister;
  uintptr_t           ioApicPhysAddr;
  size_t              toMap;

  retCode       = NO_ERROR;
  sIntOffset    = 0xFF;
  pLAPICDriver  = NULL;
  skpACPIDriver = NULL;

  /* Get CPU's spurious interrupt line */
  sSpuriousIntLine = CPUGetInterruptConfig()->spuriousInterruptLine;

  /* Get IRQ offset */
  kpUintProp = FDTGetProp(pkFdtNode, IOAPIC_FDT_INTOFF_PROP, &propLen);
  if (kpUintProp != NULL && propLen == sizeof(uint32_t))
  {
    sIntOffset = (uint8_t)FDTTOCPU32(*kpUintProp);
  }

  /* Get the handles */
  kpACPINodeProp = FDTGetProp(pkFdtNode,
                              IOAPIC_FDT_ACPI_NODE_PROP,
                              &acpiPropLen);
  kpLAPICNodeProp = FDTGetProp(pkFdtNode,
                               IOAPIC_FDT_LAPIC_NODE_PROP,
                               &lapicPropLen);

  if (sIntOffset != 0xFF &&
      kpACPINodeProp != NULL && acpiPropLen == sizeof(uint32_t) &&
      kpLAPICNodeProp != NULL && lapicPropLen == sizeof(uint32_t))
  {
    /* Get the drivers */
    skpACPIDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpACPINodeProp));
    pLAPICDriver = DriverManagerGetDeviceData(FDTTOCPU32(*kpLAPICNodeProp));

    if (skpACPIDriver != NULL && pLAPICDriver != NULL)
    {
      /* Set IRQ EOI for delegated by the LAPIC */
      sIOAPICDriver.pSetIRQEOI = pLAPICDriver->pSetIRQEOI;

      /* Get the IOAPICs */
      kpIOAPICNode = skpACPIDriver->pGetIOAPICList();
      while (kpIOAPICNode != NULL)
      {
        pNewDrvCtrl = KMalloc(sizeof(S_IOAPICControler),
                              ALIGN_ADDRESS,
                              KMALLOC_FREE_POOL);
        memset(pNewDrvCtrl, 0, sizeof(S_IOAPICControler));
        /* Link IO APIC controller */
        pNewDrvCtrl->pNext = spDrvCtrl;
        spDrvCtrl = pNewDrvCtrl;

        KERNEL_SPINLOCK_INIT(pNewDrvCtrl->lock);

        /* Map the IO APIC */
        ioApicPhysAddr = kpIOAPICNode->ioApic.ioApicAddr & ~PAGE_SIZE_MASK;
        toMap = IOAPIC_MEM_SIZE + (ioApicPhysAddr & PAGE_SIZE_MASK);
        toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

        pNewDrvCtrl->baseAddr = (uintptr_t)MemoryKernelMap(
                                                (void*)ioApicPhysAddr,
                                                toMap,
                                                MEMMGR_MAP_HARDWARE |
                                                MEMMGR_MAP_KERNEL   |
                                                MEMMGR_MAP_RW,
                                                &retCode);
        if (retCode == NO_ERROR)
        {
          pNewDrvCtrl->baseAddr |= kpIOAPICNode->ioApic.ioApicAddr &
                                    PAGE_SIZE_MASK;
          pNewDrvCtrl->mappingSize = toMap;

          /* Setup the controller */
          pNewDrvCtrl->identifier = kpIOAPICNode->ioApic.ioApicId;
          pNewDrvCtrl->gsib = kpIOAPICNode->ioApic.globalSystemInterruptBase;

          /* Get the version and max IRQ number */
          ioapicVerRegister = _Read(pNewDrvCtrl, IOAPICVER);
          pNewDrvCtrl->version = (ioapicVerRegister & IOAPIC_VERSION_MASK) >>
                                  IOAPIC_VERSION_SHIFT;
          pNewDrvCtrl->gsil = pNewDrvCtrl->gsil + 1 +
              ((ioapicVerRegister & IOAPIC_REDIR_MASK) >> IOAPIC_REDIR_SHIFT);

          /* Disable all IRQ for this  IOAPIC */
          for (i = pNewDrvCtrl->gsib; i < pNewDrvCtrl->gsil; ++i)
          {
            _SetIRQMaskFor(pNewDrvCtrl, i, false);
          }

          /* Go to next */
          kpIOAPICNode = kpIOAPICNode->pNext;
        }
        else
        {
          while (spDrvCtrl != NULL)
          {
            pNewDrvCtrl = spDrvCtrl->pNext;
            if (spDrvCtrl->baseAddr != (uintptr_t)NULL)
            {
              retCode = MemoryKernelUnmap((void*)spDrvCtrl->baseAddr,
                                          spDrvCtrl->mappingSize);
              IOAPIC_ASSERT(retCode == NO_ERROR,
                            "Failed to unmap memory",
                            ERR_UNAUTHORIZED_ACTION);
            }
            KFree(spDrvCtrl);
            spDrvCtrl = pNewDrvCtrl;
          }
          break;

          retCode = ERR_NO_MEMORY;
        }
      }

      /* Register if needed */
      if (retCode == NO_ERROR &&
          FDTGetProp(pkFdtNode, IOAPIC_FDT_INT_DRIVER_PROP, &propLen) != NULL)
      {
        /* Register as interrupt controler */
        retCode = InterruptSetDriver(&sIOAPICDriver);
        IOAPIC_ASSERT(retCode == NO_ERROR,
                      "Failed to register IO-APIC in interrupt manager",
                      retCode);
      }
    }
    else
    {
      retCode = ERR_INVALID_VALUE;
    }
  }
  else
  {
    retCode = ERR_INVALID_VALUE;
  }

  /* IO APIC is mandatory */
  IOAPIC_ASSERT(retCode == NO_ERROR, "Failed to init IOAPIC", retCode);

  return retCode;
}

static void _SetIRQMask(const uint32_t kIRQNumber, const bool kEnabled)
{
  uint32_t           remapIRQ;
  S_IOAPICControler* pCtrl;

  /* Get the remapped IRQ */
  remapIRQ = skpACPIDriver->pGetRemapedIRQ(kIRQNumber);

  /* Search for the correct IO APIC controller */
  pCtrl = spDrvCtrl;
  while (pCtrl != NULL)
  {
    if (pCtrl->gsib <= remapIRQ && pCtrl->gsil > remapIRQ)
    {
      break;
    }
    pCtrl = pCtrl->pNext;
  }

  IOAPIC_ASSERT(pCtrl != NULL, "No such IRQ", ERR_INVALID_PARAMETER);

  _SetIRQMaskFor(pCtrl, remapIRQ, kEnabled);
}

static inline void _SetIRQMaskFor(S_IOAPICControler* pCtrl,
                                  const uint32_t     kIRQNumber,
                                  const bool         kEnabled)
{
  uint32_t entryLow;
  uint32_t remapIRQ;

  IOAPIC_ASSERT(kIRQNumber >= pCtrl->gsib && kIRQNumber < pCtrl->gsil,
                "No such IRQ for current IOAPIC",
                ERR_INVALID_PARAMETER);

  /* Update the IRQ for the table */
  remapIRQ = kIRQNumber - pCtrl->gsib;

  /* Set the mask, IO APIC uses physical destination only to core 0 */
  entryLow = _GetInterruptLine(kIRQNumber);
  if (kEnabled == false)
  {
    entryLow  |= (1 << 16);
  }
  else
  {
    entryLow  &= ~(1 << 16);
  }

  _Write(pCtrl, IOREDTBLXL(remapIRQ), entryLow);
}

static E_InterruptType _HandleSpurious(const uint32_t kIntNumber)
{
  E_InterruptType intType;

  intType = INTERRUPT_TYPE_REGULAR;

  /* Check for LAPIC spurious interrupt. */
  if (kIntNumber == sSpuriousIntLine)
  {
    sIOAPICDriver.pSetIRQEOI(kIntNumber);
    intType = INTERRUPT_TYPE_SPURIOUS;
  }

  return intType;
}

static uint32_t _GetInterruptLine(const uint32_t kIRQNumber)
{
  uint32_t remapIRQ;

  remapIRQ = skpACPIDriver->pGetRemapedIRQ(kIRQNumber);

  return sIntOffset + remapIRQ;
}

static inline uint32_t _Read(S_IOAPICControler* pCtrl,
                             const uint32_t     kRegister)
{
  uint32_t value;

  KERNEL_LOCK(pCtrl->lock);

  /* Set IOREGSEL */
  _MMIOWrite32((void*)(pCtrl->baseAddr + IOREGSEL), kRegister);

  /* Get register value */
  value = _MMIORead32((void*)(pCtrl->baseAddr + IOWIN));

  KERNEL_UNLOCK(pCtrl->lock);

  return value;
}

static inline void _Write(S_IOAPICControler* pCtrl,
                          const uint32_t     kRegister,
                          const uint32_t     kVal)
{
  KERNEL_LOCK(pCtrl->lock);

  /* Set IOREGSEL */
  _MMIOWrite32((void*)(pCtrl->baseAddr + IOREGSEL), kRegister);

  /* Set register value */
  _MMIOWrite32((void*)(pCtrl->baseAddr + IOWIN), kVal);

  KERNEL_UNLOCK(pCtrl->lock);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86IOAPICDriver);

/************************************ EOF *************************************/