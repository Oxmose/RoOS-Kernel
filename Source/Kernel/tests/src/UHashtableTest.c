
/*******************************************************************************
 * @file UHashtableTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework UHashtable testing.
 *
 * @details Testing framework UHashtable testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <KernelHeap.h>
#include <UHashtable.h>
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
static uint32_t sGSeed = 0x21025;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static uint32_t _RandomGet(void);
static void* _Alloc(const size_t kSize);
static void _Free(void* ptr);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static uint32_t _RandomGet(void)
{
  sGSeed = 214013 * sGSeed + 2531011;
  return sGSeed;
}

static void* _Alloc(const size_t kSize)
{
  void* addr;
  addr = KMalloc(kSize, ALIGN_ADDRESS, KMALLOC_FREE_POOL);
  return addr;
}

static void _Free(void* ptr)
{
  KFree(ptr);
}

void UHashtableTest(void)
{
  size_t        i;
  S_UHashtable* table;
  uint32_t      data;
  E_Return      err;

  table = UHashtableCreate(UHASHTABLE_ALLOCATOR(_Alloc, _Free), &err);
  TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_CREATE_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(1),
                            table != NULL,
                            (uint64_t)1,
                            (uint64_t)(uintptr_t)table,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(2),
                            table->size == 0,
                            (uint64_t)0,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(3),
                            table->capacity == 16,
                            (uint64_t)16,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 26; ++i)
  {
    err = UHashtableSet(table, i, (void*)(i * 10));
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST_ID(i),
                            err == NO_ERROR,
                            NO_ERROR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(0),
                            table->size == 26,
                            (uint64_t)26,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(1),
                            table->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 26; ++i)
  {
    err = UHashtableGet(table, i, (void**)&data);
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST_ID(i * 2),
                            err == NO_ERROR,
                            NO_ERROR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST_ID(i * 2 + 1),
                             (uint64_t)(uintptr_t)data == i * 10,
                             (uint64_t)i * 10,
                             (uint64_t)(uintptr_t)data,
                             TEST_OS_UHASHTABLE_ENABLED)
  }

  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET_ID(0),
                            table->size == 26,
                            (uint64_t)26,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET_ID(1),
                            table->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 26; i += 2)
  {
    err = UHashtableSet(table, i, (void*)(i * 100));
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST_ID(i + 26),
                            err == NO_ERROR,
                            NO_ERROR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(2),
                           table->size == 26,
                           (uint64_t)26,
                           (uint64_t)table->size,
                           TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(3),
                           table->capacity == 64,
                           (uint64_t)64,
                           (uint64_t)table->capacity,
                           TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 26; i += 2)
  {
    err = UHashtableSet(table, i, (void*)(i * 1000));
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST_ID(i + 52),
                            err == NO_ERROR,
                            NO_ERROR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(4),
                           table->size == 26,
                           (uint64_t)26,
                           (uint64_t)table->size,
                           TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(5),
                           table->capacity == 64,
                           (uint64_t)64,
                           (uint64_t)table->capacity,
                           TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 26; ++i)
  {
    err = UHashtableGet(table, i, (void**)&data);
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST_ID(52 + i * 2),
                            err == NO_ERROR,
                            NO_ERROR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST_ID(52 + i * 2 + 1),
                             (uint64_t)(uintptr_t)data == i * (i % 2 == 0 ? 1000 : 10),
                             (uint64_t)i * (i % 2 == 0 ? 1000 : 10),
                             (uint64_t)(uintptr_t)data,
                             TEST_OS_UHASHTABLE_ENABLED)
  }

  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET_ID(2),
                           table->size == 26,
                           (uint64_t)26,
                           (uint64_t)table->size,
                           TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET_ID(3),
                           table->capacity == 64,
                           (uint64_t)64,
                           (uint64_t)table->capacity,
                           TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 26; ++i)
  {
    if(i % 2 == 0)
    {
      err = UHashtableRemove(table, i, NULL);
      TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_REMOVEBURST_ID(i),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_UHASHTABLE_ENABLED);
    }
  }
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE_ID(0),
                            table->size == 13,
                            (uint64_t)13,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE_ID(1),
                            table->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 30; ++i)
  {
    err = UHashtableGet(table, i, (void**)&data);

    if(err != NO_ERROR)
    {
      TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST_ID(104 + i * 2),
                              err == ERR_NOT_FOUND,
                              ERR_NOT_FOUND,
                              err,
                              TEST_OS_UHASHTABLE_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST_ID(104 + i * 2 + 1),
                                i % 2 == 0 || i > 25,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)i,
                                TEST_OS_UHASHTABLE_ENABLED);
    }
    else
    {
      TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST_ID(104 + i * 2 + 1),
                                (uint64_t)(uintptr_t)data == i * (i % 2 == 0 ? 1000 : 10),
                                (uint64_t)i * (i % 2 == 0 ? 1000 : 10),
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_UHASHTABLE_ENABLED);
    }
  }
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE_ID(2),
                            table->size == 13,
                            (uint64_t)13,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE_ID(3),
                            table->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  err = UHashtableDestroy(table);
  TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_DESTROY_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY_ID(1),
                            table->size == 0,
                            (uint64_t)0,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY_ID(2),
                            table->capacity == 0,
                            (uint64_t)0,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 30; ++i)
  {
    err = UHashtableGet(table, i, (void**)&data);
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST_ID(164 + i),
                            err == ERR_INVALID_PARAMETER,
                            ERR_INVALID_PARAMETER,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
  }

  table = UHashtableCreate(UHASHTABLE_ALLOCATOR(_Alloc, _Free), &err);
  TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_CREATE_ID(4),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(5),
                            table != NULL,
                            (uint64_t)1,
                            (uint64_t)(uintptr_t)table,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(6),
                            table->size == 0,
                            (uint64_t)0,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(7),
                            table->capacity == 16,
                            (uint64_t)16,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  uint32_t* table_data = _Alloc(sizeof(uint32_t) * 200);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE_ID(8),
                            table_data != NULL,
                            (uint64_t)1,
                            (uint64_t)(uintptr_t)table_data,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 200; ++i)
  {
      table_data[i] = _RandomGet();
      err = UHashtableSet(table, i, (void*)(uintptr_t)table_data[i]);
      TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST_ID(78 + i),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_UHASHTABLE_ENABLED);
  }
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(6),
                            table->size == 200,
                            (uint64_t)200,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET_ID(7),
                            table->capacity == 512,
                            (uint64_t)512,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  for(i = 0; i < 200; ++i)
  {
      err = UHashtableGet(table, i, (void**)&data);

      TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST_ID(194 + i * 2),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_UHASHTABLE_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST_ID(194 + i * 2 + 1),
                                (uint64_t)(uintptr_t)data == table_data[i],
                                (uint64_t)table_data[i],
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_UHASHTABLE_ENABLED)
  }

  err = UHashtableDestroy(table);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY_ID(3),
                            err == NO_ERROR,
                            (uint64_t)NO_ERROR,
                            (uint64_t)err,
                            TEST_OS_UHASHTABLE_ENABLED);

  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY_ID(4),
                            table->size == 0,
                            (uint64_t)0,
                            (uint64_t)table->size,
                            TEST_OS_UHASHTABLE_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY_ID(5),
                            table->capacity == 0,
                            (uint64_t)0,
                            (uint64_t)table->capacity,
                            TEST_OS_UHASHTABLE_ENABLED);

  _Free(table_data);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/