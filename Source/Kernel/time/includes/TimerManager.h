/*******************************************************************************
 * @file TimerManager.h
 *
 * @see TimerManager.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/05/2023
 *
 * @version 1.0
 *
 * @brief Kernel's time management methods.
 *
 * @details Kernel's time management method. Allow to define timers and keep
 * track on the system's time.
 *
 * @warning All the interrupt managers and timer sources drivers must be
 * initialized before using any of these functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __TIME_TIME_MANAGER_H_
#define __TIME_TIME_MANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <Interrupts.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Date structure. */
typedef struct
{
  /** @brief Day of the week. */
  uint16_t weekday;
  /** @brief Day of the month. */
  uint16_t day;
  /** @brief Month of the year. */
  uint16_t month;
  /** @brief Year. */
  uint16_t year;
} S_Date;

/** @brief Time structure. */
typedef struct
{
  /** @brief Hours. */
  uint8_t hours;
  /** @brief Minutes. */
  uint8_t minutes;
  /** @brief Seconds. */
  uint8_t seconds;
} S_DayTime;

/** @brief Defines the types of timers available. */
typedef enum
{
  /** @brief Main timer, used for scheduling and general time keeping. */
  MAIN_TIMER,
  /** @brief RTC timer, used real time clock information. */
  RTC_TIMER,
  /** @brief Auxiliary timers, can be used for general purpose. */
  AUX_TIMER,
  /** @brief Lifetime timer, keeps track of the uptime. */
  LIFETIME_TIMER
} E_TimerType;

/**
 * @brief The kernel's timer driver abstraction.
 */
typedef struct
{
  /**
   * @brief The function should return the frequency of the timer source.
   *
   * @details The function should return the frequency of the timer source.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   *
   * @return The function should return the frequency of the timer source.
   */
  uint64_t (*pGetFrequency)(void* pDriverCtrl);

  /**
   * @brief Returns the time elasped since the last timer's reset in ns.
   *
   * @details Returns the time elasped since the last timer's reset in ns. The
   * timer can be set with the pSetTimeNs function.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   *
   * @return The time in nanosecond since the last timer reset is returned.
   */
  uint64_t (*pGetTimeNs)(void* pDriverCtrl);

  /**
   * @brief Sets the time elasped in ns.
   *
   * @details Sets the time elasped in ns. The timer can be get with the
   * pGetTimeNs function.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   * @param[in] kTimeNS The time in nanoseconds to set.
   */
  void (*pSetTimeNs)(void* pDriverCtrl, const uint64_t kTimeNS);

  /**
   * @brief Returns the current date.
   *
   * @details Returns the current date in RTC date format.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   *
   * @return The current date in in RTC date format
   */
  S_Date (*pGetDate)(void* pDriverCtrl);

  /**
   * @brief Returns the current daytime.
   *
   * @details Returns the current daytime.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   *
   * @return The current daytime.
   */
  S_DayTime (*pGetDaytime)(void* pDriverCtrl);

  /**
   * @brief The function should enable the timer's interrupt.
   *
   * @details The function should enable the timer's interrupt.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   */
  void (*pEnable)(void* pDriverCtrl);

  /**
   * @brief The function should disable the timer's interrupt.
   *
   * @details The function should disable the timer's interrupt.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   */
  void (*pDisable)(void* pDriverCtrl);

  /**
   * @brief The function should set the timer's tick handler.
   *
   * @details The function should set the timer's tick handler. The handler
   * will be called at each tick received.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   * @param[in] handler The handler of the timer's interrupt.
   *
   * @return The success state or the error code.
   */
  E_Return (*pSetHandler)(void* pDriverCtrl, T_InterruptHandler handler);

  /**
   * @brief The function should remove the timer tick handler.
   *
   * @details The function should remove the timer tick handler.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   *
   * @return The success state or the error code.
   */
  E_Return (*pRemoveHandler)(void* pDriverCtrl);

  /** @brief This function can be used to execute an operation in the
   * driver at every tick of the timer.
   *
   * @param[in, out] pDriverCtrl The driver controller used by the registered
   * driver.
   *
   * @details This function can be used to execute an operation in the
   * driver at every tick of the timer such as a tick acknowledge.
   */
  void (*pTickManager)(void* pDriverCtrl);

  /**
   * @brief Contains a pointer to the driver controller, set by the driver
   * at the moment of the initialization of this structure.
   */
  void* pDriverCtrl;
} S_KernelTimer;

