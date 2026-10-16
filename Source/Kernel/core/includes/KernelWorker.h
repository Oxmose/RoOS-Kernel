/*******************************************************************************
 * @file KernelWorker.h
 *
 * @see KernelWorker.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 15/10/2026
 *
 * @version 1.0
 *
 * @brief Kernel thread worker manager.
 *
 * @details Kernel thread worker manager. Workers are used to execute tasks in
 * the background.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifndef __CORE_KERNELWORKER_H_
#define __CORE_KERNELWORKER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <KernelError.h>
#include <KernelSemaphore.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Invalid task handle */
#define KWORKER_TASK_INVALID ((T_TaskHandle)0)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Task handle type. */
typedef void* T_TaskHandle;

/**
 * @brief Kernel worker task type.
 *
 * @details Kernel worker task type. A kernel worker task is a function that is
 * executed in the background by a kernel worker thread. The task is executed
 * with the provided argument.
 *
 * @param[in] pArg The argument to pass to the task function.
 */
typedef void (*T_KernelWorkerTask)(void* pArg);

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
 * @brief Initializes the kernel worker manager.
 *
 * @details Initializes the kernel worker manager. This function creates the
 * kernel worker threads and prepares them to execute tasks.
 */
void KWorkerInit(void);

/**
 * @brief Submits a task to the kernel worker manager.
 *
 * @details Submits a task to the kernel worker manager. The task will be
 * executed in the background by one of the kernel worker threads.
 *
 * @param[in] task The task to submit.
 * @param[in] pArg The argument to pass to the task function.
 * @param[in] pCompletionSemaphore The semaphore to signal when the task is
 * complete. Can be NULL if no completion signal is needed.
 *
 * @return The error or success status of the task submission.
 */
E_Return KWorkerSubmitTask(T_KernelWorkerTask task,
                           void*              pArg,
                           S_KernelSemaphore* pCompletionSemaphore);

#endif /* #ifndef __CORE_KERNELWORKER_H_ */

/************************************ EOF *************************************/
