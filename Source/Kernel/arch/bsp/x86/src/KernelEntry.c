/*******************************************************************************
 * @file KernelEntry.c
 *
 * @see KernelEntry.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief Kernel's main boot sequence.
 *
 * @warning At this point interrupts should be disabled.
 *
 * @details Kernel's booting sequence. Initializes the rest of the kernel and
 * performs GDT, IDT and TSS initialization. Initializes the hardware and
 * software core of the kernel before calling the scheduler.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <Memory.h>
#include <stddef.h> /* TODO Remove */
#include <Console.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <DebugOutput.h>
#include <TimerManager.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <KernelEntry.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "KICKSTART"

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/* None */

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
#define KICKSTART_ASSERT(COND, MSG, ERROR) {            \
  if ((COND) == false)                                  \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false);              \
  }                                                     \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel device tree loading virtual address in memory */
extern uintptr_t _KERNEL_DEV_TREE_BASE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void X64KernelEntry(void)
{
  /* Start testing framework */
  TEST_FRAMEWORK_START();

#if OUTPUT_DEBUG_ENABLE
  /* Enable the debug output port */
  DebugOutputInit();
#endif

  KERNEL_INFO("Starting kernel...\n");

  /* Initialize the console */
  ConsoleInit();
  KERNEL_SUCCESS("Console initialized.\n");

  /* Initialize the kernel heap */
  KernelHeapInit();
  KERNEL_SUCCESS("Kernel heap initialized.\n");

  /* Initialize the device tree */
  DeviceTreeInit((uintptr_t)&_KERNEL_DEV_TREE_BASE);
  KERNEL_SUCCESS("Device tree initialized.\n");

  /* Initializes interrupts */
  InterruptInit();
  CPURegisterExceptions();
  KERNEL_SUCCESS("Interrupts initialized.\n");

  /* Initialize the CPU */
  CPUInit();
  KERNEL_SUCCESS("CPU initialized.\n");

  /* Initialize the memory manager */
  MemoryInit();
  KERNEL_SUCCESS("Memory manager initialized.\n");

  /* Initialize the drivers */
  DriverManagerInit();
  KERNEL_SUCCESS("Drivers initialized.\n");

  /* Initialize the time manager */
  TimeManagerInit();
  KERNEL_SUCCESS("Time manager initialized.\n");

  /* Now that devices are configured, start the CPU manager, in charge of
   * starting other CPUs if needed. After calling this function all the
   * running CPUs excepted this one have their interrupt enabled.
   */
  CPUStartSMP();
  KERNEL_SUCCESS("SMP started.\n");

  /* Initilize the scheduler */
  SchedulerInit();
  KERNEL_SUCCESS("Scheduler initialized.\n");

  KERNEL_INFO("Kernel started successfully.\n");

  /* Add library and core tests here */
  TEST_POINT_FUNCTION_CALL(PanicTest, TEST_PANIC_ENABLED);
  TEST_POINT_FUNCTION_CALL(UHashtableTest, TEST_OS_UHASHTABLE_ENABLED);

  #if 0
  /* TODO: Remove */
  extern void TestKernel(void);
  TestKernel();
#endif
  /* Perform first schedule */
  SchedulerSchedule();

  /* Once the scheduler is started, we should never come back here. */
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "Entry returned.", false);
}

/************************************ EOF *************************************/