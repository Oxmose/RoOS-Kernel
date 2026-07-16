/*******************************************************************************
 * @file TestFramework.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/04/2023
 *
 * @version 1.0
 *
 * @brief Testing framework.
 *
 * @details Testing framework. This modules allows to add dynamic test points
 * to the kernel an run a test suite.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <stdint.h>
#include <Critical.h>
#include <KernelError.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestFramework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Testing framework version. */
#define TEST_FRAMEWORK_VERSION "1.0"

/** @brief Defines the current module's name. */
#define MODULE_NAME "TEST FRAMEWORK"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief Test item value type.
 *
 * @details Identifies the type of the recorded test item value so the test
 * framework can format the result correctly.
 */
typedef enum
{
  TEST_TYPE_BYTE    = 0,
  TEST_TYPE_UBYTE   = 1,
  TEST_TYPE_HALF    = 2,
  TEST_TYPE_UHALF   = 3,
  TEST_TYPE_WORD    = 4,
  TEST_TYPE_UWORD   = 5,
  TEST_TYPE_DWORD   = 6,
  TEST_TYPE_UDWORD  = 7,
  TEST_TYPE_FLOAT   = 8,
  TEST_TYPE_DOUBLE  = 9,
  TEST_TYPE_RCODE   = 10,
  TEST_TYPE_POINTER = 11
} E_TestItemType;

/**
 * @brief Test item recorded by the testing framework.
 *
 * @details Stores the expected and actual values for a single test point,
 * along with the result status, ID, and a link to the next item.
 */
typedef struct S_TestItem
{
  /** @brief True when the test condition passed. */
  bool status;
  /** @brief Actual value recorded by the test. */
  uint64_t value;
  /** @brief Expected value to compare against the actual value. */
  uint64_t expected;
  /** @brief Identifier for the test point. */
  uint32_t id;
  /** @brief Value type stored in this test item. */
  E_TestItemType type;
  /** @brief Link to the next test item in the list. */
  struct S_TestItem* pNext;
} S_TestItem;

/**
 * @brief Float raw data container.
 *
 * @details Converts between a float value and its raw 32-bit representation
 * for stable storage in the test item list.
 */
typedef union
{
  /** @brief Float value. */
  float floatValue;
  /** @brief Raw 32-bit representation. */
  uint32_t rawValue;
} S_FloatRaw;

/**
 * @brief Double raw data container.
 *
 * @details Converts between a double value and its raw 64-bit representation
 * for stable storage in the test item list.
 */
typedef union
{
  /** @brief Double value. */
  double doubleValue;
  /** @brief Raw 64-bit representation. */
  uint64_t rawValue;
} S_DoubleRaw;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define TEST_ASSERT(COND, MSG, ERROR) {                 \
  if ((COND) == false)                                  \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false);              \
  }                                                     \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Test buffer pool base address defined by linker. */
extern uint8_t _KERNEL_TEST_BUFFER_BASE;
/** @brief Test buffer pool size defined by linker. */
extern uint8_t _KERNEL_TEST_BUFFER_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Number of tests executed during the suite. */
static uint32_t sTestCount = 0;

/** @brief Number of tests executed during the suite that failed. */
static uint32_t sFailures = 0;

/** @brief Number of tests executed during the suite that succeeded. */
static uint32_t sSuccess = 0;

/** @brief Memory pool head pointer. */
static uint8_t* spMemoryPoolHead = NULL;

/** @brief Test items list head */
S_TestItem* sTestList = NULL;

/** @brief Final test when allocation failed */
S_TestItem sNULLTestItem =
{
  .expected = -1,
  .id       = -1,
  .type     = TEST_TYPE_BYTE,
  .value    = -1,
  .status   = false
};

/** @brief Test spinlock */
static S_KernelSpinlock sLock;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Allocate memory from the test buffer pool.
 *
 * @details Returns a pointer to the next free block from the test buffer
 * pool, or NULL if the pool is exhausted.
 *
 * @param[in] kSize The number of bytes to allocate.
 *
 * @return Pointer to allocated memory, or NULL if allocation failed.
 */
