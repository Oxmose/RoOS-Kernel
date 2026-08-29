/*******************************************************************************
 * @file UserKernelLib.h
 *
 * @see UserKernelLib.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief User kernel library.
 *
 * @details User kernel library. This library provides non standard link
 * between the user and the kernel space.
 *
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_USER_KERNEL_LIB_H_
#define __LIB_USER_KERNEL_LIB_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

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
 * @brief Performs a system call.
 *
 * @details Performs a system call. The underlying CPU system call facility will
 * be called to perform the required operation and issue the system call.
 * The parameters for input and output are provided by the pParams parameter.
 *
 * @param[in] kSyscallId The system call identifier to use.
 * @param[in, out] pParam0 The first parameter to pass to the system call.
 * @param[in, out] pParam1 The second parameter to pass to the system call.s
 * @param[in, out] pParam2 The third parameter to pass to the system call.
 * @param[in, out] pParam3 The fourth parameter to pass to the system call.
 * @param[in, out] pParam4 The fifth parameter to pass to the system call.
 */
void Syscall(const unsigned long long kSyscallId,
             void*                    pParam0,
             void*                    pParam1,
             void*                    pParam2,
             void*                    pParam3,
             void*                    pParam4);

#ifdef _STACK_PROT
__attribute__((noreturn)) void __stack_chk_fail(void);
#endif

#endif /* #ifndef __LIB_USER_KERNEL_LIB_H_ */

/************************************ EOF *************************************/