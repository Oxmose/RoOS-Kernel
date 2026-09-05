/*******************************************************************************
 * @file SysCall.h
 *
 * @see SysCall.c
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

#ifndef __USER_SYSCALL_H_
#define __USER_SYSCALL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

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
 * FUNCTIONS
 ******************************************************************************/

 /**
 * @brief Handles a system call request from the user space.
 *
 * @details Handles a system call request from the user space. This function
 * will execute the necessary process to handle the system call, call the
 * associated kernel function and setup the return arguments.
 *
 * @param[in] kSyscallId The system call ID to handle.
 * @param[in] pParam0 The first parameter to pass to the system call handler.
 * @param[in] pParam1 The second parameter to pass to the system call handler.
 * @param[in] pParam2 The third parameter to pass to the system call handler.
 * @param[in] pParam3 The fourth parameter to pass to the system call handler.
 * @param[in] pParam4 The fifth parameter to pass to the system call handler.
 *
 * @return The result of the system call.
 */
void* SystemCallDispatcher(const uint64_t kSyscallId,
                           void*          pParam0,
                           void*          pParam1,
                           void*          pParam2,
                           void*          pParam3,
                           void*          pParam4);

#endif /* #ifndef __USER_SYSCALL_H_ */

/************************************ EOF *************************************/