
/*******************************************************************************
 * @file KMutexText.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework kernel mutex testing.
 *
 * @details Testing framework mutex testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <string.h>
#include <stddef.h>
#include <Scheduler.h>
#include <KernelError.h>
#include <KernelMutex.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestFramework.h>


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
static S_KernelThread *spTestThread;
static volatile uint64_t sMutualExcValue = 0;
static volatile int32_t sLastTid = 0;
static volatile int32_t sOrderedTid = 0;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void* _TestRoutine(void* args);
static void* _TestMutualExclusionRoutine(void* args);
static void* _TestPrioMutexRoutine(void* args);
static void* _TestFIFOMutexRoutine(void* args);
static void* _TestTrylockMutexRoutine(void* args);
static void* _TestElevationMutexRoutine(void* args);

static void _TestMutualExclusion(void);
static void _TestRecursiveMutex(void);
static void _TestPrioMutex(void);
static void _TestFIFOMutex(void);
static void _TestTrylockMutex(void);
static void _TestElevationMutex(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _TestMutualExclusionRoutine(void* args)
{
  S_KernelMutex*  pMutex;
  uint32_t        i;
  uint32_t        j;
  E_Return        error;
  S_KernelThread *pThread;

  pMutex = (S_KernelMutex*)args;
  pThread = SchedulerGetCurrentThread();

  error = NO_ERROR;
  for (i = 0; i < 100; ++i)
  {
    error |= KernelMutexLock(pMutex);
    for (j = 0; j < 10000; ++j)
    {
      ++sMutualExcValue;
    }
    error |= KernelMutexUnlock(pMutex);
  }

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_EXC_ID(400 + pThread->tid),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  return (void*)(uintptr_t)pThread->tid;
}

static void* _TestPrioMutexRoutine(void* args)
{
  S_KernelMutex*  pMutex;
  S_KernelThread* pThread;
  int32_t         getTid;
  E_Return        error0;
  E_Return        error1;

  pMutex = (S_KernelMutex*)args;
  pThread = SchedulerGetCurrentThread();

  error0 = KernelMutexLock(pMutex);
  getTid = sLastTid;
  sLastTid = pThread->tid;
  KPrintf("%d returned from core %d\n", pThread->tid, CPUGetId());
  error1 = KernelMutexUnlock(pMutex);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(pThread->tid * 10 + 200),
                          error0 == NO_ERROR,
                          NO_ERROR,
                          error0,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(pThread->tid * 10 + 201),
                          error1 == NO_ERROR,
                          NO_ERROR,
                          error1,
                          TEST_OS_KMUTEX_ENABLED);

  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_ORDER_TEST(pThread->tid * 10 + 202),
                          getTid == pThread->tid + 1,
                          pThread->tid + 1,
                          getTid,
                          TEST_OS_KMUTEX_ENABLED);

  return (void*)(uintptr_t)pThread->tid;
}

static void* _TestFIFOMutexRoutine(void* args)
{
  S_KernelMutex*  pMutex;
  S_KernelThread* pThread;
  E_Return        error0;
  E_Return        error1;

  pMutex = (S_KernelMutex*)args;
  pThread = SchedulerGetCurrentThread();

  error0 = KernelMutexLock(pMutex);
  if(sLastTid == pThread->tid + 1)
  {
      ++sOrderedTid;
  }
  sLastTid = pThread->tid;
  KPrintf("%d returned from core %d\n", pThread->tid, CPUGetId());
  error1 = KernelMutexUnlock(pMutex);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(pThread->tid * 10 + 200),
                          error0 == NO_ERROR,
                          NO_ERROR,
                          error0,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(pThread->tid * 10 + 201),
                          error1 == NO_ERROR,
                          NO_ERROR,
                          error1,
                          TEST_OS_KMUTEX_ENABLED);



  return (void*)(uintptr_t)pThread->tid;
}

static void* _TestTrylockMutexRoutine(void* args)
{
  uint32_t       tid;
  int32_t        level;
  E_Return       errorTry = NO_ERROR;
  E_Return       error0 = NO_ERROR;
  E_Return       error1 = NO_ERROR;
  uint32_t       initBase;
  S_KernelMutex* pMutexes;

  tid = SchedulerGetCurrentThread()->tid;

  pMutexes = (S_KernelMutex*)args;

  initBase = KERNEL_LOWEST_PRIORITY / 2;

  error0 = KernelMutexLock(&pMutexes[1]);
  errorTry = KernelMutexTryLock(&pMutexes[0], &level);
  if(tid > initBase)
  {
      KernelMutexUnlock(&pMutexes[0]);
  }
  error1 = KernelMutexUnlock(&pMutexes[1]);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(tid * 10),
                          error0 == NO_ERROR,
                          NO_ERROR,
                          error0,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(tid * 10 + 1),
                          error1 == NO_ERROR,
                          NO_ERROR,
                          error1,
                          TEST_OS_KMUTEX_ENABLED);

  if(tid < initBase)
  {
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(tid * 10 + 2),
                            errorTry == ERR_UNAUTHORIZED_ACTION,
                            ERR_UNAUTHORIZED_ACTION,
                            errorTry,
                            TEST_OS_KMUTEX_ENABLED);

    TEST_POINT_ASSERT_UINT(TEST_KMUTEX_TRYLOCK_TEST(tid * 10 + 3),
                            level == 0,
                            0,
                            level,
                            TEST_OS_KMUTEX_ENABLED);
  }
  else
  {
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(tid * 10 + 4),
                            errorTry == NO_ERROR,
                            NO_ERROR,
                            errorTry,
                            TEST_OS_KMUTEX_ENABLED);

    TEST_POINT_ASSERT_INT(TEST_KMUTEX_TRYLOCK_TEST(tid * 10 + 5),
                          level == 1,
                          1,
                          level,
                          TEST_OS_KMUTEX_ENABLED);
  }
  return NULL;
}

static void* _TestElevationMutexRoutine(void* args)
{
  uintptr_t prio;
  E_Return error;
  S_KernelThread* pCurThread;
  S_KernelMutex*  pElevationMutex;

  pElevationMutex = (S_KernelMutex*)args;
  pCurThread = SchedulerGetCurrentThread();
  prio = pCurThread->priority;


  if(prio == 10)
  {
    error = KernelMutexLock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(0),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    while(pElevationMutex->pWaitingList->size == 0){

    }
    SleepNs(1000000);

    KPrintf("New thread waiting and prio is %d\n", pCurThread->priority);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(1),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);

    while(pElevationMutex->pWaitingList->size == 1){

    }
    SleepNs(1000000);

    KPrintf("New thread waiting and prio is %d\n", pCurThread->priority);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(2),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);

    while(pElevationMutex->pWaitingList->size == 2){

    }
    SleepNs(1000000);

    KPrintf("New thread waiting and prio is %d\n", pCurThread->priority);
    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(3),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);



    error = KernelMutexUnlock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(4),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked the mutex and prio is %d\n", pCurThread->priority);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(5),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);
  }
  else if(prio == 12)
  {
    SleepNs(200000000);

    error = KernelMutexLock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(6),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(7),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked thread and prio is %d\n", pCurThread->priority);

    error = KernelMutexUnlock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(8),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(9),
                            pCurThread->priority == 12,
                            12,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);
  }
  else if(prio == 9)
  {
    SleepNs(6000000000);

    error = KernelMutexLock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(10),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(11),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked thread and prio is %d\n", pCurThread->priority);

    error = KernelMutexUnlock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(12),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(13),
                            pCurThread->priority == 9,
                            9,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);
  }
  else if(prio == 7)
  {
    SleepNs(4000000000);

    error = KernelMutexLock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(14),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(15),
                            pCurThread->priority == 10,
                            10,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked thread and prio is %d\n", pCurThread->priority);

    error = KernelMutexUnlock(pElevationMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(16),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);

    KPrintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

    TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(17),
                            pCurThread->priority == 7,
                            7,
                            pCurThread->priority,
                            TEST_OS_KMUTEX_ENABLED);
  }
  else
  {
    KPrintf("Unsupported test priority\n");
  }

  return NULL;
}

static void _TestMutualExclusion(void)
{
  S_KernelThread* testThread[100];
  S_KernelMutex   mutex;
  E_Return        error;
  uint32_t        i;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD\0";
  void*           retVal;
  uint32_t        tid;

  /* Create the mutex */
  error = KernelMutexInit(&mutex, KMUTEX_FLAG_QUEUING_FIFO);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_EXC_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  error = KernelMutexLock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_EXC_ID(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  /* Create the threads */
  for (i = 0; i < 100; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&testThread[i],
                          true,
                          10,
                          name,
                          0x1000,
                          cpuMask,
                          _TestMutualExclusionRoutine,
                          &mutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_EXC_ID(2 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);
  }

  error = KernelMutexUnlock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_EXC_ID(102),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  /* Join the threads */
  for (i = 0; i < 100; ++i)
  {
    tid = (uint32_t)testThread[i]->tid;
    error = JoinThread(testThread[i], &retVal);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_EXC_ID(103 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_KMUTEX_EXC_ID(203 + i),
                            (uint32_t)(uintptr_t)retVal == tid,
                            (uint32_t)tid,
                            (uint32_t)(uintptr_t)retVal,
                            TEST_OS_KMUTEX_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_KMUTEX_EXC_ID(600),
                           sMutualExcValue == 100000000,
                           100000000,
                           sMutualExcValue,
                           TEST_OS_KMUTEX_ENABLED);
}

static void _TestRecursiveMutex(void)
{
  S_KernelMutex mutex;
  E_Return      error;

  /* Create the mutex */
  error = KernelMutexInit(&mutex,
                          KMUTEX_FLAG_QUEUING_FIFO | KMUTEX_FLAG_RECURSIVE);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(1),
                          mutex.level == 0,
                          0,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);

  /* Lock the mutex recursively */
  error = KernelMutexLock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(2),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(3),
                          mutex.level == 1,
                          1,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);
  error = KernelMutexLock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(4),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(5),
                          mutex.level == 2,
                          2,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);
  error = KernelMutexLock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(6),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(7),
                          mutex.level == 3,
                          3,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);

  /* Check max recursion */
  mutex.level = UINT32_MAX;
  error = KernelMutexLock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(8),
                          error == ERR_EXCEEDED_LIMIT,
                          ERR_EXCEEDED_LIMIT,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(9),
                          mutex.level == UINT32_MAX,
                          UINT32_MAX,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);
  mutex.level = 3;
  /* Unlock the mutex recursively */
  error = KernelMutexUnlock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(10),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(11),
                          mutex.level == 2,
                          2,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);
  error = KernelMutexUnlock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(12),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(13),
                          mutex.level == 1,
                          1,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);
  error = KernelMutexUnlock(&mutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_RECURSIVE_ID(14),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_RECURSIVE_ID(15),
                          mutex.level == 0,
                          0,
                          mutex.level,
                          TEST_OS_KMUTEX_ENABLED);
}

