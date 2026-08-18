/*******************************************************************************
 * @file pic.c
 *
 * @see pic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 2.0
 *
 * @brief PIC (programmable interrupt controler) driver.
 *
 * @details PIC (programmable interrupt controler) driver. Allows to remmap
 * the PIC IRQ, set the IRQs mask and manage EoI for the X86 PIC.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <Panic.h>
#include <X64Cpu.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <Critical.h>
#include <Interrupts.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <PIC.h>

/* Unit test header */
/* TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief FDT property for chaining  */
#define PIC_FDT_HASSLAVE_PROP "is-chained"
/** @brief FDT property for interrupt offset */
#define PIC_FDT_INTOFF_PROP "int-offset"
/** @brief FDT property for comm ports */
#define PIC_FDT_COMM_PROP "comm"
/** @brief FDT property for is-interrupt-driver */
#define PIC_FDT_IS_INT_DRIVER_PROP "interrupt-controller"

/** @brief PIC End of Interrupt command. */
#define PIC_EOI 0x20

/** @brief PIC ICW4 needed flag. */
#define PIC_ICW1_ICW4      0x01
/** @brief PIC single mode flag. */
#define PIC_ICW1_SINGLE    0x02
/** @brief PIC call address interval 4 flag. */
#define PIC_ICW1_INTERVAL4 0x04
/** @brief PIC trigger level flag. */
#define PIC_ICW1_LEVEL     0x08
/** @brief PIC initialization flag. */
#define PIC_ICW1_INIT      0x10

/** @brief PIC 8086/88 (MCS-80/85) mode flag. */
#define PIC_ICW4_8086        0x01
/** @brief PIC auto (normal) EOI flag. */
#define PIC_ICW4_AUTO        0x02
/** @brief PIC buffered mode/slave flag. */
#define PIC_ICW4_BUF_SLAVE   0x08
/** @brief PIC buffered mode/master flag. */
#define PIC_ICW4_BUF_MASTER  0x0C
/** @brief PIC special fully nested (not) flag. */
#define PIC_ICW4_SFNM        0x10

/** @brief Read ISR command value */
#define PIC_READ_ISR 0x0B

/** @brief Master PIC Base interrupt line for the lowest IRQ. */
#define PIC0_BASE_INTERRUPT_LINE (sDrvCtrl.intOffset)
/** @brief Slave PIC Base interrupt line for the lowest IRQ. */
#define PIC1_BASE_INTERRUPT_LINE (PIC0_BASE_INTERRUPT_LINE + 8)

/** @brief PIC's cascading IRQ number. */
#define PIC_CASCADING_IRQ 2

/** @brief The PIC spurious irq mask. */
#define PIC_SPURIOUS_IRQ_MASK 0x80

/** @brief Master PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_MASTER 0x07
/** @brief Slave PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_SLAVE  0x0F

/** @brief PIC's minimal IRQ number. */
#define PIC_MIN_IRQ_LINE 0
/** @brief PIC's maximal IRQ number. */
#define PIC_MAX_IRQ_LINE 15

