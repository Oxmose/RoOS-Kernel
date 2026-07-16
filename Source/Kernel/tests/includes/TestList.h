/******************************************************************************
 * @file TestList.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 10/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework functions and list.
 *
 * @details Testing framework functions and list. This file gathers the enable
 * flags for unit testing as well as the testing functions declarations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __TEST_FRAMEWORK_TEST_LIST_H_
#define __TEST_FRAMEWORK_TEST_LIST_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*************************************************
 * TESTING ENABLE FLAGS
 ************************************************/
/** @brief Panic test enabled flag */
#define TEST_PANIC_ENABLED                        0
#define TEST_SCHEDULER_ENABLED                    1

/*************************************************
 * TEST IDENTIFIERS
 ************************************************/

/** @brief Panic test */
#define PANIC_TEST_SUCCESS_ID                           0

/** @brief Scheduler test */
#define SCHED_TEST_IS_INIT_ID                      1000
#define SCHED_TEST_CREATE_TEST_THREAD_ID(X)        (1100 + X)
#define SCHED_TEST_SLEEP_ID(X)                     (1200 + X)
#define SCHED_TEST_JOIN_THREAD_ID(X)               (1300 + X)
#define SCHED_TEST_GETTER_ID(X)                    (1400 + X)
#define SCHED_TEST_ERROR_THREAD_ID(X)              (1500 + X)
#define SCHED_TEST_PRIORITY_THREAD_CREATE_ID(X)    (1600 + X)
#define SCHED_TEST_PRIORITY_THREAD_JOIN_ID(X)      (1700 + X)
#define SCHED_TEST_PRIORITY_THREAD_CHECK_ID(X)     (1900 + X)
#define SCHED_TEST_MAPPING_THREAD_CREATE_ID(X)     (2000 + X)
#define SCHED_TEST_MAPPING_THREAD_JOIN_ID(X)       (2100 + X)
#define SCHED_TEST_MAPPING_THREAD_CHECK_ID(X)      (2200 + X)

/** @brief Current test name */
#define TEST_FRAMEWORK_TEST_NAME "Kernel Scheduler"

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

/** @brief Panic test function */
void PanicTest(void);

/** @brief Scheduler main test function */
void SchedulerTest(void);

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/