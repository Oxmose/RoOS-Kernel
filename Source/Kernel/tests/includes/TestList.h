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
#define TEST_OS_UHASHTABLE_ENABLED                0
#define TEST_SCHEDULER_ENABLED                    1
#define TEST_VFS_ENABLED                          0
#define TEST_OS_VECTOR_ENABLED                    0
#define TEST_CRITICAL_ENABLED                     0

/*************************************************
 * TEST IDENTIFIERS
 ************************************************/

/** @brief Panic test */
#define PANIC_TEST_SUCCESS_ID 0

/** @brief Scheduler test */
#define SCHED_TEST_IS_INIT_ID                   0
#define SCHED_TEST_CREATE_TEST_THREAD_ID(X)     (100 + X)
#define SCHED_TEST_SLEEP_ID(X)                  (200 + X)
#define SCHED_TEST_JOIN_THREAD_ID(X)            (300 + X)
#define SCHED_TEST_GETTER_ID(X)                 (400 + X)
#define SCHED_TEST_ERROR_THREAD_ID(X)           (500 + X)
#define SCHED_TEST_PRIORITY_THREAD_CREATE_ID(X) (600 + X)
#define SCHED_TEST_PRIORITY_THREAD_JOIN_ID(X)   (700 + X)
#define SCHED_TEST_PRIORITY_THREAD_CHECK_ID(X)  (900 + X)
#define SCHED_TEST_MAPPING_THREAD_CREATE_ID(X)  (1000 + X)
#define SCHED_TEST_MAPPING_THREAD_JOIN_ID(X)    (1100 + X)
#define SCHED_TEST_MAPPING_THREAD_CHECK_ID(X)   (1200 + X)

/** @brief UHashtable test */
#define TEST_UHASHTABLE_CREATE_ID(X)      (X)
#define TEST_UHASHTABLE_SETBURST_ID(X)    (100 + X)
#define TEST_UHASHTABLE_SET_ID(X)         (400 + X)
#define TEST_UHASHTABLE_GETBURST_ID(X)    (500 + X)
#define TEST_UHASHTABLE_GET_ID(X)         (1100 + X)
#define TEST_UHASHTABLE_REMOVEBURST_ID(X) (1200 + X)
#define TEST_UHASHTABLE_REMOVE_ID(X)      (1300 + X)
#define TEST_UHASHTABLE_DESTROY_ID(X)     (1400 + X)

/** @brief Vector test */
#define TEST_VECTOR_CREATE_ID(X)      (X)
#define TEST_VECTOR_GET_ID(X)         (100 + X)
#define TEST_VECTOR_INSERT_ID(X)      (200 + X)
#define TEST_VECTOR_POP_ID(X)         (300 + X)
#define TEST_VECTOR_RESIZE_ID(X)      (400 + X)
#define TEST_VECTOR_SHRINK_ID(X)      (500 + X)
#define TEST_VECTOR_COPY_ID(X)        (600 + X)
#define TEST_VECTOR_CLEAR_ID(X)       (700 + X)
#define TEST_VECTOR_DESTROY_ID(X)     (800 + X)
#define TEST_VECTOR_PUSHBURST_ID(X)   (900 + X)
#define TEST_VECTOR_GETBURST_ID(X)    (1000 + X)
#define TEST_VECTOR_SETBURST_ID(X)    (2000 + X)
#define TEST_VECTOR_INSERTBURST_ID(X) (2100 + X)
#define TEST_VECTOR_POPBURST_ID(X)    (2200 + X)

/** @brief VFS test */
#define TEST_VFS_CLEAN_PATH(X)       (X)
#define TEST_VFS_NEXT_TOKEN(X)       (100 + X)
#define TEST_VFS_FIND_NODE(X)        (200 + X)
#define TEST_VFS_ADD_NODE(X)         (300 + X)
#define TEST_VFS_REGISTER_DRIVER(X)  (400 + X)
#define TEST_VFS_REMOVE_NODE(X)      (500 + X)
#define TEST_VFS_FD_CREATE(X)        (600 + X)
#define TEST_VFS_FD_DESTROY(X)       (700 + X)
#define TEST_VFS_GENERIC(X)          (800 + X)
#define TEST_VFS_OPEN(X)             (900 + X)
#define TEST_VFS_CLOSE(X)            (1000 + X)
#define TEST_VFS_READ(X)             (1100 + X)
#define TEST_VFS_WRITE(X)            (1200 + X)
#define TEST_VFS_READDIR(X)          (1300 + X)
#define TEST_VFS_IOCTL(X)            (1400 + X)
#define TEST_VFS_MOUNT(X)            (1500 + X)
#define TEST_VFS_UNMOUNT(X)          (1600 + X)
#define TEST_VFS_REMOVE_DRIVER(X)    (1700 + X)
#define TEST_VFS_FD_TABLE_CREATE(X)  (1800 + X)
#define TEST_VFS_FD_TABLE_DESTROY(X) (1900 + X)

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

/** @brief Vector test function */
void VectorTest(void);

/** @brief VFS test functions */
void VFSCleanPathTest(void* pArgs);
void VFSFindNodeTest(void* pArgs);
void VFSAddNodeTest(void* pArgs);
void VFSRemoveNodeTest(void* pArgs);
void VFSRegDriverTest(void* pArgs);
void VFSRemoveDriverTest(void* pArgs);
void VFSCreateFDTest(void* pArgs);
void VFSDestroyFDTest(void* pArgs);
void VirtualFSTest(void);

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/