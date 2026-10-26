/*******************************************************************************
 * @file KernelWorkerTest.c
 *
 * @brief Kernel worker integration tests.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

#include <stdint.h>
#include <Critical.h>
#include <Scheduler.h>
#include <KernelSemaphore.h>
#include <KernelWorker.h>

#include <TestFramework.h>

#define KWORKER_TEST_TASK_COUNT 16

static volatile uint32_t sExecutedTaskCount;
static volatile uint32_t sTaskArguments[KWORKER_TEST_TASK_COUNT];

static void _KernelWorkerTestTask(void* pArg)
{
  uint32_t index;

  index = *(uint32_t*)pArg;
  sTaskArguments[index] = index + 1;
  AtomicIncrement32(&sExecutedTaskCount);
}

static void *_KernelWorkerTestRoutine(void* pArg)
{
  S_KernelSemaphore completionSemaphore[KWORKER_TEST_TASK_COUNT];
  E_Return error;
  uint32_t taskArguments[KWORKER_TEST_TASK_COUNT];
  uint32_t i;

  (void)pArg;

  sExecutedTaskCount = 0;
  for (i = 0; i < KWORKER_TEST_TASK_COUNT; ++i)
  {
    sTaskArguments[i] = 0;
    taskArguments[i] = i;

    error = KernelSemaphoreInit(&completionSemaphore[i],
                                KSEMAPHORE_FLAG_QUEUING_FIFO,
                                0);
    TEST_POINT_ASSERT_RCODE(TEST_KWORKER_INIT_ID(i),
                            error == NO_ERROR, NO_ERROR, error,
                            TEST_KWORKER_ENABLED);
    if (error != NO_ERROR)
    {
      TEST_FRAMEWORK_END();
      return NULL;
    }
  }

  for (i = 0; i < KWORKER_TEST_TASK_COUNT; ++i)
  {
    KWorkerSubmitTask(_KernelWorkerTestTask,
                              &taskArguments[i],
                              &completionSemaphore[i]);
    TEST_POINT_ASSERT_RCODE(TEST_KWORKER_SUBMIT_ID(i),
                            TRUE, TRUE, TRUE,
                            TEST_KWORKER_ENABLED);
  }

  for (i = 0; i < KWORKER_TEST_TASK_COUNT; ++i)
  {
    error = KernelSemaphoreWait(&completionSemaphore[i]);
    TEST_POINT_ASSERT_RCODE(TEST_KWORKER_WAIT_ID(i),
                            error == NO_ERROR, NO_ERROR, error,
                            TEST_KWORKER_ENABLED);
  }

  TEST_POINT_ASSERT_UINT(TEST_KWORKER_EXECUTION_ID,
                         sExecutedTaskCount == KWORKER_TEST_TASK_COUNT,
                         KWORKER_TEST_TASK_COUNT,
                         sExecutedTaskCount,
                         TEST_KWORKER_ENABLED);
  for (i = 0; i < KWORKER_TEST_TASK_COUNT; ++i)
  {
    TEST_POINT_ASSERT_UINT(TEST_KWORKER_ARGUMENT_ID(i),
                           sTaskArguments[i] == i + 1,
                           i + 1,
                           sTaskArguments[i],
                           TEST_KWORKER_ENABLED);
  }

  for (i = 0; i < KWORKER_TEST_TASK_COUNT; ++i)
  {
    error = KernelSemaphoreDestroy(&completionSemaphore[i]);
    TEST_POINT_ASSERT_RCODE(TEST_KWORKER_DESTROY_ID(i),
                            error == NO_ERROR, NO_ERROR, error,
                            TEST_KWORKER_ENABLED);
  }
  TEST_FRAMEWORK_END();

  return NULL;
}

void KernelWorkerTest(void)
{
  E_Return        error;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";
  S_KernelThread* pTestThread;

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  /* Create the test thread */
  pTestThread = NULL;
  error = CreateThread(&pTestThread,
                       true,
                       0,
                       name,
                       0x1000,
                       cpuMask,
                       _KernelWorkerTestRoutine,
                       (void*)0xDEADC0DEBEEF0000ULL,
                         NULL);
  TEST_POINT_ASSERT_RCODE(TEST_KWORKER_INIT_THREAD_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_KWORKER_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KWORKER_INIT_THREAD_ID(1),
                            pTestThread != NULL,
                            0xFFFFFFFFFFFFFFFFULL,
                            (uintptr_t)pTestThread,
                            TEST_KWORKER_ENABLED);
}

#endif
