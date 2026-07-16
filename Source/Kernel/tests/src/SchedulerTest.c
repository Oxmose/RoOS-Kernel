
/*******************************************************************************
 * @file SchedulerTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework scheduler testing.
 *
 * @details Testing framework scheduler testing.
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
#include <TimerManager.h>

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
static uint32_t sLastPrio;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void* TestRoutine(void* args);
static void* TestRoutineFail(void* args);
static void* TestJoinRoutine(void* args);
static void* TestMappingRoutine(void* args);
static void TestCreateInvalid(void);
static void TestCreateValid(void);
static void TestSleep(void);
static void TestJoin(void);
static void TestErrored(void);
static void TestGetters(void);
static void TestPriority(void);
static void TestMappedCore(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* TestRoutine(void* args)
{
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(17),
                            args == (void*)0xDEADC0DEBEEF0000ULL,
                            0xDEADC0DEBEEF0000ULL,
                            (uintptr_t)args,
                            TEST_SCHEDULER_ENABLED);

  TestSleep();
  TestJoin();
  TestErrored();
  TestGetters();
  TestPriority();
  TestMappedCore();

  TEST_FRAMEWORK_END();
  while (true){}

  return NULL;
}

static void* TestRoutineFail(void* args)
{
  TEST_POINT_ASSERT_UBYTE(SCHED_TEST_CREATE_TEST_THREAD_ID(16),
                          false,
                          false,
                          true,
                          TEST_SCHEDULER_ENABLED);
  (void)args;
  while (true){}

  return NULL;
}

static void* TestJoinRoutine(void* args)
{
  SleepNs((uint64_t)args);
  if ((uint64_t)args == 0)
  {
    return (void*)0xC0DE;
  }
  else
  {
    return (void*)0xD00D;
  }
}

static void* TestErroredRoutine(void* args)
{
  (void)args;
  SchedulerSetCurrentThreadErrored();

  TEST_POINT_ASSERT_UBYTE(SCHED_TEST_ERROR_THREAD_ID(3),
                          false,
                          false,
                          true,
                          TEST_SCHEDULER_ENABLED);

  return (void*)-1;
}

static void* TestPriorityRoutine(void* args)
{
  uint8_t priority;
  volatile uint32_t i;

  priority = (uint8_t)(uintptr_t)args;

  /* Perform work to wait */
  for (i = 0; i < 1000; ++i)
  {}

  if (priority != 0)
  {
    TEST_POINT_ASSERT_UBYTE(SCHED_TEST_PRIORITY_THREAD_CHECK_ID(priority),
                            priority == sLastPrio + 1,
                            priority,
                            sLastPrio + 1,
                            TEST_SCHEDULER_ENABLED);
    sLastPrio = priority;
  }

  return args;
}

static void* TestMappingRoutine(void* args)
{
  uint8_t cpuMapped;

  cpuMapped = (uint8_t)(uintptr_t)args;


  TEST_POINT_ASSERT_UBYTE(SCHED_TEST_MAPPING_THREAD_CHECK_ID(cpuMapped),
                          cpuMapped == CPUGetId(),
                          cpuMapped,
                          CPUGetId(),
                          TEST_SCHEDULER_ENABLED);
  return args;
}

static void TestCreateInvalid(void)
{
  S_KernelThread* pTestThread;
  E_Return        error;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  /* Invalid routine */
  pTestThread = NULL;
  error = CreateThread(&pTestThread,
                       true,
                       10,
                       name,
                       0x1000,
                       cpuMask,
                       NULL,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(2),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(3),
                            pTestThread == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTestThread,
                            TEST_SCHEDULER_ENABLED);
  /* Invalid stack size */
  pTestThread = NULL;
  error = CreateThread(&pTestThread,
                       true,
                       10,
                       name,
                       0,
                       cpuMask,
                       TestRoutineFail,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(4),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(5),
                            pTestThread == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTestThread,
                            TEST_SCHEDULER_ENABLED);

  /* Invalid name */
  pTestThread = NULL;
  error = CreateThread(&pTestThread,
                       true,
                       10,
                       NULL,
                       0x1000,
                       cpuMask,
                       TestRoutineFail,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(6),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(7),
                            pTestThread == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTestThread,
                            TEST_SCHEDULER_ENABLED);

  /* Invalid priority */
  pTestThread = NULL;
  error = CreateThread(&pTestThread,
                       true,
                       64,
                       name,
                       0x1000,
                       cpuMask,
                       TestRoutineFail,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(8),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(9),
                            pTestThread == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTestThread,
                            TEST_SCHEDULER_ENABLED);

  /* Invalid thread pointer */
  pTestThread = NULL;
  error = CreateThread(NULL,
                       true,
                       0,
                       name,
                       0x1000,
                       cpuMask,
                       TestRoutineFail,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(10),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);

  /* Invalid mapping */
  pTestThread = NULL;
  CPU_MASK_RESET(cpuMask);
  error = CreateThread(&pTestThread,
                       true,
                       0,
                       name,
                       0x1000,
                       cpuMask,
                       TestRoutineFail,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(12),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(13),
                            pTestThread == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTestThread,
                            TEST_SCHEDULER_ENABLED);
  pTestThread = NULL;
  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, CPUGetCount());
  error = CreateThread(&pTestThread,
                       true,
                       0,
                       name,
                       0x1000,
                       cpuMask,
                       TestRoutineFail,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(14),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(15),
                            pTestThread == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTestThread,
                            TEST_SCHEDULER_ENABLED);
}

