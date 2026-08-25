/*******************************************************************************
 * @file CoreProcessFS.h
 *
 * @see CoreProcessFS.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/08/2026
 *
 * @version 1.0
 *
 * @brief Kernel's processes PROCFS entries manager.
 *
 * @details Kernel's processes PROCFS entries manager. This module manages the
 * PROCFS entries for the kernel's processes. It allows to read the information
 * from the kernel's processes and write it to the buffer given as parameter.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_COREPROCESSFS_H_
#define __CORE_COREPROCESSFS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <CtrlBlock.h>
#include <KernelError.h>

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
 * @brief Creates the ProcFS entry for the process.
 *
 * @details Creates the ProcFS entry for the process. This function will create
 * the entry and register it in the ProcFS. The entry will be used to read the
 * information from the process.
 *
 * @param[in, out] pProcess The process for which the entry should be created.
 *
 * @return The function returns the success state or the error code.
 */
E_Return CoreProcessFSCreateEntry(S_KernelProcess* pProcess);

/**
 * @brief Deletes the ProcFS entry for the process.
 *
 * @details Deletes the ProcFS entry for the process. This function will delete
 * the entry and unregister it from the ProcFS. All sub entries will be deleted
 * as well.
 *
 * @param[in, out] pProcess The process for which the entry should be deleted.
 */
void CoreProcessFSDeleteEntry(S_KernelProcess* pProcess);

/**
 * @brief Creates the ProcFS entry for the thread.
 *
 * @details Creates the ProcFS entry for the thread. This function will create
 * the entry and register it in the ProcFS. The entry will be used to read the
 * information from the thread.
 *
 * @param[in, out] pThread The thread for which the entry should be created.
 *
 * @return The function returns the success state or the error code.
 */
E_Return CoreProcessFSCreateThreadEntry(S_KernelThread* pThread);

/**
 * @brief Deletes the ProcFS entry for the thread.
 *
 * @details Deletes the ProcFS entry for the thread. This function will delete
 * the entry and unregister it from the ProcFS.
 *
 * @param[in, out] pThread The thread for which the entry should be deleted.
 */
void CoreProcessFSDeleteThreadEntry(S_KernelThread* pThread);

#endif /* #ifndef __CORE_COREPROCESSFS_H_ */

/************************************ EOF *************************************/