static void _TestPrioMutex(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[KERNEL_LOWEST_PRIORITY + 1];
  S_KernelMutex   orderMutex;
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "MUTEX_ORDER_TEST\0";

  error = KernelMutexInit(&orderMutex, KMUTEX_FLAG_QUEUING_PRIO);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  error = KernelMutexLock(&orderMutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&pThreads[i],
                         true,
                         KERNEL_LOWEST_PRIORITY - i,
                         name,
                         0x1000,
                         cpuMask,
                         _TestPrioMutexRoutine,
                          (void*)&orderMutex);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(2 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);
  }
  sLastTid = pThreads[i - 1]->tid + 1;

  SleepNs(500000000);
  /* Give mutex */
  error = KernelMutexUnlock(&orderMutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(100),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);
      TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ORDER_TEST(101 + i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_OS_KMUTEX_ENABLED);
  }
}

static void _TestFIFOMutex(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[KERNEL_LOWEST_PRIORITY + 1];
  S_KernelMutex   orderMutex;
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "MUTEX_FIFO_TEST\0";

  error = KernelMutexInit(&orderMutex, KMUTEX_FLAG_QUEUING_FIFO);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  error = KernelMutexLock(&orderMutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&pThreads[i],
                         true,
                         KERNEL_LOWEST_PRIORITY - i,
                         name,
                         0x1000,
                         cpuMask,
                         _TestFIFOMutexRoutine,
                          (void*)&orderMutex);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(2 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);
  }
  sLastTid = pThreads[i - 1]->tid + 1;
  sOrderedTid = 0;

  SleepNs(500000000);
  /* Give mutex */
  error = KernelMutexUnlock(&orderMutex);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(100),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);
      TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_TEST(101 + i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_OS_KMUTEX_ENABLED);
  }

  KPrintf("Returned with %d in a row\n", sOrderedTid);

  TEST_POINT_ASSERT_UINT(TEST_KMUTEX_FIFO_TEST(999),
                         sOrderedTid != KERNEL_LOWEST_PRIORITY + 1,
                         0,
                         sOrderedTid,
                         TEST_OS_KMUTEX_ENABLED);
}

