/*******************************************************************************
 * @file Interrupts.c
 *
 * @see Interrupts.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
 *
 * @version 3.0
 *
 * @brief Interrupt manager.
 *
 * @details Interrupt manager. Allows to attach ISR to interrupt lines and
 * manage IRQ used by the CPU. We also define the general interrupt handler
 * here.
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
#include <stdbool.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <Scheduler.h>
#include <KernelHeap.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <Interrupts.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module's name */
#define MODULE_NAME "INTERRUPTS"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define INTERRUPT_ASSERT(COND, MSG, ERROR) {              \
  if ((COND) == false)                                    \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false);                \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initial placeholder for the IRQ mask driver.
 *
 * @param kIRQNumber Unused.
 * @param enabled Unused.
 */
static void _InitDriverSetIRQMask(const uint32_t kIRQNumber,
                                  const bool     kEnabled);

/**
 * @brief Initial placeholder for the IRQ EOI driver.
 *
 * @param kIRQNumber Unused.
 */
static void _InitDriverSetIRQEOI(const uint32_t kIRQNumber);

/**
 * @brief Initial placeholder for the spurious handler driver.
 *
 * @param kIntNumber Unused.
 */
static E_InterruptType _InitDriverHandleSpurious(const uint32_t kIntNumber);

/**
 * @brief Initial placeholder for the get int line driver.
 *
 * @param kIRQNumber Unused.
 */
static uint32_t _InitDriverGetIRQIntLine(const uint32_t kIRQNumber);

/**
 * @brief Kernel's spurious interrupt handler.
 *
 * @details Spurious interrupt handler. This function should only be
 * called by an assembly interrupt handler. The function will handle spurious
 * interrupts.
 */
static void _SpuriousHandler(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the handlers for each interrupt. */
static T_InterruptHandler* spHandlerTable;

/** @brief The current interrupt driver to be used by the kernel. */
static S_InterruptDriver sInterruptDriver;

/** @brief Stores the CPU's interrupt configuration. */
static const S_CPUInterruptConfiguration* kspCpuInterruptConfig;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _InitDriverSetIRQMask(const uint32_t kIRQNumber,
                                  const bool     kEnabled)
{
  (void)kIRQNumber;
  (void)kEnabled;
}

static void _InitDriverSetIRQEOI(const uint32_t kIRQNumber)
{
  (void)kIRQNumber;
}

static E_InterruptType _InitDriverHandleSpurious(const uint32_t kIntNumber)
{
  (void)kIntNumber;
  return INTERRUPT_TYPE_REGULAR;
}

static uint32_t _InitDriverGetIRQIntLine(const uint32_t kIRQNumber)
{
  (void)kIRQNumber;
  return 0;
}

static void _SpuriousHandler(void)
{
  InterruptSetEOI(kspCpuInterruptConfig->spuriousInterruptLine);
}

void InterruptMainHandler(void)
{
  T_InterruptHandler handler;
  S_KernelThread*    pCurrentThread;
  uint32_t           intId;
  bool               schedule;

  /* Get the current thread */
  pCurrentThread = SchedulerGetCurrentThread();
  intId          = CPUGetContextInterruptNumber(pCurrentThread);

  /* Check for spurious interrupt */
  if (sInterruptDriver.pHandleSpurious(intId) != INTERRUPT_TYPE_SPURIOUS)
  {
    /* Select custom handlers */
    if (intId <= kspCpuInterruptConfig->maxInterruptLine &&
        spHandlerTable[intId] != NULL)
    {
      handler = spHandlerTable[intId];
    }
    else
    {
      handler = NULL;
      PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "Unhandled interrupt", true);
    }

    /* Execute the handler */
    schedule = handler();
  }
  else
  {
    _SpuriousHandler();
    schedule = true;
  }

  /* Schedule, we will never return */
  if (schedule == true)
  {
    SchedulerSchedule();
  }
  else
  {
    CPURestoreContext(pCurrentThread);
  }

  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "Interrupt Handler Returned.",
        true);
}

