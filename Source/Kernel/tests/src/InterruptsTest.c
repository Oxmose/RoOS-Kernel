
/*******************************************************************************
 * @file InterruptsTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework interrupts testing.
 *
 * @details Testing framework interrupts testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Scheduler.h>
#include <Interrupts.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestFramework.h>


/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Master PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_MASTER 0x07
/** @brief Slave PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_SLAVE  0x0F

#define INT_PIC_IRQ_OFFSET 0x30

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static bool _HandlerTest(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static bool _HandlerTest(void)
{
  return false;
}

void InterruptsTest(void)
{
  E_Return err;

  const S_CPUInterruptConfiguration* pConfig;
  pConfig = CPUGetInterruptConfig();

  /* TEST REGISTER < MIN */
  err = InterruptRegister(pConfig->minInterruptLine - 1, _HandlerTest, false);
  TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT(0),
                          err == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          err,
                          TEST_INTERRUPT_ENABLED);

  /* TEST REGISTER > MAX */
  err = InterruptRegister(pConfig->maxInterruptLine + 1, _HandlerTest, false);
  TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT(1),
                          err == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          err,
                          TEST_INTERRUPT_ENABLED);

  /* TEST REMOVE < MIN */
  err = InterruptRemove(pConfig->minInterruptLine - 1, false);
  TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT(2),
                          err == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          err,
                          TEST_INTERRUPT_ENABLED);

  /* TEST REMOVE > MAX */
  err = InterruptRemove(pConfig->maxInterruptLine + 1, false);
  TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT(3),
                          err == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          err,
                          TEST_INTERRUPT_ENABLED);

  /* TEST NULL HANDLER */
  err = InterruptRegister(pConfig->minInterruptLine, NULL, false);
  TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT(4),
                          err == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          err,
                          TEST_INTERRUPT_ENABLED);

  /* TEST REGISTER WHEN ALREADY REGISTERED */
  err = InterruptRegister(pConfig->minInterruptLine, _HandlerTest, false);
  TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT(5),
                          err == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION,
                          err,
                          TEST_INTERRUPT_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/