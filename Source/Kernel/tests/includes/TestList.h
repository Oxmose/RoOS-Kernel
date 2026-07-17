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
#define TEST_KHEAP_ENABLED                        0
#define TEST_DEVTREE_ENABLED                      0
#define TEST_CRITICAL_ENABLED                     1
#define TEST_INTERRUPT_ENABLED                    0
#define TEST_OS_UHASHTABLE_ENABLED                0
#define TEST_SCHEDULER_ENABLED                    0

/*************************************************
 * TEST IDENTIFIERS
 ************************************************/

/** @brief Panic test */
#define PANIC_TEST_SUCCESS_ID                      0

/** @brief Scheduler test */
#define SCHED_TEST_IS_INIT_ID                      0
#define SCHED_TEST_CREATE_TEST_THREAD_ID(X)        (100 + X)
#define SCHED_TEST_SLEEP_ID(X)                     (200 + X)
#define SCHED_TEST_JOIN_THREAD_ID(X)               (300 + X)
#define SCHED_TEST_GETTER_ID(X)                    (400 + X)
#define SCHED_TEST_ERROR_THREAD_ID(X)              (500 + X)
#define SCHED_TEST_PRIORITY_THREAD_CREATE_ID(X)    (600 + X)
#define SCHED_TEST_PRIORITY_THREAD_JOIN_ID(X)      (700 + X)
#define SCHED_TEST_PRIORITY_THREAD_CHECK_ID(X)     (900 + X)
#define SCHED_TEST_MAPPING_THREAD_CREATE_ID(X)     (1000 + X)
#define SCHED_TEST_MAPPING_THREAD_JOIN_ID(X)       (1100 + X)
#define SCHED_TEST_MAPPING_THREAD_CHECK_ID(X)      (1200 + X)

/** @brief UHashtable test */
#define TEST_UHASHTABLE_CREATE_ID(X)      (X)
#define TEST_UHASHTABLE_SETBURST_ID(X)    (100 + X)
#define TEST_UHASHTABLE_SET_ID(X)         (400 + X)
#define TEST_UHASHTABLE_GETBURST_ID(X)    (500 + X)
#define TEST_UHASHTABLE_GET_ID(X)         (1100 + X)
#define TEST_UHASHTABLE_REMOVEBURST_ID(X) (1200 + X)
#define TEST_UHASHTABLE_REMOVE_ID(X)      (1300 + X)
#define TEST_UHASHTABLE_DESTROY_ID(X)     (1400 + X)

/** @brief Interrupt test */
#define TEST_INTERRUPT(X) (X)

/** @brief Critical tests  */
#define CRITICAL_TEST_CREATE_THREAD_ID            0
#define TEST_CRITICAL_CREATE_THREADS_LOCAL(X)     (100 + X)
#define TEST_CRITICAL_JOIN_THREADS_LOCAL(X)       (200 + X)
#define TEST_CRITICAL_CREATE_THREADS_GLOBAL0(X)   (300 + X)
#define TEST_CRITICAL_JOIN_THREADS_GLOBAL0(X)     (400 + X)
#define TEST_CRITICAL_CREATE_THREADS_GLOBAL1(X)   (500 + X)
#define TEST_CRITICAL_JOIN_THREADS_GLOBAL1(X)     (600 + X)
#define TEST_CRITICAL_CREATE_THREADS_SPINLOCK(X)  (700 + X)
#define TEST_ATOMICS_JOIN_THREADS_SPINLOCK(X)     (800 + X)
#define TEST_CRITICAL_CREATE_THREADS_INCREMENT(X) (900 + X)
#define TEST_ATOMICS_JOIN_THREADS_INCREMENT(X)    (1000 + X)
#define TEST_CRITICAL_CREATE_THREADS_DECREMENT(X) (1100 + X)
#define TEST_ATOMICS_JOIN_THREADS_DECREMENT(X)    (1200 + X)
#define TEST_CRITICAL_VALUE_LOCAL                 1300
#define TEST_CRITICAL_VALUE_GLOBAL0               1301
#define TEST_CRITICAL_VALUE_GLOBAL1               1302
#define TEST_CRITICAL_VALUE_SPINLOCK              1303
#define TEST_CRITICAL_VALUE_INCREMENT             1304
#define TEST_CRITICAL_VALUE_DECREMENT             1305

/** @brief FDT tests */
#define TEST_DEVTREE_PARSE          0
#define TEST_DEVTREE_GETPROP0       1
#define TEST_DEVTREE_GETPROP1       2
#define TEST_DEVTREE_GETFIRSTPROP0  3
#define TEST_DEVTREE_GETFIRSTPROP1  4
#define TEST_DEVTREE_GETNEXTPROP0   5
#define TEST_DEVTREE_GETNEXTPROP1   6
#define TEST_DEVTREE_GETNEXTPROP2   7
#define TEST_DEVTREE_GETNEXTPROP3   8
#define TEST_DEVTREE_GETNEXTPROP4   9
#define TEST_DEVTREE_GETCHILD0      10
#define TEST_DEVTREE_GETCHILD1      11
#define TEST_DEVTREE_GETCHILD2      12
#define TEST_DEVTREE_GETCHILD3      13
#define TEST_DEVTREE_GETCHILD4      14
#define TEST_DEVTREE_GETNEXTNODE0   15
#define TEST_DEVTREE_GETNEXTNODE1   16
#define TEST_DEVTREE_GETNEXTNODE2   17
#define TEST_DEVTREE_GETNEXTNODE3   18
#define TEST_DEVTREE_GETNEXTNODE4   19
#define TEST_DEVTREE_GETNEXTNODE5   20
#define TEST_DEVTREE_GETNEXTNODE6   21
#define TEST_DEVTREE_GETNODEBYNAME0 22
#define TEST_DEVTREE_GETNODEBYNAME1 23
#define TEST_DEVTREE_GETHANDLE0     24
#define TEST_DEVTREE_GETHANDLE1     25
#define TEST_DEVTREE_GETHANDLE2     26
#define TEST_DEVTREE_GETMEMORY0     27
#define TEST_DEVTREE_GETMEMORY1     28
#define TEST_DEVTREE_GETMEMORY2     29
#define TEST_DEVTREE_GETMEMORY3     30
#define TEST_DEVTREE_GETRESMEMORY0  31
#define TEST_DEVTREE_GETRESMEMORY1  32
#define TEST_DEVTREE_GETRESMEMORY2  33
#define TEST_DEVTREE_GETRESMEMORY3  34
#define TEST_DEVTREE_GETNODEBYNAME  35

/** @brief Kernel heap tests */
#define TEST_KHEAP_NO_FREE_ALIGN(X) (0 + X)
#define TEST_KHEAP_NO_FREE_RANGE(X) (100 + X)
#define TEST_KHEAP_NO_FREE_ALLOC(X) (200 + X)
#define TEST_KHEAP_FREE_ALIGN(X)    (300 + X)
#define TEST_KHEAP_FREE_RANGE(X)    (400 + X)
#define TEST_KHEAP_FREE_ALLOC(X)    (500 + X)

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
/** @brief UHashtable test function */
void UHashtableTest(void);
/** @brief Interrupts test function */
void InterruptsTest(void);
/** @brief Critical test function */
void CriticalTest(void);
/** @brief Device Tree test function */
void DeviceTreeTest(void);
/** @brief Kernel heap test function */
void KernelHeapTest(void);

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/