/** @brief Current module name */
#define MODULE_NAME "X86 PIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief x86 PIC driver controler. */
typedef struct
{
  /** @brief CPU command port. */
  uint16_t cpuMasterCommPort;
  /** @brief CPU data port. */
  uint16_t cpuMasterDataPort;
  /** @brief CPU command port. */
  uint16_t cpuSlaveCommPort;
  /** @brief CPU data port. */
  uint16_t cpuSlaveDataPort;
  /** @brief Tells if the PIC has a slave */
  bool hasSlave;
  /** @brief PIC IRQ interrupt offset */
  uint8_t intOffset;
  /** @brief Controler lock for concurrent accesses */
  S_KernelSpinlock lock;
} S_PICControler;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the PIC to ensure correctness of execution.
 *
 * @details Assert macro used by the PIC to ensure correctness of execution.
 * Due to the critical nature of the PIC, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define PIC_ASSERT(COND, MSG, ERROR) {                    \
  if ((COND) == false)                                     \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the PIC driver to the system.
 *
 * @details Attaches the PIC driver to the system. This function will use the
 * FDT to initialize the PIC hardware and retreive the PIC parameters.
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
 * @brief Acknowleges an IRQ.
 *
 * @details Acknowlege an IRQ by setting the End Of Interrupt bit for this IRQ.
 *
 * @param[in] kIRQNumber The irq number to Acknowlege.
 */
static void _SetIRQEOI(const uint32_t kIRQNumber);

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

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PIC driver instance. */
static S_Driver sX86PICDriver =
{
  .pName         = "X86 PIC Driver",
  .pDescription  = "X86 Programable Interrupt Controler Driver for roOs",
  .pCompatible   = "x86,x86-pic",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/** @brief PIC interrupt driver instance. */
static S_InterruptDriver sPicDriver =
{
  .pSetIRQMask          = _SetIRQMask,
  .pSetIRQEOI           = _SetIRQEOI,
  .pHandleSpurious      = _HandleSpurious,
  .pGetIRQInterruptLine = _GetInterruptLine
};

/** @brief PIC driver controler instance */
static S_PICControler sDrvCtrl =
{
  .cpuMasterCommPort = 0,
  .cpuMasterDataPort = 0,
  .cpuSlaveCommPort  = 0,
  .cpuSlaveDataPort  = 0,
  .hasSlave          = false,
  .intOffset         = 0
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uint32_t* kpUintProp;
  size_t          propLen;
  E_Return        retCode;

  /* Init spinlock */
  KERNEL_SPINLOCK_INIT(sDrvCtrl.lock);

  /* Check for slave */
  if (FDTGetProp(pkFdtNode, PIC_FDT_HASSLAVE_PROP, &propLen) != NULL)
  {
    sDrvCtrl.hasSlave = true;
  }

  /* Get IRQ offset */
  kpUintProp = FDTGetProp(pkFdtNode, PIC_FDT_INTOFF_PROP, &propLen);
  PIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
             "Invlaid PIC FDT property.",
             ERR_INVALID_VALUE);

  sDrvCtrl.intOffset = (uint8_t)FDTTOCPU32(*kpUintProp);

  /* Get the com ports */
  kpUintProp = FDTGetProp(pkFdtNode, PIC_FDT_COMM_PROP, &propLen);
  if (sDrvCtrl.hasSlave == true)
  {
    PIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t) * 4,
               "Invlaid PIC FDT property.",
               ERR_INVALID_VALUE);

    sDrvCtrl.cpuMasterCommPort = (uint8_t)FDTTOCPU32(*kpUintProp);
    sDrvCtrl.cpuMasterDataPort = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));
    sDrvCtrl.cpuSlaveCommPort  = (uint8_t)FDTTOCPU32(*(kpUintProp + 2));
    sDrvCtrl.cpuSlaveDataPort  = (uint8_t)FDTTOCPU32(*(kpUintProp + 3));
  }
  else
  {
    PIC_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t) * 2,
               "Invlaid PIC FDT property.",
               ERR_INVALID_VALUE);

    sDrvCtrl.cpuMasterCommPort = (uint8_t)FDTTOCPU32(*kpUintProp);
    sDrvCtrl.cpuMasterDataPort = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));
  }

  /* Initialize the master, remap IRQs */
  CPUPortWriteByte(PIC_ICW1_ICW4 | PIC_ICW1_INIT, sDrvCtrl.cpuMasterCommPort);
  CPUPortWriteByte(PIC0_BASE_INTERRUPT_LINE, sDrvCtrl.cpuMasterDataPort);
  CPUPortWriteByte(0x4, sDrvCtrl.cpuMasterDataPort);
  CPUPortWriteByte(PIC_ICW4_8086, sDrvCtrl.cpuMasterDataPort);
  /* Set EOI */
  CPUPortWriteByte(PIC_EOI, sDrvCtrl.cpuMasterCommPort);
  /* Disable all IRQs */
  CPUPortWriteByte(0xFF, sDrvCtrl.cpuMasterDataPort);

  if (sDrvCtrl.hasSlave == true)
  {
    /* Initialize the slave, remap IRQs */
    CPUPortWriteByte(PIC_ICW1_ICW4 | PIC_ICW1_INIT, sDrvCtrl.cpuSlaveCommPort);
    CPUPortWriteByte(PIC1_BASE_INTERRUPT_LINE, sDrvCtrl.cpuSlaveDataPort);
    CPUPortWriteByte(0x2,  sDrvCtrl.cpuSlaveDataPort);
    CPUPortWriteByte(PIC_ICW4_8086,  sDrvCtrl.cpuSlaveDataPort);
    /* Set EOI */
    CPUPortWriteByte(PIC_EOI, sDrvCtrl.cpuSlaveCommPort);
    /* Disable all IRQs */
    CPUPortWriteByte(0xFF, sDrvCtrl.cpuSlaveDataPort);
  }

  /* Register if needed */
  if (FDTGetProp(pkFdtNode, PIC_FDT_IS_INT_DRIVER_PROP, &propLen) != NULL)
  {
    /* Register as interrupt controler */
    retCode = InterruptSetDriver(&sPicDriver);
    PIC_ASSERT(retCode == NO_ERROR,
               "Could not register PIC in interrupt manager",
               retCode);
  }

  return NO_ERROR;
}

