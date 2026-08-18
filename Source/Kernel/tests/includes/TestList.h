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
#define TEST_OS_KMUTEX_ENABLED                    0
#define TEST_OS_KSEMAPHORE_ENABLED                0
#define TEST_KHEAP_ENABLED                        0
#define TEST_DEVTREE_ENABLED                      0
#define TEST_CRITICAL_ENABLED                     0
#define TEST_INTERRUPT_ENABLED                    0
#define TEST_OS_UHASHTABLE_ENABLED                0
#define TEST_SCHEDULER_ENABLED                    1
#define TEST_VFS_ENABLED                          0
#define TEST_OS_VECTOR_ENABLED                    0
#define TEST_OS_KQUEUE_ENABLED                    0
#define TEST_OS_FAST_QUEUES_ENABLED               0
#define TEST_CPUID_ENABLED                        0

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

/** @brief Fast queue test */
#define TEST_FASTQUEUE_INIT              0
#define TEST_FASTQUEUE_CREATE_ID(X)     (1 + X)
#define TEST_FASTQUEUE_EMPTY_ID(X)      (100 + X)
#define TEST_FASTQUEUE_PUSH_ID(X)       (200 + X)
#define TEST_FASTQUEUE_POP_ID(X)        (300 + X)
#define TEST_FASTQUEUE_WRAP_ID(X)       (400 + X)
#define TEST_FASTQUEUE_CORNER_ID(X)     (500 + X)
#define TEST_FASTQUEUE_CONCURRENCY_ID(X)(600 + X)

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

/** @brief CPUID tests */
#define TEST_CPUID_VENDOR_ID        0
#define TEST_CPUID_NAME_ID          1
#define TEST_CPUID_FAMILY_ID        2
#define TEST_CPUID_LEVEL_ID         3
#define TEST_CPUID_FLAGS_ID         4

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
#define TEST_DEVTREE_NULL_INPUT0    36
#define TEST_DEVTREE_NULL_INPUT1    37
#define TEST_DEVTREE_NULL_INPUT2    38
#define TEST_DEVTREE_NULL_INPUT3    39
#define TEST_DEVTREE_NULL_INPUT4    40
#define TEST_DEVTREE_TREE_COUNT0    41
#define TEST_DEVTREE_TREE_COUNT1    42
#define TEST_DEVTREE_TREE_COUNT2    43
#define TEST_DEVTREE_TREE_PROP0     44
#define TEST_DEVTREE_TREE_PROP1     45
#define TEST_DEVTREE_TREE_NAME0     46
#define TEST_DEVTREE_TREE_NAME1     47
#define TEST_DEVTREE_TREE_NAME2     48
#define TEST_DEVTREE_TREE_HANDLE0   49
#define TEST_DEVTREE_TREE_HANDLE1   50
#define TEST_DEVTREE_TREE_HANDLE2   51
#define TEST_DEVTREE_TREE_MEM0      52
#define TEST_DEVTREE_TREE_MEM1      53
#define TEST_DEVTREE_TREE_MEM2      54
#define TEST_DEVTREE_TREE_RES0      55
#define TEST_DEVTREE_TREE_RES1      56
#define TEST_DEVTREE_TREE_RES2      57

/** @brief Kernel heap tests */
#define TEST_KHEAP_NO_FREE_ALIGN(X) (0 + X)
#define TEST_KHEAP_NO_FREE_RANGE(X) (100 + X)
#define TEST_KHEAP_NO_FREE_ALLOC(X) (200 + X)
#define TEST_KHEAP_FREE_ALIGN(X)    (300 + X)
#define TEST_KHEAP_FREE_RANGE(X)    (400 + X)
#define TEST_KHEAP_FREE_ALLOC(X)    (500 + X)

