/*******************************************************************************
 * @file SysCall.c
 *
 * @see SysCall.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/08/2026
 *
 * @version 1.0
 *
 * @brief Kernel's user entry point.
 *
 * @details Kernel system call manager. Used to register and handle system call
 * entry and exti points.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <stdint.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <SysCall.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the current module name */
#define MODULE_NAME "SYSCALL"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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
 * FUNCTIONS
 ******************************************************************************/
void SystemCallDispatcher(const uint64_t kSyscallId,
                          void*          pParam0,
                          void*          pParam1,
                          void*          pParam2,
                          void*          pParam3,
                          void*          pParam4)
{
  (void)kSyscallId;
  (void)pParam0;
  (void)pParam1;
  (void)pParam2;
  (void)pParam3;
  (void)pParam4;
}

/************************************ EOF *************************************/