void InterruptInit(void)
{
  size_t tableSize;

  /* Get the CPU interrupt configuration */
  kspCpuInterruptConfig = CPUGetInterruptConfig();

  tableSize = sizeof(T_InterruptHandler) *
              kspCpuInterruptConfig->maxInterruptLine + 1;

  /* Allocate and blank custom interrupt handlers */
  spHandlerTable = KMalloc(tableSize, ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);
  memset(spHandlerTable, 0, tableSize);

  /* Init driver */
  sInterruptDriver.pGetIRQInterruptLine = _InitDriverGetIRQIntLine;
  sInterruptDriver.pHandleSpurious      = _InitDriverHandleSpurious;
  sInterruptDriver.pSetIRQEOI           = _InitDriverSetIRQEOI;
  sInterruptDriver.pSetIRQMask          = _InitDriverSetIRQMask;
}

E_Return InterruptSetDriver(const S_InterruptDriver* kpDriver)
{
  E_Return retCode;

  /* We can only set one interrupt manager*/
  INTERRUPT_ASSERT(sInterruptDriver.pGetIRQInterruptLine ==
                   _InitDriverGetIRQIntLine,
                   "Only one interrupt driver can be registered.",
                   ERR_UNAUTHORIZED_ACTION)

  if (kpDriver != NULL &&
      kpDriver->pSetIRQEOI != NULL &&
      kpDriver->pSetIRQMask != NULL &&
      kpDriver->pHandleSpurious != NULL &&
      kpDriver->pGetIRQInterruptLine != NULL)
  {
    sInterruptDriver = *kpDriver;
    retCode          = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return InterruptRegister(const uint32_t           kInterruptId,
                           const T_InterruptHandler kHandler,
                           const bool               kIsIRQ)
{
  uint32_t interruptLine;
  E_Return retCode;

  if (kHandler != NULL)
  {
    if (kIsIRQ == true)
    {
      /* Get the interrupt line attached to the IRQ number. */
      interruptLine = sInterruptDriver.pGetIRQInterruptLine(kInterruptId);
    }
    else
    {
      interruptLine = kInterruptId;
    }

    if (interruptLine >= kspCpuInterruptConfig->minInterruptLine &&
        interruptLine <= kspCpuInterruptConfig->maxInterruptLine)
    {
      if (spHandlerTable[interruptLine] == NULL)
      {
        spHandlerTable[interruptLine] = kHandler;
        retCode                       = NO_ERROR;
      }
      else
      {
        retCode = ERR_UNAUTHORIZED_ACTION;
      }
    }
    else
    {
      retCode = ERR_INVALID_PARAMETER;
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return InterruptRemove(const uint32_t kInterruptId, const bool kIsIRQ)
{
  uint32_t interruptLine;
  E_Return retCode;

  if (kIsIRQ == true)
  {
    /* Get the interrupt line attached to the IRQ number. */
    interruptLine = sInterruptDriver.pGetIRQInterruptLine(kInterruptId);
  }
  else
  {
    interruptLine = kInterruptId;
  }

  if (interruptLine >= kspCpuInterruptConfig->minInterruptLine &&
      interruptLine <= kspCpuInterruptConfig->maxInterruptLine)
  {
    spHandlerTable[interruptLine] = NULL;
    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

void InterruptRestore(const uint32_t kPrevState)
{
  if (kPrevState != 0)
  {
    CPUInterruptEnable();
  }
}

uint32_t InterruptDisable(void)
{
  return CPUInterruptDisable();
}

void InterruptSetIRQMask(const uint32_t kIRQNumber, const bool kEnabled)
{
  sInterruptDriver.pSetIRQMask(kIRQNumber, kEnabled);
}

void InterruptSetEOI(const uint32_t kInterruptId)
{
  sInterruptDriver.pSetIRQEOI(kInterruptId);
}

/************************************ EOF *************************************/