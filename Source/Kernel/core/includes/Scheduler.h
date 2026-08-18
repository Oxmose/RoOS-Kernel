/*******************************************************************************
 * @file Scheduler.h
 *
 * @see Scheduler.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/06/2024
 *
 * @version 6.0
 *
 * @brief Kernel's thread scheduler.
 *
 * @details Kernel's thread scheduler. Thread creation and management functions
 * are located in this file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_SCHEDULER_H_
#define __CORE_SCHEDULER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <CtrlBlock.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Scheduler's thread lowest priority. */
#define KERNEL_LOWEST_PRIORITY  63
/** @brief Scheduler's thread highest priority. */
#define KERNEL_HIGHEST_PRIORITY 0

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
 * @brief Initializes the scheduler service.
 *
 * @details Initializes the scheduler features and structures. The idle and
 * init threads are created. Once set, the scheduler starts to schedule the
 * threads.
 *
 * @warning This function will never return if the initialization was successful
 * and the scheduler started.
 */
void SchedulerInit(void);

/**
 * @brief Calls the scheduler dispatch function.
 *
 * @details Calls the scheduler. This function will select the next thread to
 * schedule and execute it.
 *
 * @warning The current thread's context must be saved before calling this
 * function. Usually, this function is only called in interrupt handlers after
 * the thread's context was saved. Use schedSchedule to save the context.
 *
 * @return This functio does not return but to maintain compatibility is
 * return false.
 */

bool SchedulerSchedule(void);

/**
 * @brief Returns the handle to the current running thread.
 *
 * @details Returns the handle to the current running thread. This value should
 * never be NULL as a thread should always be elected for running.
 *
 * @return A handle to the current running thread is returned.
 */
S_KernelThread* SchedulerGetCurrentThread(void);

/**
 * @brief Sets the thread as errored.
 *
 * @details Sets the thread as errored and prevents it from executing again.
 *
 * @param[out] pThread The thread to set to errored.
 */
void SchedulerSetThreadErrored(S_KernelThread* pThread);

/**
 * @brief Tells if the scheduler has been initialized.
 *
 * @details Tells if the scheduler has been initialized.
 *
 * @return True is returned once the scheduler is initialized, false otherwise.
 */
bool SchedulerIsInitialized(void);

#endif /* #ifndef __CORE_SCHEDULER_H_ */

/************************************ EOF *************************************/