static void _SetIRQMask(const uint32_t kIRQNumber, const bool kEnabled)
{
  uint8_t  initMask;
  uint32_t cascadingNumber;

  PIC_ASSERT(kIRQNumber <= PIC_MAX_IRQ_LINE,
             "Could not find PIC IRQ",
             ERR_INVALID_VALUE);

  KERNEL_LOCK(sDrvCtrl.lock);

  /* Manage master PIC */
  if (kIRQNumber < 8)
  {
    /* Retrieve initial mask */
    initMask = CPUPortReadByte(sDrvCtrl.cpuMasterDataPort);

    /* Set new mask value */
    if (kEnabled == false)
    {
        initMask |= 1 << kIRQNumber;
    }
    else
    {
        initMask &= ~(1 << kIRQNumber);
    }

    /* Set new mask */
    CPUPortWriteByte(initMask, sDrvCtrl.cpuMasterDataPort);
  }

  /* Manage slave PIC. WARNING, cascading will be enabled */
  if (kIRQNumber > 7)
  {
    PIC_ASSERT(sDrvCtrl.hasSlave == true,
               "Could not find PIC IRQ (chained)",
               ERR_INVALID_VALUE);

    /* Set new IRQ number */
    cascadingNumber = kIRQNumber - 8;

    /* Retrieve initial mask */
    initMask = CPUPortReadByte(sDrvCtrl.cpuMasterDataPort);

    /* Set new mask value */
    initMask &= ~(1 << PIC_CASCADING_IRQ);

    /* Set new mask */
    CPUPortWriteByte(initMask, sDrvCtrl.cpuMasterDataPort);

    /* Retrieve initial mask */
    initMask = CPUPortReadByte(sDrvCtrl.cpuSlaveDataPort);

    /* Set new mask value */
    if (kEnabled == false)
    {
      initMask |= 1 << cascadingNumber;
    }
    else
    {
      initMask &= ~(1 << cascadingNumber);
    }

    /* Set new mask */
    CPUPortWriteByte(initMask, sDrvCtrl.cpuSlaveDataPort);

    /* If all is masked then disable cascading */
    if (initMask == 0xFF)
    {
      /* Retrieve initial mask */
      initMask = CPUPortReadByte(sDrvCtrl.cpuMasterDataPort);

      /* Set new mask value */
      initMask  |= 1 << PIC_CASCADING_IRQ;

      /* Set new mask */
      CPUPortWriteByte(initMask, sDrvCtrl.cpuMasterDataPort);
    }
  }

  KERNEL_UNLOCK(sDrvCtrl.lock);
}