static void* _GetTestMemory(const size_t kSize);

/**
 * @brief Initialize and link a new test item.
 *
 * @details Creates a new test item, updates the test counters, and links the
 * item into the global test list.
 *
 * @param[in] kTestId The test point identifier.
 * @param[in] kCondition The result of the test condition.
 * @param[in] kExpected The expected raw value.
 * @param[in] kValue The actual raw value.
 * @param[out] ppitem Pointer to receive the allocated test item.
 */
static void _InitTestItem(const uint32_t kTestId,
                          const bool     kCondition,
                          const uint64_t kExpected,
                          const uint64_t kValue,
                          S_TestItem**   ppitem);

/**
 * @brief Halt execution and notify QEMU.
 *
 * @details Sends the exit code to QEMU and halts the CPU indefinitely.
 */
void _KillQEMU(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _GetTestMemory(const size_t kSize)
{
  uint8_t* pAddress;

  if ((uint32_t)(uintptr_t)&_KERNEL_TEST_BUFFER_SIZE +
      &_KERNEL_TEST_BUFFER_BASE >
      spMemoryPoolHead + kSize)
  {
    pAddress = spMemoryPoolHead;
    spMemoryPoolHead += kSize;
  }
  else
  {
    pAddress = NULL;
  }

  return pAddress;
}

static void _InitTestItem(const uint32_t kTestId,
                          const bool     kCondition,
                          const uint64_t kExpected,
                          const uint64_t kValue,
                          S_TestItem**   ppitem)
{
  KERNEL_LOCK(sLock);

  /* Create test item and allocate memory */
  *ppitem = _GetTestMemory(sizeof(S_TestItem));
  if (*ppitem != NULL)
  {
    /* Setup information */
    (*ppitem)->id       = kTestId;
    (*ppitem)->status   = kCondition;
    (*ppitem)->expected = kExpected;
    (*ppitem)->value    = kValue;

    /* If the condition is expected then increment */
    if (kCondition == true)
    {
      ++sSuccess;
    }
    else
    {
      ++sFailures;
    }
    ++sTestCount;

    /* Link test */
    (*ppitem)->pNext = sTestList;
    sTestList = *ppitem;
  }
  else
  {
    /* Link NULL test item */
    ++sFailures;
    ++sTestCount;

    /* Link test */
    if (sTestList != &sNULLTestItem)
    {

      sNULLTestItem.pNext = sTestList;
      sTestList = &sNULLTestItem;
    }

    *ppitem = &sNULLTestItem;
  }

  KERNEL_UNLOCK(sLock);
}

void _KillQEMU(void)
{
  while(1)
  {
    __asm__ __volatile__("outw %w0, %w1" : : "ax" (0x2000), "Nd" (0x604));
    __asm__ ("hlt");
  }
}

void TestFrameworkInit(void)
{
  KERNEL_SPINLOCK_INIT(sLock);
  spMemoryPoolHead = &_KERNEL_TEST_BUFFER_BASE;
}

void TestFrameworkEnd(void)
{
  uint32_t    i;
  S_TestItem* pTestCursor;

  /* Print output header */
  KPrintf("\n#-------- TESTING SECTION START --------#\n");
  KPrintf("{\n");
  KPrintf("\t\"version\": \"" TEST_FRAMEWORK_VERSION "\",\n");
  KPrintf("\t\"name\": \"" TEST_FRAMEWORK_TEST_NAME "\",\n");
  KPrintf("\t\"number_of_tests\": %d,\n", sTestCount);
  KPrintf("\t\"failures\": %d,\n", sFailures);
  KPrintf("\t\"success\": %d,\n", sSuccess);
  KPrintf("\t\"test_suite\": {\n");

  pTestCursor = sTestList;

  for (i = 0; i < sTestCount && pTestCursor != NULL; ++i)
  {
    KPrintf("\t\t\"%d\": {\n", pTestCursor->id);

    KPrintf("\t\t\t\"result\": %lu,\n", pTestCursor->value);
    KPrintf("\t\t\t\"expected\": %lu,\n", pTestCursor->expected);
    KPrintf("\t\t\t\"status\": %d,\n", pTestCursor->status);
    KPrintf("\t\t\t\"type\": %d\n", pTestCursor->type);
    KPrintf("\t\t}");

    if (i < sTestCount - 1)
    {
      KPrintf(",\n");
    }
    else
    {
      KPrintf("\n");
    }

    pTestCursor = pTestCursor->pNext;
  }

  KPrintf("\t}\n");
  KPrintf("}\n");
  KPrintf("#-------- TESTING SECTION END --------#\n");

  _KillQEMU();
  while(1)
  {
    CPUInterruptDisable();
    CPUHalt();
  }
}

void TestFrameworkAssertUint(const uint32_t kTestId,
                             const bool     kCondition,
                             const uint32_t kExpected,
                             const uint32_t kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_UDWORD;
}

void TestFrameworkAssertInt(const uint32_t kTestId,
                            const bool     kCondition,
                            const int32_t  kExpected,
                            const int32_t  kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_DWORD;
}

void TestFrameworkAssertHuint(const uint32_t kTestId,
                              const bool     kCondition,
                              const uint16_t kExpected,
                              const uint16_t kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_UHALF;
}

void TestFrameworkAssertHint(const uint32_t kTestId,
                             const bool     kCondition,
                             const int16_t  kExpected,
                             const int16_t  kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_HALF;
}

void TestFrameworkAssertUbyte(const uint32_t kTestId,
                              const bool     kCondition,
                              const uint8_t  kExpected,
                              const uint8_t  kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_UBYTE;
}

void TestFrameworkAssertByte(const uint32_t kTestId,
                             const bool     kCondition,
                             const int8_t   kExpected,
                             const int8_t   kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_BYTE;
}

void TestFrameworkAssertUdword(const uint32_t kTestId,
                               const bool     kCondition,
                               const uint64_t kExpected,
                               const uint64_t kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                kExpected,
                kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_UDWORD;
}

void TestFrameworkAssertDword(const uint32_t kTestId,
                              const bool     kCondition,
                              const int64_t  kExpected,
                              const int64_t  kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_DWORD;
}

void TestFrameworkAssertFloat(const uint32_t kTestId,
                              const bool     kCondition,
                              const float    kExpected,
                              const float    kValue)
{
  S_TestItem* pItem;
  S_FloatRaw  convExpected;
  S_FloatRaw  convValue;

  /* Init */
  convExpected.floatValue = kExpected;
  convValue.floatValue = kValue;
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)convExpected.rawValue,
                (uint64_t)convValue.rawValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_FLOAT;
}

void TestFrameworkAssertDouble(const uint32_t kTestId,
                               const bool     kCondition,
                               const double   kExpected,
                               const double   kValue)
{
  S_TestItem*  pItem;
  S_DoubleRaw  convExpected;
  S_DoubleRaw  convValue;

  /* Init */
  convExpected.doubleValue = kExpected;
  convValue.doubleValue = kValue;
  _InitTestItem(kTestId,
                kCondition,
                convExpected.rawValue,
                convValue.rawValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_DOUBLE;
}

void TestFrameworkAssertErrCode(const uint32_t kTestId,
                                const bool     kCondition,
                                const E_Return kExpected,
                                const E_Return kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_RCODE;
}

void TestFrameworkAssertPointer(const uint32_t kTestId,
                                const bool     kCondition,
                                const uintptr_t kExpected,
                                const uintptr_t kValue)
{
  S_TestItem* pItem;

  /* Init */
  _InitTestItem(kTestId,
                kCondition,
                (uint64_t)kExpected,
                (uint64_t)kValue,
                &pItem);

  /* Set additional data */
  pItem->type = TEST_TYPE_POINTER;
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/