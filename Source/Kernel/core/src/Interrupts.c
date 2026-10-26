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
#include <TestFramework.h>

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
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
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

/** @brief Stores the interrupt allocation map. */
static uint64_t* spInterruptAllocMap;
/** @brief Stores the size of the interrupt allocation map. */
static uint32_t sInterruptAllocMapSize;

/** @brief Stores the interrupt lock. */
static S_KernelSpinlock sInterruptLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _InitDriverSetIRQEOI(const uint32_t kIRQNumber)
{
  (void)kIRQNumber;
}

static E_InterruptType _InitDriverHandleSpurious(const uint32_t kIntNumber)
{
  (void)kIntNumber;
  return INTERRUPT_TYPE_REGULAR;
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
  pCurrentThread = GetCurrentThread();
  intId          = CPUGetContextInterruptNumber(pCurrentThread);

  /* Check for spurious interrupt */
  if (sInterruptDriver.pHandleSpurious(intId) != INTERRUPT_TYPE_SPURIOUS)
  {
    /* Select custom handlers */
    if (intId < kspCpuInterruptConfig->totalInterruptLines &&
        spHandlerTable[intId] != NULL)
    {
      handler = spHandlerTable[intId];
    }
    else
    {
      handler = NULL;
      PANIC(ERR_UNAUTHORIZED_ACTION,
            MODULE_NAME,
            "Unhandled interrupt",
            true,
            false);
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
    SchedulerRestoreThread(pCurrentThread);
  }

  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "Interrupt Handler Returned.",
        true,
        false);
}

void InterruptInit(void)
{
  size_t   tableSize;
  uint32_t i;

  /* Get the CPU interrupt configuration */
  kspCpuInterruptConfig = CPUGetInterruptConfig();

  tableSize = sizeof(T_InterruptHandler) *
              kspCpuInterruptConfig->totalInterruptLines;

  /* Allocate and blank custom interrupt handlers */
  spHandlerTable = KMalloc(tableSize, KMALLOC_NO_FREE_POOL);
  memset(spHandlerTable, 0, tableSize);

  /* Setup the interrupts allocator */
  tableSize = kspCpuInterruptConfig->totalInterruptLines / 64;
  if ((kspCpuInterruptConfig->totalInterruptLines & 63) != 0)
  {
    tableSize++;
  }
  sInterruptAllocMapSize = tableSize;
  tableSize *= sizeof(uint64_t);
  spInterruptAllocMap = KMalloc(tableSize, KMALLOC_NO_FREE_POOL);
  memset(spInterruptAllocMap, 0, tableSize);

  /* Setup the non free interrupt lines from begining */
  for (i = 0; i < kspCpuInterruptConfig->minFreeInterruptLine; ++i)
  {
    spInterruptAllocMap[i / 64] |= (1ULL << (i & 63));
  }
  /* Setup the non free interrupt lines to the end */
  for (i = kspCpuInterruptConfig->maxFreeInterruptLine + 1;
       i < tableSize / sizeof(uint64_t);
       ++i)
  {
    spInterruptAllocMap[i / 64] |= (1ULL << (i & 63));
  }
  /* Setup the special interrupt lines */
  i = kspCpuInterruptConfig->schedulerInterruptLine;
  spInterruptAllocMap[i / 64] |= (1ULL << (i & 63));
  i = kspCpuInterruptConfig->spuriousInterruptLine;
  spInterruptAllocMap[i / 64] |= (1ULL << (i & 63));
  i = kspCpuInterruptConfig->ipiInterruptLine;
  spInterruptAllocMap[i / 64] |= (1ULL << (i & 63));

  /* Init driver */
  sInterruptDriver.pHandleSpurious    = _InitDriverHandleSpurious;
  sInterruptDriver.pSetEndOfInterrupt = _InitDriverSetIRQEOI;
}

E_Return InterruptSetDriver(const S_InterruptDriver* kpDriver)
{
  E_Return retCode;

  /* We can only set one interrupt manager*/
  INTERRUPT_ASSERT(sInterruptDriver.pHandleSpurious ==
                   _InitDriverHandleSpurious,
                   "Only one interrupt driver can be registered.",
                   ERR_UNAUTHORIZED_ACTION)

  if (kpDriver != NULL &&
      kpDriver->pHandleSpurious != NULL &&
      kpDriver->pSetEndOfInterrupt != NULL)
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
                           const T_InterruptHandler kHandler)
{
  E_Return retCode;

  if (kHandler != NULL)
  {
    KERNEL_LOCK(sInterruptLock);
    if (kInterruptId < kspCpuInterruptConfig->totalInterruptLines)
    {
      if (spHandlerTable[kInterruptId] == NULL)
      {
        spHandlerTable[kInterruptId] = kHandler;
        retCode                      = NO_ERROR;
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
    KERNEL_UNLOCK(sInterruptLock);
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return InterruptRemove(const uint32_t kInterruptId)
{
  E_Return retCode;

  KERNEL_LOCK(sInterruptLock);
  if (kInterruptId < kspCpuInterruptConfig->totalInterruptLines)
  {
    spHandlerTable[kInterruptId] = NULL;
    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }
  KERNEL_UNLOCK(sInterruptLock);

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

void InterruptSetEOI(const uint32_t kInterruptId)
{
  sInterruptDriver.pSetEndOfInterrupt(kInterruptId);
}

int32_t AllocateInterruptLine(void)
{
  uint32_t i;
  int32_t  newInt;

  KERNEL_LOCK(sInterruptLock);

  newInt = -1;
  for (i = 0; i < sInterruptAllocMapSize; ++i)
  {
    if (spInterruptAllocMap[i] != UINT64_MAX)
    {
      for (uint32_t j = 0; j < 64; ++j)
      {
        if ((spInterruptAllocMap[i] & (1ULL << j)) == 0)
        {
          spInterruptAllocMap[i] |= (1ULL << j);
          newInt = (int32_t)(i * 64 + j);
          i = sInterruptAllocMapSize;
          break;
        }
      }
    }
  }

  KERNEL_UNLOCK(sInterruptLock);

  return newInt;
}

void FreeInterruptLine(const int32_t kInterruptId)
{
  KERNEL_LOCK(sInterruptLock);

  if (kInterruptId >= 0 &&
      (uint32_t)kInterruptId < kspCpuInterruptConfig->totalInterruptLines)
  {
    spInterruptAllocMap[kInterruptId / 64] &= ~(1ULL << (kInterruptId & 63));
  }

  KERNEL_UNLOCK(sInterruptLock);
}
/************************************ EOF *************************************/