static void _TestTrylockMutex(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[KERNEL_LOWEST_PRIORITY + 1];
  S_KernelMutex   mutexes[2];
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "MUTEX_TRYLOCK_TEST\0";

  error = KernelMutexInit(&mutexes[0], KMUTEX_FLAG_QUEUING_FIFO);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  error = KernelMutexInit(&mutexes[1], KMUTEX_FLAG_QUEUING_PRIO);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  error = KernelMutexLock(&mutexes[1]);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(2),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&pThreads[i],
                         true,
                         KERNEL_LOWEST_PRIORITY - i,
                         name,
                         0x1000,
                         cpuMask,
                         _TestTrylockMutexRoutine,
                         (void*)mutexes);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(3 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);
  }

  SleepNs(500000000);
  /* Give mutex */
  error = KernelMutexUnlock(&mutexes[1]);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(100),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);
      TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TEST(101 + i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_OS_KMUTEX_ENABLED);
  }
}

static void _TestElevationMutex(void)
{
  S_KernelMutex mutex;
  uint32_t i;
  E_Return error;
  S_KernelThread* pThreads[4];
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "MUTEX_ELEVATION_TEST\0";

  error = KernelMutexInit(&mutex,
                          KMUTEX_FLAG_PRIO_ELEVATION | KMUTEX_FLAG_PRIORITY(10) |
                          KMUTEX_FLAG_QUEUING_PRIO);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(18),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0 % CPUGetCount());
  error = CreateThread(&pThreads[0],
                       true,
                       10,
                       name,
                       0x1000,
                       cpuMask,
                       _TestElevationMutexRoutine,
                       (void*)&mutex);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(19),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 1 % CPUGetCount());
  error = CreateThread(&pThreads[1],
                            true,
                            12,
                            name,
                            0x1000,
                            cpuMask,
                            _TestElevationMutexRoutine,
                            (void*)&mutex);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(20),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 2 % CPUGetCount());
  error = CreateThread(&pThreads[2],
                            true,
                            9,
                            name,
                            0x1000,
                            cpuMask,
                            _TestElevationMutexRoutine,
                            (void*)&mutex);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(21),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 3 % CPUGetCount());
  error = CreateThread(&pThreads[3],
                            true,
                            7,
                            name,
                            0x1000,
                            cpuMask,
                            _TestElevationMutexRoutine,
                            (void*)&mutex);

  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(22),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);

  for(i = 0; i < 4; ++i)
  {
    error = JoinThread(pThreads[i], &retVal);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_ELEVATION_PRIO(23 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KMUTEX_ENABLED);
  }
}