#define TEST_KQUEUE_CREATE_NODE0_ID             0
#define TEST_KQUEUE_CREATE_NODE1_ID             1
#define TEST_KQUEUE_DELETENode0_ID              2
#define TEST_KQUEUE_CREATE0_ID                  3
#define TEST_KQUEUE_CREATE1_ID                  4
#define TEST_KQUEUE_DELETE0_ID                  5
#define TEST_KQUEUE_DELETE1_ID                  6
#define TEST_KQUEUE_PUSH0_ID                    7
#define TEST_KQUEUE_POP0_ID                     8
#define TEST_KQUEUE_POP1_ID                     9
#define TEST_KQUEUE_CREATE_FIND0_ID             10
#define TEST_KQUEUE_CREATE_FIND1_ID             11
#define TEST_KQUEUE_CREATE_FIND2_ID             12
#define TEST_KQUEUE_SIZE0_ID                    13
#define TEST_KQUEUE_SIZE1_ID                    14
#define TEST_KQUEUE_CREATE_NODEBURST0_ID(IDVAL) (100 + IDVAL)
#define TEST_KQUEUE_CREATE_NODEBURST1_ID(IDVAL) \
    (TEST_KQUEUE_CREATE_NODEBURST0_ID(40) + IDVAL)
#define TEST_KQUEUE_PUSHPRIOBURST0_ID(IDVAL) \
    (TEST_KQUEUE_CREATE_NODEBURST1_ID(40) + IDVAL)
#define TEST_KQUEUE_PUSHBURST0_ID(IDVAL) \
    (TEST_KQUEUE_PUSHPRIOBURST0_ID(40) + IDVAL)
#define TEST_KQUEUE_POPBURST0_ID(IDVAL) \
    (TEST_KQUEUE_PUSHBURST0_ID(40) + IDVAL)
#define TEST_KQUEUE_POPBURST1_ID(IDVAL) \
    (TEST_KQUEUE_POPBURST0_ID(120) + IDVAL)
#define TEST_KQUEUE_DELETENODEBURST0_ID(IDVAL) \
    (TEST_KQUEUE_POPBURST1_ID(120) + IDVAL)
#define TEST_KQUEUE_DELETENODEBURST1_ID(IDVAL) \
    (TEST_KQUEUE_DELETENODEBURST0_ID(40) + IDVAL)

/** @brief Kernel mutex tests */
#define TEST_KMUTEX_CREATE_TEST(X)    (X)
#define TEST_KMUTEX_EXC_ID(X)         (1000 + X)
#define TEST_KMUTEX_RECURSIVE_ID(X)   (10000 + X)
#define TEST_KMUTEX_ORDER_TEST(X)     (100000 + X)
#define TEST_KMUTEX_FIFO_TEST(X)      (1000000 + X)
#define TEST_KMUTEX_TRYLOCK_TEST(X)   (10000000 + X)
#define TEST_KMUTEX_ELEVATION_PRIO(X) (100000000 + X)

/** @brief Kernel semaphore tests */
#define TEST_KSEMAPHORE_CREATE_TEST(X)    (X)
#define TEST_KSEMAPHORE_EXC_ID(X)         (1000 + X)
#define TEST_KSEMAPHORE_ORDER_TEST(X)     (100000 + X)
#define TEST_KSEMAPHORE_FIFO_TEST(X)      (1000000 + X)
#define TEST_KSEMAPHORE_TRYWAIT_TEST(X)   (10000000 + X)
#define TEST_KSEMAPHORE_MULTIPLE_TEST(X)  (100000000 + X)

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
/** @brief Vector test function */
void VectorTest(void);
/** @brief Fast queue test function */
void FastQueuesTest(void);
/** @brief KQueue test function */
void KQueuesTest(void);
/** @brief CPUID test function */
void CPUIDTest(void);
/** @brief Kernel mutex test function */
void KernelMutexTest(void);
/** @brief Kernel semaphore test function */
void KernelSemaphoreTest(void);
/** @brief VFS test functions */
void VFSCleanPathTest(void* pArgs);
void VFSFindNodeTest(void* pArgs);
void VFSAddNodeTest(void* pArgs);
void VFSRemoveNodeTest(void* pArgs);
void VFSRegDriverTest(void* pArgs);
void VFSRemoveDriverTest(void* pArgs);
void VFSCreateFDTest(void* pArgs);
void VFSDestroyFDTest(void* pArgs);
void VFSGenericTest(void* pArgs);
void VirtualFSTest(void);


#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/