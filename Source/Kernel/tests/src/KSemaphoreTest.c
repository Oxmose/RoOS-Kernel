
/*******************************************************************************
 * @file KSemaphoreTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework kernel semaphore testing.
 *
 * @details Testing framework semaphore testing.
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
#include <KernelOutput.h>
#include <KernelSemaphore.h>

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
static void* _TestPrioSemaphoreRoutine(void* args);
static void* _TestFIFOSemaphoreRoutine(void* args);
static void* _TestTryWaitSemaphoreRoutine(void* args);

static void _TestMutualExclusion(void);
static void _TestPrioSemaphore(void);
static void _TestFIFOSemaphore(void);
static void _TestTryWaitSemaphore(void);
static void _TestMultiplePostSemaphore(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _TestMutualExclusionRoutine(void* args)
{
  S_KernelSemaphore *pSemaphore;
  uint32_t        i;
  uint32_t        j;
  E_Return        error;
  S_KernelThread *pThread;

  pSemaphore = (S_KernelSemaphore *)args;
  pThread = SchedulerGetCurrentThread();

  error = NO_ERROR;
  for (i = 0; i < 100; ++i)
  {
    error |= KernelSemaphoreWait(pSemaphore);
    for (j = 0; j < 10000; ++j)
    {
      ++sMutualExcValue;
    }
    error |= KernelSemaphorePost(pSemaphore);
  }

  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_EXC_ID(400 + pThread->tid),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  return (void*)(uintptr_t)pThread->tid;
}

static void* _TestPrioSemaphoreRoutine(void* args)
{
  S_KernelSemaphore*  pSemaphore;
  S_KernelThread* pThread;
  int32_t         getTid;
  E_Return        error0;
  E_Return        error1;

  pSemaphore = (S_KernelSemaphore*)args;
  pThread = SchedulerGetCurrentThread();

  error0 = KernelSemaphoreWait(pSemaphore);
  getTid = sLastTid;
  sLastTid = pThread->tid;
  KPrintf("%d returned from core %d\n", pThread->tid, CPUGetId());
  error1 = KernelSemaphorePost(pSemaphore);

  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(pThread->tid * 10 + 200),
                          error0 == NO_ERROR,
                          NO_ERROR,
                          error0,
                          TEST_OS_KSEMAPHORE_ENABLED);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(pThread->tid * 10 + 201),
                          error1 == NO_ERROR,
                          NO_ERROR,
                          error1,
                          TEST_OS_KSEMAPHORE_ENABLED);

  TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_ORDER_TEST(pThread->tid * 10 + 202),
                          getTid == pThread->tid + 1,
                          pThread->tid + 1,
                          getTid,
                          TEST_OS_KSEMAPHORE_ENABLED);

  return (void*)(uintptr_t)pThread->tid;
}

static void* _TestFIFOSemaphoreRoutine(void* args)
{
  S_KernelSemaphore*  pSemaphore;
  S_KernelThread* pThread;
  E_Return        error0;
  E_Return        error1;

  pSemaphore = (S_KernelSemaphore*)args;
  pThread = SchedulerGetCurrentThread();

  error0 = KernelSemaphoreWait(pSemaphore);
  if (sLastTid == pThread->tid + 1)
  {
      ++sOrderedTid;
  }
  sLastTid = pThread->tid;
  KPrintf("%d returned from core %d\n", pThread->tid, CPUGetId());
  error1 = KernelSemaphorePost(pSemaphore);

  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(pThread->tid * 10 + 200),
                          error0 == NO_ERROR,
                          NO_ERROR,
                          error0,
                          TEST_OS_KSEMAPHORE_ENABLED);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(pThread->tid * 10 + 201),
                          error1 == NO_ERROR,
                          NO_ERROR,
                          error1,
                          TEST_OS_KSEMAPHORE_ENABLED);



  return (void*)(uintptr_t)pThread->tid;
}

static void* _TestTryWaitSemaphoreRoutine(void* args)
{
  uint32_t       tid;
  int32_t        level;
  E_Return       errorTry = NO_ERROR;
  E_Return       error0 = NO_ERROR;
  E_Return       error1 = NO_ERROR;
  uint32_t       initBase;
  S_KernelSemaphore* pSemaphorees;

  tid = SchedulerGetCurrentThread()->tid;

  pSemaphorees = (S_KernelSemaphore*)args;

  initBase = KERNEL_LOWEST_PRIORITY / 2;

  error0 = KernelSemaphoreWait(&pSemaphorees[1]);
  errorTry = KernelSemaphoreTryWait(&pSemaphorees[0], &level);
  if (tid > initBase)
  {
      KernelSemaphorePost(&pSemaphorees[0]);
  }
  error1 = KernelSemaphorePost(&pSemaphorees[1]);

  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(tid * 10),
                          error0 == NO_ERROR,
                          NO_ERROR,
                          error0,
                          TEST_OS_KSEMAPHORE_ENABLED);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(tid * 10 + 1),
                          error1 == NO_ERROR,
                          NO_ERROR,
                          error1,
                          TEST_OS_KSEMAPHORE_ENABLED);

  if (tid < initBase)
  {
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(tid * 10 + 2),
                            errorTry == ERR_UNAUTHORIZED_ACTION,
                            ERR_UNAUTHORIZED_ACTION,
                            errorTry,
                            TEST_OS_KSEMAPHORE_ENABLED);

    TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_TRYWAIT_TEST(tid * 10 + 3),
                            level == 0,
                            0,
                            level,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }
  else
  {
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(tid * 10 + 4),
                            errorTry == NO_ERROR,
                            NO_ERROR,
                            errorTry,
                            TEST_OS_KSEMAPHORE_ENABLED);

    TEST_POINT_ASSERT_INT(TEST_KSEMAPHORE_TRYWAIT_TEST(tid * 10 + 5),
                          level == 1,
                          1,
                          level,
                          TEST_OS_KSEMAPHORE_ENABLED);
  }
  return NULL;
}

static void _TestMutualExclusion(void)
{
  S_KernelThread* testThread[100];
  S_KernelSemaphore   semaphore;
  E_Return        error;
  uint32_t        i;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD\0";
  void*           retVal;
  uint32_t        tid;

  /* Create the semaphore */
  error = KernelSemaphoreInit(&semaphore, KSEMAPHORE_FLAG_QUEUING_FIFO, 0);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_EXC_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

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
                          &semaphore);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_EXC_ID(2 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }

  error = KernelSemaphorePost(&semaphore);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_EXC_ID(102),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  /* Join the threads */
  for (i = 0; i < 100; ++i)
  {
    tid = (uint32_t)testThread[i]->tid;
    error = JoinThread(testThread[i], &retVal);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_EXC_ID(103 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_EXC_ID(203 + i),
                            (uint32_t)(uintptr_t)retVal == tid,
                            (uint32_t)tid,
                            (uint32_t)(uintptr_t)retVal,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_KSEMAPHORE_EXC_ID(600),
                           sMutualExcValue == 100000000,
                           100000000,
                           sMutualExcValue,
                           TEST_OS_KSEMAPHORE_ENABLED);
}

