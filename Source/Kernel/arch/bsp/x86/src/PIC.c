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
 * @brief PIC (programmable interrupt controller) driver.
 *
 * @details PIC (programmable interrupt controller) driver. Allows to remmap
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
#include <KernelError.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <PIC.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
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
#define PIC0_BASE_INTERRUPT_LINE (0)
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

/** @brief PIC's master command ports. */
#define PIC_MASTER_COMM_PORT 0x20
/** @brief PIC's master data ports. */
#define PIC_MASTER_DATA_PORT 0x21
/** @brief PIC's slave command ports. */
#define PIC_SLAVE_COMM_PORT  0xA0
/** @brief PIC's slave data ports. */
#define PIC_SLAVE_DATA_PORT  0xA1

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

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
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the PIC driver to the system.
 *
 * @details Attaches the PIC driver to the system. This function will
 * initialize the PIC hardware and retreive the PIC parameters.
 *
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
/** @brief PIC driver instance. */
static S_LayerDriver  sX86PICDriver =
{
  .pName         = "X86 PIC Driver",
  .pDescription  = "X86 Programable Interrupt Controller Driver for roOs",
  .pVersion      = "2.0",
  .layer         = 0,
  .pDriverAttach = _Attach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(void)
{
  /* Initialize the master, remap IRQs */
  CPUPortWriteByte(PIC_ICW1_ICW4 | PIC_ICW1_INIT, PIC_MASTER_COMM_PORT);
  CPUPortWriteByte(PIC0_BASE_INTERRUPT_LINE, PIC_MASTER_DATA_PORT);
  CPUPortWriteByte(0x4, PIC_MASTER_DATA_PORT);
  CPUPortWriteByte(PIC_ICW4_8086, PIC_MASTER_DATA_PORT);
  /* Set EOI */
  CPUPortWriteByte(PIC_EOI, PIC_MASTER_COMM_PORT);
  /* Disable all IRQs */
  CPUPortWriteByte(0xFF, PIC_MASTER_DATA_PORT);

  /* Initialize the slave, remap IRQs */
  CPUPortWriteByte(PIC_ICW1_ICW4 | PIC_ICW1_INIT, PIC_SLAVE_COMM_PORT);
  CPUPortWriteByte(PIC1_BASE_INTERRUPT_LINE, PIC_SLAVE_DATA_PORT);
  CPUPortWriteByte(0x2,  PIC_SLAVE_DATA_PORT);
  CPUPortWriteByte(PIC_ICW4_8086,  PIC_SLAVE_DATA_PORT);
  /* Set EOI */
  CPUPortWriteByte(PIC_EOI, PIC_SLAVE_COMM_PORT);
  /* Disable all IRQs */
  CPUPortWriteByte(0xFF, PIC_SLAVE_DATA_PORT);

  return NO_ERROR;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_LAYER(sX86PICDriver);

/************************************ EOF *************************************/