static void TestSleep(void)
{
  uint64_t currentTime;
  uint64_t newTime;
  E_Return error;

  currentTime = TimeGetUptime();
  error = SleepNs(0xFFFFFFFFFFFFFFFF);
  newTime = TimeGetUptime();
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_SLEEP_ID(0),
                          error == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(1),
                           newTime - currentTime < 100000000,
                           100000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);

  currentTime = TimeGetUptime();
  error = SleepNs(1000000);
  newTime = TimeGetUptime();
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_SLEEP_ID(2),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(3),
                           newTime - currentTime > 1000000,
                           1000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(4),
                           newTime - currentTime < 5000000,
                           5000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);
  currentTime = TimeGetUptime();
  error = SleepNs(10000000);
  newTime = TimeGetUptime();
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_SLEEP_ID(5),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(6),
                           newTime - currentTime > 10000000,
                           10000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(7),
                           newTime - currentTime < 15000000,
                           15000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);
  currentTime = TimeGetUptime();
  error = SleepNs(100000000);
  newTime = TimeGetUptime();
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_SLEEP_ID(8),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(9),
                           newTime - currentTime > 100000000,
                           100000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_UDWORD(SCHED_TEST_SLEEP_ID(10),
                           newTime - currentTime < 105000000,
                           105000000,
                           newTime - currentTime,
                           TEST_SCHEDULER_ENABLED);
}

static void TestJoin(void)
{
  S_KernelThread* pTestThread;
  S_KernelThread* pSelf;
  S_KernelThread  copyThread;
  E_Return        error;
  S_CPUMask       cpuMask;
  void*           returnValue;
  char            name[32] = "TEST_THREAD_JOIN\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  /* Join after end */
  error = CreateThread(&pTestThread,
                       true,
                       20,
                       name,
                       0x1000,
                       cpuMask,
                       TestJoinRoutine,
                       (void*)0);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  SleepNs(5000000000);
  error = JoinThread(pTestThread, &returnValue);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_JOIN_THREAD_ID(2),
                            returnValue == (void*)0xC0DE,
                            ((uintptr_t)0xC0DE),
                            (uintptr_t)returnValue,
                            TEST_SCHEDULER_ENABLED);

  /* Join before end */
  error = CreateThread(&pTestThread,
                       true,
                       20,
                       name,
                       0x1000,
                       cpuMask,
                       TestJoinRoutine,
                       (void*)1000000000);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(3),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  error = JoinThread(pTestThread, &returnValue);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(4),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_JOIN_THREAD_ID(5),
                            returnValue == (void*)0xD00D,
                            ((uintptr_t)0xD00D),
                            (uintptr_t)returnValue,
                            TEST_SCHEDULER_ENABLED);



  /* Join self */
  pSelf = SchedulerGetCurrentThread();
  returnValue = (void*)0xDEAD;
  error = JoinThread(pSelf, &returnValue);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(6),
                          error == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_JOIN_THREAD_ID(7),
                            returnValue == (void*)0xDEAD,
                            ((uintptr_t)0xDEAD),
                            (uintptr_t)returnValue,
                            TEST_SCHEDULER_ENABLED);

  /* Join other process */
  error = CreateThread(&pTestThread,
                       true,
                       20,
                       name,
                       0x1000,
                       cpuMask,
                       TestJoinRoutine,
                       (void*)1000000000);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(8),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  memcpy(&copyThread, pTestThread, sizeof(S_KernelThread));
  copyThread.pProcess = (void*)0xDEADBEEFC0DE;
  returnValue = (void*)0xDEAD;
  error = JoinThread(&copyThread, &returnValue);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(9),
                          error == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_JOIN_THREAD_ID(10),
                            returnValue == (void*)0xDEAD,
                            ((uintptr_t)0xDEAD),
                            (uintptr_t)returnValue,
                            TEST_SCHEDULER_ENABLED);
  error = JoinThread(pTestThread, &returnValue);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_JOIN_THREAD_ID(11),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_JOIN_THREAD_ID(12),
                            returnValue == (void*)0xD00D,
                            ((uintptr_t)0xD00D),
                            (uintptr_t)returnValue,
                            TEST_SCHEDULER_ENABLED);

}

