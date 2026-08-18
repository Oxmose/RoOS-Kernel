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
/**
 * @brief Defines the size of the window for which the CPU load is calculated
 * in ticks
 */
#define CPU_LOAD_TICK_WINDOW 100
/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines a thread routine type. */
typedef void* (*T_ThreadRoutine)(void*);

/** @brief Context statistics, used to calculate the CPU scores. */
typedef struct
{
  /** @brief Stores the times spent in idle in the last window */
  uint64_t idleTimes[CPU_LOAD_TICK_WINDOW];
  /** @brief Stores the total time in the last window */
  uint64_t totalTimes[CPU_LOAD_TICK_WINDOW];
  /** @brief Stores the current idle time average */
  uint64_t idleTime;
  /** @brief Stores the current total time average */
  uint64_t totalTime;
  /** @brief Index in the times table */
  uint32_t timesIdx;
  /** @brief Stores the last saved time. */
  uint64_t lastTime;
  /** @brief Stores the fitness score of the CPU */
  uint64_t score;
} S_ScheduleContextStatistics;

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
 * @brief Returns the handle to the current running process.
 *
 * @details Returns the handle to the current running process. This value should
 * never be NULL as a process should always be elected for running.
 *
 * @return A handle to the current running process is returned.
 */
S_KernelProcess* SchedulerGetCurrentProcess(void);

/**
 * @brief Sets the current thread as errored.
 *
 * @details Sets the currrent thread as errored and prevents it from executing
 * again.
 */
void SchedulerSetCurrentThreadErrored(void);

/**
 * @brief Sets the current thread to waiting state.
 *
 * #details Sets the current thread to waiting state. The thread will not be
 * scheduled until it is set to ready state again.
 *
 * @warning This function should be called with the thread's lock acquired.
 */
void SchedulerSetCurrentThreadToWaiting(void);

/**
 * @brief Sets the thread to ready state.
 *
 * @details Sets the thread to ready state. The thread will be scheduled again
 * when it is the highest priority thread in the ready state.
 *
 * @param[out] pThread The thread to set to ready state.
 */
void SchedulerSetThreadToReady(S_KernelThread* pThread);

/**
 * @brief Set the thread's priority.
 *
 * @details Set the thread's priority. The thread will be scheduled according to
 * its new priority.
 *
 * @param[out] pThread The thread to set the priority.
 * @param[in] kPriority The priority to set. Must be between KERNEL_HIGHEST_PRIORITY
 * and KERNEL_LOWEST_PRIORITY.
 */
void SchedulerSetThreadPriority(S_KernelThread* pThread,
                                const uint32_t  kPriority);

/**
 * @brief Tells if the scheduler has been initialized.
 *
 * @details Tells if the scheduler has been initialized.
 *
 * @return True is returned once the scheduler is initialized, false otherwise.
 */
bool SchedulerIsInitialized(void);

/**
 * @brief Returns the scheduler statistics for a given CPU.
 *
 * @param kCPU The CPU to get the statistics for.
 *
 * @return The scheduler statistics for the given CPU are returned. If the CPU
 * is not valid, NULL is returned.
 */
const S_ScheduleContextStatistics* SchedulerGetStatistics(const uint32_t kCPU);

/**
 * @brief Creates and starts a new thread.
 *
 * @details Creates and starts a new thread. The thread is created with the given
 * parameters and is scheduled for execution. The thread's routine is executed
 * with the given arguments. The thread's return value can be retrieved by
 * calling JoinThread.
 *
 * @param[out] ppThread The pointer to the thread structure. This is the handle
 * of the thread for the user.
 * @param[in] kIsKernel Tells if the created thread is a kernel or user thread.
 * @param[in] kPriority The priority of the thread.
 * @param[in] kpName The name of the thread.
 * @param[in] kStackSize The thread's stack size in bytes, must be a multiple of
 * the system's page size.
 * @param[in] kMappedCPUs The CPU affinity set for this thread.
 * @param[in] kRoutine The thread routine to be executed.
 * @param[in] args The arguments to be used by the thread.
 *
 * @return The function returns NO_ERROR if the thread was created successfully,
 * or an error code otherwise.
 */
E_Return CreateThread(S_KernelThread**      ppThread,
                      const bool            kIsKernel,
                      const uint8_t         kPriority,
                      const char            kName[THREAD_NAME_MAX_LENGTH],
                      const size_t          kStackSize,
                      const S_CPUMask       kMappedCPUs,
                      const T_ThreadRoutine kRoutine,
                      void*                 args);

/**
 * @brief Joins a thread and retrieves its return value.
 *
 * @details Joins a thread and retrieves its return value. This function waits
 * for the specified thread to finish its execution and retrieves its return
 * value. The thread must be in a joinable state, and the calling thread will be
 * blocked until the target thread completes.
 *
 * @param[in] pThread The thread to join.
 * @param[out] ppReturnValue The pointer to the return value.
 *
 * @return The function returns NO_ERROR if the thread was joined successfully,
 * or an error code otherwise.
 */
E_Return JoinThread(S_KernelThread* pThread,
                    void**          ppReturnValue);

/**
 * @brief Puts the calling thread to sleep for a specified duration in
 * nanoseconds.
 *
 * @details Puts the calling thread to sleep for a specified duration in
 * nanoseconds. The thread will be blocked and will not be scheduled until the
 * specified time has elapsed.
 *
 * @param[in] kTimeNs The duration in nanoseconds for which the thread should
 * sleep.
 *
 * @return The function returns NO_ERROR if the thread was put to sleep
 * successfully, or an error code otherwise.
 */
E_Return SleepNs(const uint64_t kTimeNs);

#endif /* #ifndef __CORE_SCHEDULER_H_ */

/************************************ EOF *************************************/