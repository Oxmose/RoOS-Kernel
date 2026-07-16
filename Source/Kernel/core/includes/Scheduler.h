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
 * @brief Tells if the scheduler has been initialized.
 *
 * @details Tells if the scheduler has been initialized.
 *
 * @return True is returned once the scheduler is initialized, false otherwise.
 */
bool SchedulerIsInitialized(void);

/* TODO: Document */
const S_ScheduleContextStatistics* SchedulerGetStatistics(const uint32_t kCPU);

E_Return CreateThread(S_KernelThread**      ppThread,
                      const bool            kIsKernel,
                      const uint8_t         kPriority,
                      const char            kName[THREAD_NAME_MAX_LENGTH],
                      const size_t          kStackSize,
                      const S_CPUMask       kMappedCPUs,
                      const T_ThreadRoutine kRoutine,
                      void*                 args);

E_Return JoinThread(S_KernelThread* pThread,
                    void**          ppReturnValue);

E_Return SleepNs(const uint64_t timeNs);

#endif /* #ifndef __CORE_SCHEDULER_H_ */

/************************************ EOF *************************************/