/** @brief Clock ID for the clock and timer functions */
typedef enum
{
  /**
   * @brief System-wide realtime clock. Setting this clock requires
   * appropriate privileges.
   */
  CLOCK_REALTIME = 0,
  /**
   * @brief Clock that cannot be set and represents monotonic time since some
   * unspecified starting point.
   */
  CLOCK_MONOTONIC = 1,
} E_ClockType;

/** @brief Real arithmetic type capable of representing times. */
typedef int64_t T_Time;

/**
 * @brief Structure holding an interval broken down into seconds and
 * nanoseconds.
 */
typedef struct
{
  /** @brief Whole seconds (valid values are >= 0) */
  T_Time tvSec;
  /** @brief Nanoseconds (valid values are [0, 999999999]) */
  long int tvNsec;
} S_TimeSpec;

/**
 * @brief Timer periodic task structure.
 *
 * @details Timer periodic task structure. This structure is used to store the
 * information for the timer manager. The task will be executed at every tick of
 * the timer in the timer ISR context. The task will be executed in the context
 * of the timer ISR, so it should be a short and fast operation. The task should
 * not block or perform any long operations. The
 *
 * @param[in] pArg The argument to pass to the task function.
 */
typedef void (*T_TimerPeriodicTask)(void* pArg);

/** @brief Type for a timer handle. */
typedef void* T_TimerHandle;

/** @brief Structure for a periodic timer task. */
typedef struct
{
  /** @brief Pointer to the task function. */
  T_TimerPeriodicTask pTask;
  /** @brief Pointer to the argument for the task function. */
  void* pArg;
  /** @brief Period of the task in nanoseconds. */
  uint64_t periodNs;
} S_TimerPeriodicTask;


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
 * @brief Initializes the time manager.
 *
 * @details Initializes the time manager. The timers will be added based on the
 * FDT configuration. This time manager should be initialized after all drivers
 * were attached.
 */
void TimeManagerInit(void);

/**
 * @brief Returns the current uptime.
 *
 * @details Return the current uptime of the system in ns.
 *
 * @return The current uptime in ns.
 */
uint64_t TimeGetUptime(void);

/**
 * @brief Returns the current daytime from RTC.
 *
 * @details Return the current daytime from RTC.
 *
 * @return The current daytime from RTC.
 */
S_DayTime TimeGetDayTime(void);

/**
 * @brief Returns the current date from RTC.
 *
 * @details Return the current date from RTC.
 *
 * @return The current date from RTC.
 */
S_Date TimeGetDate(void);

/**
 * @brief Returns the number of CPU's ticks since the CPU started.
 *
 * @details Returns the number of CPU's ticks since the CPU started.
 *
 * @param[in] kCpuId The CPU identifer of which to get the number of ticks.
 *
 * @return The number of CPU's ticks since the CPU started.
 */
uint64_t TimeGetTicks(const uint8_t kCpuId);

/**
 * @brief Performs a wait for ms nanoseconds.
 *
 * @details Performs a wait for ms nanoseconds based on the kernel's main
 * timer.
 *
 * @param[in] kNs The time to wait in nanoseconds.
 *
 * @warning This function must only be called before the scheduler is
 * running. Otherwise the function will have undefined behavior.
 */
void TimeWaitNoScheduler(const uint64_t kNs);

/**
 * @brief Adds a periodic task to the timer manager.
 *
 * @details Adds a periodic task to the timer manager. The task will be executed
 * at every tick of the timer in the timer ISR context. The task will be
 * executed in the context of the timer ISR, so it should be a short and fast
 * operation. The task should not block or perform any long operations. The task
 * will be executed in the context of the timer ISR, so it should be a short and
 * fast operation. The task should not block or perform any long operations.
 *
 * @param[in] pTask Pointer to the periodic task function.
 * @param[in] kPeriodNs The period of the task in nanoseconds.
 *
 * @return A handle to the added task, which can be used to remove it later.
 */
T_TimerHandle TimeManagerAddTask(S_TimerPeriodicTask* pTask,
                                 const uint32_t       kPeriodNs);

/**
 * @brief Removes a periodic task from the timer manager.
 *
 * @details Removes a periodic task from the timer manager. The task will no
 * longer be executed at every tick of the timer in the timer ISR context.
 *
 * @param[in] taskHandle The handle of the task to remove, as returned by
 * TimeManagerAddTask.
 */
void TimeManagerRemoveTask(T_TimerHandle taskHandle);

#endif /* #ifndef __TIME_TIME_MANAGER_H_ */

/************************************ EOF *************************************/