static void* _TestRoutine(void* args)
{
  TEST_POINT_ASSERT_POINTER(TEST_KMUTEX_CREATE_TEST(2),
                            args == (void*)0xDEADC0DEBEEF0000ULL,
                            0xDEADC0DEBEEF0000ULL,
                            (uintptr_t)args,
                            TEST_OS_KMUTEX_ENABLED);
#if 1
  KPrintf("Mutual Exclusion Test\n");
  _TestMutualExclusion();
  KPrintf("Try Lock Test\n");
  _TestTrylockMutex();
  KPrintf("Recursive Test\n");
  _TestRecursiveMutex();
  KPrintf("Elevation Test\n");
  _TestElevationMutex();
  KPrintf("Priority Ordering Test\n");
  _TestPrioMutex();
  KPrintf("FIFO Ordering Test\n");
  _TestFIFOMutex();
#else
  (void)_TestMutualExclusion;
  (void)_TestRecursiveMutex;
  (void)_TestPrioMutex;
  (void)_TestFIFOMutex;
  (void)_TestTrylockMutex;
  _TestElevationMutex();
#endif

  TEST_FRAMEWORK_END();

  return NULL;
}

void KernelMutexTest(void)
{
  /* Create the test thread */
  E_Return  error;
  S_CPUMask cpuMask;
  char      name[32] = "TEST_THREAD\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  /* Create the test thread */
  spTestThread = NULL;
  error = CreateThread(&spTestThread,
                       true,
                       0,
                       name,
                       0x1000,
                       cpuMask,
                       _TestRoutine,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KMUTEX_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KMUTEX_CREATE_TEST(1),
                            spTestThread != NULL,
                            0xFFFFFFFFFFFFFFFFULL,
                            (uintptr_t)spTestThread,
                            TEST_OS_KMUTEX_ENABLED);

  if (error != NO_ERROR)
  {
    TEST_FRAMEWORK_END();
  }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/