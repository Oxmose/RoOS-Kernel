
/*******************************************************************************
 * @file CriticalTests.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework critical testing.
 *
 * @details Testing framework critical testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <CPUSet.h>
#include <Critical.h>
#include <Scheduler.h>
#include <KernelError.h>

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
static volatile uint64_t criticalValueTest;
static volatile uint64_t lockValueTest;
static T_U32Atomic incValueTest;
static T_U32Atomic decValueTest;
static T_U32Atomic orValueTest;
static T_U32Atomic exchangeLockTest;
static volatile uint32_t exchangeCountTest;
static T_Spinlock lock = SPINLOCK_INIT_VALUE;
static T_Spinlock spinlock = SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void* _TestRoutine(void* args);
static void* _CriticalLocalRoutine(void* args);
static void* _CriticalGlobalRoutine0(void* args);
static void* _CriticalGlobalRoutine1(void* args);
static void* _SpinlockTestRoutine(void* args);
static void* _AtomicIncRoutine(void* args);
static void* _AtomicDecRoutine(void* args);
static void* _AtomicOrRoutine(void* args);
static void* _AtomicExchangeRoutine(void* args);
static void _TestLocal(void);
static void _TestGlobal(void);
static void _TestGlobalSecond(void);
static void _TestSpinlock(void);
static void _TestIncrement(void);
static void _TestDecrement(void);
static void _TestAtomicOrExchange(void);
static void _TestAtomicOrExchangeConcurrent(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _CriticalLocalRoutine(void* args)
{
  (void)args;
  uint32_t i;
  volatile uint32_t j;
  uint64_t save;
  uint32_t intState;

  for (i = 0; i < 1000000; ++i)
  {
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    save = criticalValueTest;
    for (j = 0; j < 100; ++j);
    criticalValueTest = save + 1;
    KERNEL_EXIT_CRITICAL_LOCAL(intState);
  }

  return NULL;
}

static void* _CriticalGlobalRoutine0(void* args)
{
  (void)args;
  uint32_t i;
  volatile uint32_t j;
  uint64_t save;

  for (i = 0; i < 1000000; ++i)
  {
    SpinlockAcquire(&lock);
    save = criticalValueTest;
    for (j = 0; j < 100; ++j);
    criticalValueTest = save + 1;
    SpinlockRelease(&lock);
  }

  return NULL;
}

static void* _CriticalGlobalRoutine1(void* args)
{
  (void)args;
  uint32_t i;
  volatile uint32_t j;
  uint64_t save;

  for (i = 0; i < 1000000; ++i)
  {
    SpinlockAcquire(&lock);
    save = criticalValueTest;
    for (j = 0; j < 100; ++j);
    criticalValueTest = save + 1;
    SpinlockRelease(&lock);
  }

  return NULL;
}

static void* _SpinlockTestRoutine(void* args)
{
  (void)args;
  uint32_t i;
  volatile uint32_t j;
  uint64_t save;


  for (i = 0; i < 1000000; ++i)
  {
    SpinlockAcquire(&spinlock);
    save = lockValueTest;
    for (j = 0; j < 100; ++j);
    lockValueTest = save + 1;
    SpinlockRelease(&spinlock);
  }

  return NULL;
}

static void* _AtomicIncRoutine(void* args)
{
  (void)args;
  uint32_t i;

  for (i = 0; i < 1000000; ++i)
  {
    AtomicIncrement32(&incValueTest);
  }

  return NULL;
}

static void* _AtomicDecRoutine(void* args)
{
  (void)args;
  uint32_t i;

  for (i = 0; i < 1000000; ++i)
  {
    AtomicDecrement32(&decValueTest);
  }

  return NULL;
}

static void* _AtomicOrRoutine(void* args)
{
  uint32_t i;
  uint32_t bit;

  bit = 1U << (uint32_t)(uintptr_t)args;
  for (i = 0; i < 100000; ++i)
  {
    AtomicOr32(&orValueTest, bit);
  }

  return NULL;
}

static void* _AtomicExchangeRoutine(void* args)
{
  uint32_t i;

  (void)args;
  for (i = 0; i < 100000; ++i)
  {
    while (AtomicExchange32(&exchangeLockTest, 1) != 0);
    ++exchangeCountTest;
    AtomicExchange32(&exchangeLockTest, 0);
  }

  return NULL;
}

static void _TestLocal(void)
{
  uint32_t i;
  E_Return error;
  S_KernelThread* pThreads[20];
  void* retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  criticalValueTest = 0;

  error = NO_ERROR;
  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _CriticalLocalRoutine,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_LOCAL(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);

      TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_JOIN_THREADS_LOCAL(i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_LOCAL,
                         criticalValueTest == 20 * 1000000,
                         20 * 1000000,
                         criticalValueTest,
                         TEST_CRITICAL_ENABLED);

  if (error != NO_ERROR)
  {
    TEST_FRAMEWORK_END();
  }
}

static void _TestGlobal(void)
{
  uint32_t i;
  E_Return error;
  S_KernelThread* pThreads[20];
  void* retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);
  CPU_MASK_SET(cpuMask, 1);
  CPU_MASK_SET(cpuMask, 2);
  CPU_MASK_SET(cpuMask, 3);

  criticalValueTest = 0;

  error = NO_ERROR;
  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _CriticalGlobalRoutine0,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_GLOBAL0(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);

      TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_JOIN_THREADS_GLOBAL0(i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_GLOBAL0,
                         criticalValueTest == 20 * 1000000,
                         20 * 1000000,
                         criticalValueTest,
                         TEST_CRITICAL_ENABLED);

  if (error != NO_ERROR)
  {
    TEST_FRAMEWORK_END();
  }
}

static void _TestGlobalSecond(void)
{
  uint32_t i;
  E_Return error;
  S_KernelThread* pThreads[20];
  void* retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);
  CPU_MASK_SET(cpuMask, 1);
  CPU_MASK_SET(cpuMask, 2);
  CPU_MASK_SET(cpuMask, 3);

  criticalValueTest = 0;

  error = NO_ERROR;
  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _CriticalGlobalRoutine1,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_GLOBAL1(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
      error = JoinThread(pThreads[i], &retVal);

      TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_JOIN_THREADS_GLOBAL1(i),
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_GLOBAL1,
                         criticalValueTest == 20 * 1000000,
                         20 * 1000000,
                         criticalValueTest,
                         TEST_CRITICAL_ENABLED);

  if (error != NO_ERROR)
  {
    TEST_FRAMEWORK_END();
  }
}

static void _TestSpinlock(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[20];
  void*           retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  lockValueTest = 0;
  error = NO_ERROR;

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _SpinlockTestRoutine,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_SPINLOCK(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = JoinThread(pThreads[i], &retVal);

    TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_JOIN_THREADS_SPINLOCK(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_SPINLOCK,
                         lockValueTest == 20 * 1000000,
                         20 * 1000000,
                         lockValueTest,
                         TEST_CRITICAL_ENABLED);
}

static void _TestIncrement(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[20];
  void*           retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  incValueTest = 0;
  error = NO_ERROR;

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _AtomicIncRoutine,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_INCREMENT(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = JoinThread(pThreads[i], &retVal);

    TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_JOIN_THREADS_INCREMENT(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_INCREMENT,
                         incValueTest == 20 * 1000000,
                         20 * 1000000,
                         incValueTest,
                         TEST_CRITICAL_ENABLED);
}

static void _TestDecrement(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[20];
  void*           retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  decValueTest = 20 * 1000000;
  error = NO_ERROR;

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _AtomicDecRoutine,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_DECREMENT(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = JoinThread(pThreads[i], &retVal);

    TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_JOIN_THREADS_DECREMENT(i),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT,
                         decValueTest == 0,
                         0,
                         decValueTest,
                         TEST_CRITICAL_ENABLED);
}

static void _TestAtomicOrExchange(void)
{
  T_U32Atomic value;
  uint32_t     previousValue;

  value = 0x0F0F0000;
  previousValue = AtomicOr32(&value, 0x0000F0F0);

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT + 1,
                         previousValue == 0x0F0F0000,
                         0x0F0F0000,
                         previousValue,
                         TEST_CRITICAL_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT + 2,
                         value == 0x0F0FF0F0,
                         0x0F0FF0F0,
                         value,
                         TEST_CRITICAL_ENABLED);

  previousValue = AtomicExchange32(&value, 0x12345678);

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT + 3,
                         previousValue == 0x0F0FF0F0,
                         0x0F0FF0F0,
                         previousValue,
                         TEST_CRITICAL_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT + 4,
                         value == 0x12345678,
                         0x12345678,
                         value,
                         TEST_CRITICAL_ENABLED);
}

static void _TestAtomicOrExchangeConcurrent(void)
{
  uint32_t        i;
  E_Return        error;
  S_KernelThread* pThreads[20];
  void*           retVal;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  orValueTest = 0;
  error = NO_ERROR;

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _AtomicOrRoutine,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_VALUE_DECREMENT + 5 + i,
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = JoinThread(pThreads[i], &retVal);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_VALUE_DECREMENT + 25 + i,
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT + 45,
                         orValueTest == 0x000FFFFF,
                         0x000FFFFF,
                         orValueTest,
                         TEST_CRITICAL_ENABLED);

  exchangeLockTest = 0;
  exchangeCountTest = 0;
  error = NO_ERROR;

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i % CPUGetCount());
    error = CreateThread(&pThreads[i],
                         true,
                         1,
                         name,
                         0x1000,
                         cpuMask,
                         _AtomicExchangeRoutine,
                         (void*)(uintptr_t)i,
                         NULL);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_VALUE_DECREMENT + 46 + i,
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  for (i = 0; i < 20 && error == NO_ERROR; ++i)
  {
    error = JoinThread(pThreads[i], &retVal);

    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_VALUE_DECREMENT + 66 + i,
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_CRITICAL_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_DECREMENT + 86,
                         exchangeCountTest == 20 * 100000,
                         20 * 100000,
                         exchangeCountTest,
                         TEST_CRITICAL_ENABLED);
}

static void* _TestRoutine(void* args)
{
  (void)args;

  _TestLocal();
  _TestGlobal();
  _TestGlobalSecond();
  _TestSpinlock();
  _TestIncrement();
  _TestDecrement();
  _TestAtomicOrExchange();
  _TestAtomicOrExchangeConcurrent();

  TEST_FRAMEWORK_END();
  return NULL;
}

void CriticalTest(void)
{
  E_Return        returnCode;
  S_KernelThread* pThread;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);
  CPU_MASK_SET(cpuMask, 1);
  CPU_MASK_SET(cpuMask, 2);
  CPU_MASK_SET(cpuMask, 3);

  returnCode = CreateThread(&pThread,
                            true,
                            0,
                            name,
                            0x1000,
                            cpuMask,
                            _TestRoutine,
                            NULL,
                            NULL);

  /* We should never reach this point */
  TEST_POINT_ASSERT_RCODE(CRITICAL_TEST_CREATE_THREAD_ID,
                          returnCode == NO_ERROR,
                          NO_ERROR,
                          returnCode,
                          TEST_CRITICAL_ENABLED);
  if (returnCode != NO_ERROR)
  {
    TEST_FRAMEWORK_END();
  }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/