static void _TestPrioSemaphore(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[KERNEL_LOWEST_PRIORITY + 1];
  S_KernelSemaphore   orderSemaphore;
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "SEMAPHORE_ORDER_TEST\0";

  error = KernelSemaphoreInit(&orderSemaphore, KSEMAPHORE_FLAG_QUEUING_PRIO, 1);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  error = KernelSemaphoreWait(&orderSemaphore);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&pThreads[i],
                         true,
                         KERNEL_LOWEST_PRIORITY - i,
                         name,
                         0x1000,
                         cpuMask,
                         _TestPrioSemaphoreRoutine,
                          (void*)&orderSemaphore);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(2 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }
  sLastTid = pThreads[i - 1]->tid + 1;

  SleepNs(500000000);
  /* Give Semaphore */
  error = KernelSemaphorePost(&orderSemaphore);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(100),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);
      TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_ORDER_TEST(101 + i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_OS_KSEMAPHORE_ENABLED);
  }
}

static void _TestFIFOSemaphore(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[KERNEL_LOWEST_PRIORITY + 1];
  S_KernelSemaphore   orderSemaphore;
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "SEMAPHORE_FIFO_TEST\0";

  error = KernelSemaphoreInit(&orderSemaphore, KSEMAPHORE_FLAG_QUEUING_FIFO, 1);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  error = KernelSemaphoreWait(&orderSemaphore);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&pThreads[i],
                         true,
                         KERNEL_LOWEST_PRIORITY - i,
                         name,
                         0x1000,
                         cpuMask,
                         _TestFIFOSemaphoreRoutine,
                          (void*)&orderSemaphore);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(2 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }
  sLastTid = pThreads[i - 1]->tid + 1;
  sOrderedTid = 0;

  SleepNs(500000000);
  /* Give Semaphore */
  error = KernelSemaphorePost(&orderSemaphore);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(100),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);
      TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_TEST(101 + i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_OS_KSEMAPHORE_ENABLED);
  }

  KPrintf("Returned with %d in a row\n", sOrderedTid);

  TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_FIFO_TEST(999),
                         sOrderedTid != KERNEL_LOWEST_PRIORITY + 1,
                         0,
                         sOrderedTid,
                         TEST_OS_KSEMAPHORE_ENABLED);
}