static void TestErrored(void)
{
  S_KernelThread* pTestThread;
  E_Return        error;
  S_CPUMask       cpuMask;
  void*           returnValue;
  char            name[32] = "TEST_THREAD_JOIN\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  /* Join after end */
  error = CreateThread(&pTestThread,
                       true,
                       20,
                       name,
                       0x1000,
                       cpuMask,
                       TestErroredRoutine,
                       (void*)0);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_ERROR_THREAD_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  returnValue = NULL;
  error = JoinThread(pTestThread, &returnValue);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_ERROR_THREAD_ID(1),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_ERROR_THREAD_ID(2),
                            returnValue == (void*)NULL,
                            ((uintptr_t)NULL),
                            (uintptr_t)returnValue,
                            TEST_SCHEDULER_ENABLED);
}

static void TestGetters(void)
{
  S_KernelThread* pSelf;
  S_KernelProcess* pSelfProcess;

  /* Get current thread */
  pSelf = SchedulerGetCurrentThread();
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_GETTER_ID(0),
                            spTestThread == pSelf,
                            ((uintptr_t)spTestThread),
                            (uintptr_t)pSelf,
                            TEST_SCHEDULER_ENABLED);

  /* Get current process */
  pSelfProcess = SchedulerGetCurrentProcess();
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_GETTER_ID(1),
                            spTestThread->pProcess == pSelfProcess,
                            ((uintptr_t)spTestThread->pProcess),
                            (uintptr_t)pSelfProcess,
                            TEST_SCHEDULER_ENABLED);
}

static void TestPriority(void)
{
  uint32_t        i;
  S_KernelThread *pThreads[64];
  E_Return        error;
  S_CPUMask       cpuMask;
  void*           returnValue;
  char            name[32] = "TEST_THREAD_PRIO\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  sLastPrio = 0;

  for (i = 0; i < 64; ++i)
  {
    error = CreateThread(&pThreads[i],
                         true,
                         63 - i,
                         name,
                         0x1000,
                         cpuMask,
                         TestPriorityRoutine,
                         (void*)(uintptr_t)(63 - i));
   TEST_POINT_ASSERT_RCODE(SCHED_TEST_PRIORITY_THREAD_CREATE_ID(i),
                           error == NO_ERROR,
                           NO_ERROR,
                           error,
                           TEST_SCHEDULER_ENABLED);
  }
  for (i = 0; i < 64; ++i)
  {
    error = JoinThread(pThreads[i], &returnValue);
    TEST_POINT_ASSERT_RCODE(SCHED_TEST_PRIORITY_THREAD_JOIN_ID(i * 2),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_SCHEDULER_ENABLED);
    TEST_POINT_ASSERT_POINTER(SCHED_TEST_PRIORITY_THREAD_JOIN_ID(i * 2 + 1),
                              returnValue == (void*)(uintptr_t)(63 - i),
                              ((uintptr_t)(63 - i)),
                              (uintptr_t)returnValue,
                              TEST_SCHEDULER_ENABLED);
  }
}

static void TestMappedCore(void)
{
  uint32_t        i;
  S_KernelThread *pThread;
  E_Return        error;
  S_CPUMask       cpuMask;
  void*           returnValue;
  char            name[32] = "TEST_THREAD_PRIO\0";
  uint32_t        coreCount;

  coreCount = CPUGetCount();


  /* Test one to each core */
  for (i = 0; i < coreCount; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i);
    error = CreateThread(&pThread,
                         true,
                         0,
                         name,
                         0x1000,
                         cpuMask,
                         TestMappingRoutine,
                         (void*)(uintptr_t)(i));
   TEST_POINT_ASSERT_RCODE(SCHED_TEST_MAPPING_THREAD_CREATE_ID(i),
                           error == NO_ERROR,
                           NO_ERROR,
                           error,
                           TEST_SCHEDULER_ENABLED);

    error = JoinThread(pThread, &returnValue);
    TEST_POINT_ASSERT_RCODE(SCHED_TEST_MAPPING_THREAD_JOIN_ID(i * 2),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_SCHEDULER_ENABLED);
    TEST_POINT_ASSERT_POINTER(SCHED_TEST_MAPPING_THREAD_JOIN_ID(i * 2 + 1),
                              returnValue == (void*)(uintptr_t)(i),
                              ((uintptr_t)(i)),
                              (uintptr_t)returnValue,
                              TEST_SCHEDULER_ENABLED);
  }
}

static void TestCreateValid(void)
{
  E_Return        error;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

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
                       TestRoutine,
                       (void*)0xDEADC0DEBEEF0000ULL);
  TEST_POINT_ASSERT_RCODE(SCHED_TEST_CREATE_TEST_THREAD_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_SCHEDULER_ENABLED);
  TEST_POINT_ASSERT_POINTER(SCHED_TEST_CREATE_TEST_THREAD_ID(1),
                            spTestThread != NULL,
                            0xFFFFFFFFFFFFFFFFULL,
                            (uintptr_t)spTestThread,
                            TEST_SCHEDULER_ENABLED);
}

void SchedulerTest(void)
{


  /* Check isInitialized state */
  TEST_POINT_ASSERT_UBYTE(SCHED_TEST_IS_INIT_ID,
                          SchedulerIsInitialized() == true,
                          true,
                          SchedulerIsInitialized(),
                          TEST_SCHEDULER_ENABLED);

  TestCreateValid();
  TestCreateInvalid();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/