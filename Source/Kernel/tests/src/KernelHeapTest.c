
/*******************************************************************************
 * @file KernelHeapTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework kernel heap testing.
 *
 * @details Testing framework kernel heap testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <stdbool.h>
#include <KernelHeap.h>
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
extern uint8_t _KERNEL_HEAP_BASE;
extern uint8_t _KERNEL_NON_FREE_HEAP_BASE;
extern uint8_t _KERNEL_HEAP_SIZE;
extern uint8_t _KERNEL_NON_FREE_HEAP_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
static uintptr_t sAllocated[200];
static uintptr_t sSecondAllocated[200];

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void _TestKHeapNoFree(void);
static void _TestKHeapFree(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _TestKHeapNoFree(void)
{
  uintptr_t allocated;
  uintptr_t previousAlloc;
  uintptr_t upBound;
  uintptr_t downBound;
  uint32_t  i;

  downBound = (uintptr_t)&_KERNEL_NON_FREE_HEAP_BASE;
  upBound = downBound + (uintptr_t)&_KERNEL_NON_FREE_HEAP_SIZE;

  /* Test 1 Byte alignement */
  allocated = downBound - 1;
  for (i = 0; i < 5; ++i)
  {
    previousAlloc = allocated;
    allocated = (uintptr_t)KMalloc(1, ALIGN_1_BYTE, KMALLOC_NO_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(i),
                              (allocated & 0xF) == i,
                              i,
                              allocated & 0xF,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(i),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(i),
                              previousAlloc +1 == allocated,
                              previousAlloc +1,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  /* Test 2 Byte alignement */
  previousAlloc = allocated;
  allocated = (uintptr_t)KMalloc(1, ALIGN_2_BYTES, KMALLOC_NO_FREE_POOL);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(5),
                            (allocated & 0x1) == 0,
                            0,
                            allocated & 0x1,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(5),
                            downBound <= allocated && allocated <= upBound,
                            0,
                            allocated,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(5),
                            previousAlloc +2 == allocated,
                            previousAlloc +2,
                            allocated,
                            TEST_KHEAP_ENABLED);

  for (i = 0; i < 4; ++i)
  {
    previousAlloc = allocated;
    allocated = (uintptr_t)KMalloc(1, ALIGN_2_BYTES, KMALLOC_NO_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(i + 6),
                              (allocated & 0x1) == 0,
                              0,
                              allocated & 0x1,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(i + 6),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(i + 6),
                              previousAlloc +2 == allocated,
                              previousAlloc +2,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  /* Test 4 Byte alignement */
  previousAlloc = allocated;
  allocated = (uintptr_t)KMalloc(1, ALIGN_4_BYTES, KMALLOC_NO_FREE_POOL);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(10),
                            (allocated & 0x3) == 0,
                            0,
                            allocated & 0x3,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(10),
                            downBound <= allocated && allocated <= upBound,
                            0,
                            allocated,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(10),
                            previousAlloc +2 == allocated,
                            previousAlloc +2,
                            allocated,
                            TEST_KHEAP_ENABLED);

  for (i = 0; i < 5; ++i)
  {
    previousAlloc = allocated;
    allocated = (uintptr_t)KMalloc(1, ALIGN_4_BYTES, KMALLOC_NO_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(i + 11),
                              (allocated & 0x3) == 0,
                              0,
                              allocated & 0x3,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(i + 11),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(i + 11),
                              previousAlloc +4 == allocated,
                              previousAlloc +4,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  /* Test 8 Byte alignement */
  previousAlloc = allocated;
  allocated = (uintptr_t)KMalloc(1, ALIGN_8_BYTES, KMALLOC_NO_FREE_POOL);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(16),
                            (allocated & 0x7) == 0,
                            0,
                            allocated & 0x7,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(16),
                            downBound <= allocated && allocated <= upBound,
                            0,
                            allocated,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(16),
                            previousAlloc +4 == allocated,
                            previousAlloc +4,
                            allocated,
                            TEST_KHEAP_ENABLED);
  for (i = 0; i < 3; ++i)
  {
    previousAlloc = allocated;
    allocated = (uintptr_t)KMalloc(1, ALIGN_8_BYTES, KMALLOC_NO_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALIGN(i + 17),
                              (allocated & 0x7) == 0,
                              0,
                              allocated & 0x7,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_RANGE(i + 17),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_NO_FREE_ALLOC(i + 17),
                              previousAlloc +8 == allocated,
                              previousAlloc +8,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }
}

static void _TestKHeapFree(void)
{
  uintptr_t allocated;
  uintptr_t upBound;
  uintptr_t downBound;
  int32_t  i;

  downBound = (uintptr_t)&_KERNEL_HEAP_BASE;
  upBound = downBound + (uintptr_t)&_KERNEL_HEAP_SIZE;

  /* Test 1 Byte alignement */
  for (i = 0; i < 5; ++i)
  {
    allocated = (uintptr_t)KMalloc(1, ALIGN_1_BYTE, KMALLOC_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(i),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  /* Test 2 Byte alignement */
  allocated = (uintptr_t)KMalloc(1, ALIGN_2_BYTES, KMALLOC_FREE_POOL);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALIGN(5),
                            (allocated & 0x1) == 0,
                            0,
                            allocated & 0x1,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(5),
                            downBound <= allocated && allocated <= upBound,
                            0,
                            allocated,
                            TEST_KHEAP_ENABLED);

  for (i = 0; i < 4; ++i)
  {
    allocated = (uintptr_t)KMalloc(1, ALIGN_2_BYTES, KMALLOC_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALIGN(i + 6),
                              (allocated & 0x1) == 0,
                              0,
                              allocated & 0x1,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(i + 6),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  /* Test 4 Byte alignement */
  allocated = (uintptr_t)KMalloc(1, ALIGN_4_BYTES, KMALLOC_FREE_POOL);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALIGN(10),
                            (allocated & 0x3) == 0,
                            0,
                            allocated & 0x3,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(10),
                            downBound <= allocated && allocated <= upBound,
                            0,
                            allocated,
                            TEST_KHEAP_ENABLED);

  for (i = 0; i < 5; ++i)
  {
    allocated = (uintptr_t)KMalloc(1, ALIGN_4_BYTES, KMALLOC_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALIGN(i + 11),
                              (allocated & 0x3) == 0,
                              0,
                              allocated & 0x3,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(i + 11),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  /* Test 8 Byte alignement */
  allocated = (uintptr_t)KMalloc(1, ALIGN_8_BYTES, KMALLOC_FREE_POOL);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALIGN(16),
                            (allocated & 0x7) == 0,
                            0,
                            allocated & 0x7,
                            TEST_KHEAP_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(16),
                            downBound <= allocated && allocated <= upBound,
                            0,
                            allocated,
                            TEST_KHEAP_ENABLED);
  for (i = 0; i < 3; ++i)
  {
    allocated = (uintptr_t)KMalloc(1, ALIGN_8_BYTES, KMALLOC_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALIGN(i + 17),
                              (allocated & 0x7) == 0,
                              0,
                              allocated & 0x7,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_RANGE(i + 17),
                              downBound <= allocated && allocated <= upBound,
                              0,
                              allocated,
                              TEST_KHEAP_ENABLED);
  }

  for (i = 0; i < 200; ++i)
  {
    sAllocated[i] = (uintptr_t)KMalloc(32, ALIGN_8_BYTES, KMALLOC_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALLOC(i * 3),
                              (sAllocated[i] & 0x7) == 0,
                              0,
                              sAllocated[i] & 0x7,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALLOC(i * 3 + 1),
                              downBound <= sAllocated[i] && sAllocated[i] <= upBound,
                              0,
                              sAllocated[i],
                              TEST_KHEAP_ENABLED);
    if (i != 0)
    {
      TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALLOC(i * 3 + 2),
                                sAllocated[i] >= sAllocated[i - 1] + 32,
                                sAllocated[i - 1] + 32,
                                sAllocated[i],
                                TEST_KHEAP_ENABLED);
    }
  }
  for (i = 199; i >= 0; --i)
  {
    KFree((void*)sAllocated[i]);
    sSecondAllocated[i] = (uintptr_t)KMalloc(32, ALIGN_8_BYTES, KMALLOC_FREE_POOL);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALLOC(600 + i * 3),
                              (sAllocated[i] & 0x7) == 0,
                              0,
                              sAllocated[i] & 0x7,
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALLOC(600 + i * 3 + 1),
                              downBound <= sAllocated[i] && sAllocated[i] <= upBound,
                              0,
                              sAllocated[i],
                              TEST_KHEAP_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_KHEAP_FREE_ALLOC(600 + i * 3 + 2),
                              sAllocated[i] == sSecondAllocated[i],
                              sAllocated[i],
                              sSecondAllocated[i],
                              TEST_KHEAP_ENABLED);
    KFree((void*)sSecondAllocated[i]);
  }
}

void KernelHeapTest(void)
{
  _TestKHeapNoFree();
  _TestKHeapFree();

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/