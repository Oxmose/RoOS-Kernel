/*******************************************************************************
 * @file VectorTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework vector testing.
 *
 * @details Testing framework vector testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <Vector.h>
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
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void* _Alloc(const size_t kSize);
static void _Free(void* ptr);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _Alloc(const size_t kSize)
{
  void* addr;
  addr = KMalloc(kSize, ALIGN_ADDRESS, KMALLOC_FREE_POOL);
  return addr;
}

static void _Free(void* ptr)
{
  KFree(ptr, KMALLOC_FREE_POOL);
}

void VectorTest(void)
{
  size_t i;
  uint64_t data;
  uint64_t data2;
  S_Vector* vector;
  S_Vector* vector_cpy;
  E_Return err;

  vector = VectorCreate(VECTOR_ALLOCATOR(_Alloc, _Free), (void*)0, 0, &err);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_CREATE_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CREATE_ID(1),
                            vector != NULL,
                            (uint64_t)1,
                            (uint64_t)(uintptr_t)vector,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CREATE_ID(2),
                            vector->size == 0,
                            (uint64_t)0,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CREATE_ID(3),
                            vector->capacity == 0,
                            (uint64_t)0,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < 20; ++i)
  {
      err = VectorPush(vector, (void*)i);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_PUSHBURST_ID(i * 2),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_PUSHBURST_ID(i * 2 + 1),
                                vector->size == i + 1,
                                (uint64_t)i + 1,
                                (uint64_t)(uintptr_t)vector->size,
                                TEST_OS_VECTOR_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(0),
                            vector->size == 20,
                            (uint64_t)20,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(1),
                            vector->capacity == 32,
                            (uint64_t)32,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; ++i)
  {
      VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 2),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 2 + 1),
                                (uint64_t)(uintptr_t)data == i,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }

  for(i = 0; i < 30; i += 2)
  {
      err = VectorInsert(vector, (void*)(i + 100), i);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_INSERTBURST_ID(i * 2),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_INSERTBURST_ID(i * 2 + 1),
                                (uint64_t)(uintptr_t)vector->size == i / 2 + 20 + 1,
                                (uint64_t)i / 2 + 20 + 1,
                                (uint64_t)(uintptr_t)vector->size,
                                TEST_OS_VECTOR_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_INSERT_ID(0),
                            vector->size == 35,
                            (uint64_t)35,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_INSERT_ID(1),
                            vector->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; ++i)
  {
      VectorGet(vector, i, (void**)&data);

      if(i < 30)
      {
          data2 = (i % 2 ? i / 2 : i + 100);
      }
      else
      {
          data2 = 14 + i - 29;
      }
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 3 + 100),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 3 + 101),
                                (uint64_t)(uintptr_t)data == (uint64_t)(uintptr_t)vector->ppArray[i],
                                (uint64_t)(uintptr_t)vector->ppArray[i],
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 3 + 102),
                                (uint64_t)(uintptr_t)data == data2,
                                (uint64_t)data2,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(2),
                            vector->size == 35,
                            (uint64_t)35,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(3),
                            vector->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < 6; i++)
  {
      data2 = 19 - i;

      err = VectorPop(vector, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_POPBURST_ID(i * 2),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_POPBURST_ID(i * 2 + 1),
                                (uint64_t)(uintptr_t)data == data2,
                                (uint64_t)data2,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_POP_ID(0),
                            vector->size == 29,
                            (uint64_t)29,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_POP_ID(1),
                            vector->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; ++i)
  {
      if(i < 35)
      {
          data2 = (i % 2 ? (i / 2) : i + 100);
      }
      else
      {
          data2 = 14 + i - 29;
      }

      VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 2 + 300),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 2 + 301),
                                (uint64_t)(uintptr_t)data == data2,
                                (uint64_t)data2,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);

  }

  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(4),
                            vector->size == 29,
                            (uint64_t)29,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(5),
                            vector->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; i++)
  {
      err = VectorSet(vector, i, (void*)i);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_SETBURST_ID(i * 2),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_SETBURST_ID(i * 2 + 1),
                                (uint64_t)(uintptr_t)i == (uint64_t)(uintptr_t)vector->ppArray[i],
                                (uint64_t)(uintptr_t)vector->ppArray[i],
                                (uint64_t)(uintptr_t)i,
                                TEST_OS_VECTOR_ENABLED);
  }

  for(i = 0; i < vector->size; ++i)
  {
      VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 2 + 400),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 2 + 401),
                                (uint64_t)(uintptr_t)data == i,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }

  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(6),
                            vector->size == 29,
                            (uint64_t)29,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET_ID(7),
                            vector->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  err = VectorResize(vector, 20);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_RESIZE_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE_ID(1),
                            vector->size == 20,
                            (uint64_t)20,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE_ID(2),
                            vector->capacity == 64,
                            (uint64_t)64,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; ++i)
  {
      VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 2 + 500),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 2 + 501),
                                (uint64_t)(uintptr_t)data == i,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }

  err = VectorResize(vector, 80);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_RESIZE_ID(3),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE_ID(4),
                            vector->size == 80,
                            (uint64_t)80,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE_ID(5),
                            vector->capacity == 80,
                            (uint64_t)80,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < 20; ++i)
  {
      VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 2 + 600),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 2 + 601),
                                (uint64_t)(uintptr_t)data == i,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }

  err = VectorResize(vector, 20);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_RESIZE_ID(6),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE_ID(7),
                            vector->size == 20,
                            (uint64_t)20,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE_ID(8),
                            vector->capacity == 80,
                            (uint64_t)80,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  err = VectorSrink(vector);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_SHRINK_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_SHRINK_ID(1),
                            vector->size == 20,
                            (uint64_t)20,
                            (uint64_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_SHRINK_ID(2),
                            vector->capacity == 20,
                            (uint64_t)20,
                            (uint64_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; ++i)
  {
      VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 2 + 700),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 2 + 701),
                                (uint64_t)(uintptr_t)data == i,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }

  vector_cpy = VectorCopy(vector, &err);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_COPY_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_COPY_ID(1),
                            vector_cpy != NULL,
                            (uint64_t)1,
                            (uint64_t)(uintptr_t)vector_cpy,
                            TEST_OS_VECTOR_ENABLED);

  for(i = 0; i < vector->size; ++i)
  {
      err = VectorGet(vector, i, (void**)&data);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 4 + 800),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      err = VectorGet(vector_cpy, i, (void**)&data2);
      TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST_ID(i * 4 + 801),
                              err == NO_ERROR,
                              NO_ERROR,
                              err,
                              TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 4 + 802),
                                (uint64_t)(uintptr_t)data == (uint64_t)(uintptr_t)data2,
                                (uint64_t)(uintptr_t)data,
                                (uint64_t)(uintptr_t)data2,
                                TEST_OS_VECTOR_ENABLED);
      TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST_ID(i * 4 + 803),
                                (uint64_t)(uintptr_t)data == i,
                                (uint64_t)i,
                                (uint64_t)(uintptr_t)data,
                                TEST_OS_VECTOR_ENABLED);
  }
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_COPY_ID(2),
                            vector->size == vector_cpy->size,
                            (uint64_t)vector->size,
                            (uint64_t)(uintptr_t)vector_cpy->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_COPY_ID(3),
                            vector->capacity == vector_cpy->capacity,
                            (uint64_t)vector->capacity,
                            (uint64_t)(uintptr_t)vector_cpy->capacity,
                            TEST_OS_VECTOR_ENABLED);

  err = VectorClear(vector);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_CLEAR_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CLEAR_ID(1),
                            vector->size == 0,
                            (uint64_t)0,
                            (uint64_t)(uintptr_t)vector->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CLEAR_ID(2),
                            vector->capacity == 20,
                            (uint64_t)20,
                            (uint64_t)(uintptr_t)vector->capacity,
                            TEST_OS_VECTOR_ENABLED);

  err = VectorDestroy(vector_cpy);
  TEST_POINT_ASSERT_RCODE(TEST_VECTOR_DESTROY_ID(0),
                          err == NO_ERROR,
                          NO_ERROR,
                          err,
                          TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_DESTROY_ID(1),
                            vector_cpy->size == 0,
                            (uint64_t)0,
                            (uint64_t)(uintptr_t)vector_cpy->size,
                            TEST_OS_VECTOR_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_DESTROY_ID(2),
                            vector_cpy->capacity == 0,
                            (uint64_t)0,
                            (uint64_t)(uintptr_t)vector_cpy->capacity,
                            TEST_OS_VECTOR_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/