static void _SetIRQEOI(const uint32_t kIRQNumber)
{
  KERNEL_LOCK(sDrvCtrl.lock);

  PIC_ASSERT(kIRQNumber <= PIC_MAX_IRQ_LINE,
             "Could not find PIC IRQ",
             ERR_INVALID_VALUE);

  /* End of interrupt signal */
  if (kIRQNumber > 7)
  {
    PIC_ASSERT(sDrvCtrl.hasSlave == true,
               "Could not find PIC IRQ (chained)",
               ERR_INVALID_VALUE);

    CPUPortWriteByte(PIC_EOI, sDrvCtrl.cpuSlaveCommPort);
  }
  CPUPortWriteByte(PIC_EOI, sDrvCtrl.cpuMasterCommPort);

  KERNEL_UNLOCK(sDrvCtrl.lock);
}

static E_InterruptType _HandleSpurious(const uint32_t kIntNumber)
{
  uint8_t         isrVal;
  uint32_t        irqNumber;
  E_InterruptType type;

  KERNEL_LOCK(sDrvCtrl.lock);

  irqNumber = kIntNumber - PIC0_BASE_INTERRUPT_LINE;

  /* Check if regular soft interrupt */
  if (irqNumber <= PIC_MAX_IRQ_LINE)
  {
    /* Check the PIC type */
    if (irqNumber > 7)
    {
      PIC_ASSERT(sDrvCtrl.hasSlave == true,
                  "Could not find spurious PIC IRQ (chained)",
                  ERR_INVALID_VALUE);

      /* This is not a potential spurious irq */
      if (irqNumber == PIC_SPURIOUS_IRQ_SLAVE)
      {
        /* Read the ISR mask */
        CPUPortWriteByte(PIC_READ_ISR, sDrvCtrl.cpuSlaveCommPort);
        isrVal = CPUPortReadByte(sDrvCtrl.cpuSlaveCommPort);
        if ((isrVal & PIC_SPURIOUS_IRQ_MASK) != 0)
        {
          type = INTERRUPT_TYPE_REGULAR;
        }
        else
        {
          /* Send EOI on master */
          _SetIRQEOI(PIC_CASCADING_IRQ);
          type = INTERRUPT_TYPE_SPURIOUS;
        }
      }
      else
      {
        type = INTERRUPT_TYPE_REGULAR;
      }
    }
    else
    {
      /* This is not a potential spurious irq */
      if (irqNumber == PIC_SPURIOUS_IRQ_MASTER)
      {
        /* Read the ISR mask */
        CPUPortWriteByte(PIC_READ_ISR, sDrvCtrl.cpuMasterCommPort);
        isrVal = CPUPortReadByte(sDrvCtrl.cpuMasterCommPort);
        if ((isrVal & PIC_SPURIOUS_IRQ_MASK) != 0)
        {
          type = INTERRUPT_TYPE_REGULAR;
        }
        else
        {
          type = INTERRUPT_TYPE_SPURIOUS;
        }
      }
      else
      {
        type = INTERRUPT_TYPE_REGULAR;
      }
    }
  }
  else
  {
    type = INTERRUPT_TYPE_REGULAR;
  }

  KERNEL_UNLOCK(sDrvCtrl.lock);

  return type;
}

static uint32_t _GetInterruptLine(const uint32_t kIRQNumber)
{
  if (kIRQNumber > PIC_MAX_IRQ_LINE)
  {
    return 0xFFFFFFFF;
  }
  return kIRQNumber + PIC0_BASE_INTERRUPT_LINE;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86PICDriver);

/************************************ EOF *************************************/