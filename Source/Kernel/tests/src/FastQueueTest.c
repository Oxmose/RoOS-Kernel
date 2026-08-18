/*******************************************************************************
 * @file FastQueueTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/08/2026
 *
 * @version 1.0
 *
 * @brief Testing framework fast queue testing.
 *
 * @details Testing framework fast queue testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*****************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <FastQueue.h>
#include <KernelError.h>
#include <Scheduler.h>
#include <Critical.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestFramework.h>

/*****************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*****************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*****************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*****************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*****************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
typedef struct
{
  S_FastQueue* pQueue;
  uint32_t values[32];
  uint32_t popped[32];
  T_U32Atomic pushCount;
  T_U32Atomic popCount;
  uint32_t expectedCount;
} S_FastQueueConcurrencyContext;

static void* FastQueueProducerRoutine(void* pArgs)
{
  S_FastQueueConcurrencyContext* pContext;
  uint32_t i;

  pContext = (S_FastQueueConcurrencyContext*)pArgs;
  for (i = 0; i < pContext->expectedCount; ++i)
  {
    pContext->values[i] = i + 1;
    FQueuePush(pContext->pQueue, &pContext->values[i]);
    AtomicIncrement32(&pContext->pushCount);
    SleepNs(10000);
  }

  return NULL;
}

static void* FastQueueConsumerRoutine(void* pArgs)
{
  S_FastQueueConcurrencyContext* pContext;
  uint32_t value;
  uint32_t itemIndex;

  pContext = (S_FastQueueConcurrencyContext*)pArgs;
  while (pContext->popCount < pContext->expectedCount)
  {
    if (FQueuePop(pContext->pQueue, &value) == true)
    {
      itemIndex = AtomicIncrement32(&pContext->popCount);
      pContext->popped[itemIndex] = value;
    }
    else
    {
      SleepNs(1000);
    }
  }

  return NULL;
}

/*****************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* FastQueuesTestRoutine(void* args)
{
  (void)args;
  S_FastQueue* queue;
  S_FastQueue* singleQueue;
  uint32_t values[4] = {10, 20, 30, 40};
  uint32_t singleValues[2] = {50, 60};
  uint32_t popped[4] = {0};
  uint32_t sentinel = 0xDEADBEEF;
  bool available;

  queue = FQueueCreate(3, sizeof(uint32_t));
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CREATE_ID(0),
                           queue != NULL,
                           (uint64_t)1,
                           (uint64_t)(uintptr_t)queue,
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CREATE_ID(1),
                           queue->maxSize == 4,
                           (uint64_t)4,
                           (uint64_t)queue->maxSize,
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CREATE_ID(2),
                           queue->elementSize == sizeof(uint32_t),
                           (uint64_t)sizeof(uint32_t),
                           (uint64_t)queue->elementSize,
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(queue, &popped[0]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_EMPTY_ID(0),
                           available == false,
                           (uint64_t)0,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);

  singleQueue = FQueueCreate(1, sizeof(uint32_t));
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(0),
                           singleQueue != NULL,
                           (uint64_t)1,
                           (uint64_t)(uintptr_t)singleQueue,
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(1),
                           singleQueue->maxSize == 2,
                           (uint64_t)2,
                           (uint64_t)singleQueue->maxSize,
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(singleQueue, &sentinel);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(2),
                           available == false,
                           (uint64_t)0,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(3),
                           sentinel == 0xDEADBEEF,
                           (uint64_t)0xDEADBEEF,
                           (uint64_t)sentinel,
                           TEST_OS_FAST_QUEUES_ENABLED);

  FQueuePush(singleQueue, &singleValues[0]);
  available = FQueuePop(singleQueue, &sentinel);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(4),
                           available == true,
                           (uint64_t)1,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(5),
                           sentinel == singleValues[0],
                           (uint64_t)singleValues[0],
                           (uint64_t)sentinel,
                           TEST_OS_FAST_QUEUES_ENABLED);

  FQueueDestroy(singleQueue);

  FQueuePush(queue, &values[0]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_PUSH_ID(0),
                           queue->head == 1,
                           (uint64_t)1,
                           (uint64_t)queue->head,
                           TEST_OS_FAST_QUEUES_ENABLED);

  FQueuePush(queue, &values[1]);
  FQueuePush(queue, &values[2]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_PUSH_ID(1),
                           queue->head == 3,
                           (uint64_t)3,
                           (uint64_t)queue->head,
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(queue, &popped[0]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(0),
                           available == true,
                           (uint64_t)1,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(1),
                           popped[0] == values[0],
                           (uint64_t)values[0],
                           (uint64_t)popped[0],
                           TEST_OS_FAST_QUEUES_ENABLED);

  FQueuePush(queue, &values[3]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_WRAP_ID(0),
                           queue->head == 0,
                           (uint64_t)0,
                           (uint64_t)queue->head,
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(queue, &popped[1]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(2),
                           available == true,
                           (uint64_t)1,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(3),
                           popped[1] == values[1],
                           (uint64_t)values[1],
                           (uint64_t)popped[1],
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(queue, &popped[2]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(4),
                           available == true,
                           (uint64_t)1,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(5),
                           popped[2] == values[2],
                           (uint64_t)values[2],
                           (uint64_t)popped[2],
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(queue, &popped[3]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(6),
                           available == true,
                           (uint64_t)1,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_POP_ID(7),
                           popped[3] == values[3],
                           (uint64_t)values[3],
                           (uint64_t)popped[3],
                           TEST_OS_FAST_QUEUES_ENABLED);

  available = FQueuePop(queue, &popped[3]);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_EMPTY_ID(1),
                           available == false,
                           (uint64_t)0,
                           (uint64_t)(available ? 1 : 0),
                           TEST_OS_FAST_QUEUES_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CORNER_ID(6),
                           queue->head == queue->tail,
                           (uint64_t)queue->head,
                           (uint64_t)queue->tail,
                           TEST_OS_FAST_QUEUES_ENABLED);

  {
    S_FastQueue* concurrencyQueue;
    S_FastQueueConcurrencyContext context;
    S_KernelThread* producerThread;
    S_KernelThread* consumerThread;
    S_CPUMask cpuMask;
    E_Return error;
    void* returnValue;
    uint32_t i;

    concurrencyQueue = FQueueCreate(8, sizeof(uint32_t));
    TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CONCURRENCY_ID(0),
                             concurrencyQueue != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)concurrencyQueue,
                             TEST_OS_FAST_QUEUES_ENABLED);

    context.pQueue = concurrencyQueue;
    context.expectedCount = 24;
    context.pushCount = 0;
    context.popCount = 0;
    for (i = 0; i < context.expectedCount; ++i)
    {
      context.values[i] = i + 1;
      context.popped[i] = 0;
    }

    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, 0);

    const char producerName[32] = "FQ_PRODUCER";
    const char consumerName[32] = "FQ_CONSUMER";

    error = CreateThread(&producerThread,
                         true,
                         50,
                         producerName,
                         0x1000,
                         cpuMask,
                         FastQueueProducerRoutine,
                         &context);
    TEST_POINT_ASSERT_RCODE(TEST_FASTQUEUE_CONCURRENCY_ID(1),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_FAST_QUEUES_ENABLED);

    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, 1);
    error = CreateThread(&consumerThread,
                         true,
                         50,
                         consumerName,
                         0x1000,
                         cpuMask,
                         FastQueueConsumerRoutine,
                         &context);
    TEST_POINT_ASSERT_RCODE(TEST_FASTQUEUE_CONCURRENCY_ID(2),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_FAST_QUEUES_ENABLED);

    error = JoinThread(producerThread, &returnValue);
    TEST_POINT_ASSERT_RCODE(TEST_FASTQUEUE_CONCURRENCY_ID(3),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_FAST_QUEUES_ENABLED);

    error = JoinThread(consumerThread, &returnValue);
    TEST_POINT_ASSERT_RCODE(TEST_FASTQUEUE_CONCURRENCY_ID(4),
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_OS_FAST_QUEUES_ENABLED);

    TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CONCURRENCY_ID(5),
                             context.pushCount == context.expectedCount,
                             (uint64_t)context.expectedCount,
                             (uint64_t)context.pushCount,
                             TEST_OS_FAST_QUEUES_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CONCURRENCY_ID(6),
                             context.popCount == context.expectedCount,
                             (uint64_t)context.expectedCount,
                             (uint64_t)context.popCount,
                             TEST_OS_FAST_QUEUES_ENABLED);

    for (i = 0; i < context.expectedCount; ++i)
    {
      TEST_POINT_ASSERT_UDWORD(TEST_FASTQUEUE_CONCURRENCY_ID(7 + i),
                               context.popped[i] == (i + 1),
                               (uint64_t)(i + 1),
                               (uint64_t)context.popped[i],
                               TEST_OS_FAST_QUEUES_ENABLED);
    }

    FQueueDestroy(concurrencyQueue);
  }

  FQueueDestroy(queue);
  TEST_FRAMEWORK_END();

  return NULL;
}

void FastQueuesTest(void)
{
  E_Return error;
  S_KernelThread* testThread;
  S_CPUMask cpuMask;
  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  const char testName[32] = "Test";
  error = CreateThread(&testThread,
                         true,
                         0,
                         testName,
                         0x1000,
                         cpuMask,
                         FastQueuesTestRoutine,
                         NULL);
  TEST_POINT_ASSERT_RCODE(TEST_FASTQUEUE_INIT,
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_OS_FAST_QUEUES_ENABLED);
}

#endif

/************************************ EOF *************************************/