static void _TestTryWaitSemaphore(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[KERNEL_LOWEST_PRIORITY + 1];
  S_KernelSemaphore   semaphores[2];
  S_CPUMask       cpuMask;
  void*           retVal;
  char            name[32] = "SEMAPHORE_TRYWAIT_TEST\0";

  error = KernelSemaphoreInit(&semaphores[0], KSEMAPHORE_FLAG_QUEUING_FIFO, 1);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  error = KernelSemaphoreInit(&semaphores[1], KSEMAPHORE_FLAG_QUEUING_PRIO, 1);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);
  error = KernelSemaphoreWait(&semaphores[1]);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(2),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());

    error = CreateThread(&pThreads[i],
                         true,
                         KERNEL_LOWEST_PRIORITY - i,
                         name,
                         0x1000,
                         cpuMask,
                         _TestTryWaitSemaphoreRoutine,
                         (void*)semaphores);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(3 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }

  SleepNs(500000000);
  /* Give semaphore */
  error = KernelSemaphorePost(&semaphores[1]);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(100),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);
      TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TEST(101 + i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_OS_KSEMAPHORE_ENABLED);
  }
}

static void _TestMultiplePostSemaphore(void)
{
  uint32_t i;
  E_Return error;
  S_KernelSemaphore semMultiple;

  error = KernelSemaphoreInit(&semMultiple, KSEMAPHORE_FLAG_QUEUING_FIFO, 0);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_MULTIPLE_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  for (i = 0; i < 100; ++i)
  {
    error = KernelSemaphorePost(&semMultiple);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_MULTIPLE_TEST(1 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }

  for (i = 0; i < 100; ++i)
  {
    error = KernelSemaphoreWait(&semMultiple);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_MULTIPLE_TEST(101 + i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_KSEMAPHORE_ENABLED);
  }

  semMultiple.lockState = 0x7FFFFFFF;
  error = KernelSemaphorePost(&semMultiple);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_MULTIPLE_TEST(201),
                          error == ERR_EXCEEDED_LIMIT,
                          ERR_EXCEEDED_LIMIT,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);

  error = KernelSemaphoreDestroy(&semMultiple);
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_MULTIPLE_TEST(202),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);
}

static void* _TestRoutine(void* args)
{
  TEST_POINT_ASSERT_POINTER(TEST_KSEMAPHORE_CREATE_TEST(2),
                            args == (void*)0xDEADC0DEBEEF0000ULL,
                            0xDEADC0DEBEEF0000ULL,
                            (uintptr_t)args,
                            TEST_OS_KSEMAPHORE_ENABLED);
#if 1
  KPrintf("Mutual Exclusion Test\n");
  _TestMutualExclusion();
  KPrintf("Try Wait Test\n");
  _TestTryWaitSemaphore();
  KPrintf("Priority Ordering Test\n");
  _TestPrioSemaphore();
  KPrintf("FIFO Ordering Test\n");
  _TestFIFOSemaphore();
  KPrintf("Multiple PostTest\n");
  _TestMultiplePostSemaphore();
#else
  (void)_TestMutualExclusion;
  (void)_TestPrioSemaphore;
  (void)_TestFIFOSemaphore;
  (void)_TestTryWaitSemaphore;
  _TestMultiplePostSemaphore();
#endif

  TEST_FRAMEWORK_END();

  return NULL;
}

void KernelSemaphoreTest(void)
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
  TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_TEST(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_KSEMAPHORE_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KSEMAPHORE_CREATE_TEST(1),
                            spTestThread != NULL,
                            0xFFFFFFFFFFFFFFFFULL,
                            (uintptr_t)spTestThread,
                            TEST_OS_KSEMAPHORE_ENABLED);

  if (error != NO_ERROR)
  {
    TEST_FRAMEWORK_END();
  }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/