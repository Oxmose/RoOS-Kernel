/*******************************************************************************
 * @file Vector.c
 *
 * @see Vector.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.0
 *
 * @brief Vector structures.
 *
 * @details Vector structures. Vectors are used to dynamically store data, while
 * growing when needed. This type of vector can store data pointers and values
 * of the size of a pointer.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <Vector.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/**
 * @brief Growth factor used when the vector has not space left.
 *
 * @warning This value must be greater than 1.
*/
#define VECTOR_GROWTH_FACTOR 2

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Used to grow the size of a vector. The new vector is filled with
 * previous data.
 *
 * @param[out] VECTOR The vector to update.
 * @param[out] NEW_SIZE The new size to be computed.
 * @param[out] NEW_ARRAY The array to receive the created memory region.
*/
#define GROW_VECTOR_SIZE(VECTOR, NEW_SIZE, NEW_ARRAY) {                   \
  if (VECTOR->capacity == VECTOR->size)                                   \
  {                                                                       \
    NEW_SIZE = VECTOR->capacity * VECTOR_GROWTH_FACTOR;                   \
    if (NEW_SIZE == 0)                                                    \
    {                                                                     \
      NEW_SIZE = VECTOR_GROWTH_FACTOR;                                    \
    }                                                                     \
                                                                          \
    /* Check if did not overflow on the size */                           \
    if (NEW_SIZE <= VECTOR->capacity)                                     \
    {                                                                     \
      return ERR_EXCEEDED_LIMIT;                                          \
    }                                                                     \
                                                                          \
    /* Allocate new array */                                              \
    NEW_ARRAY = VECTOR->allocator.pMalloc(NEW_SIZE * sizeof(void*),       \
                                          VECTOR->allocator.pMeta);       \
    if (NEW_ARRAY == NULL)                                                \
    {                                                                     \
      return ERR_NO_MEMORY;                                               \
    }                                                                     \
                                                                          \
    /* Free old array */                                                  \
    if (VECTOR->ppArray != NULL)                                          \
    {                                                                     \
      /* Copy array */                                                    \
      memcpy(NEW_ARRAY, VECTOR->ppArray, VECTOR->size * sizeof(void*));   \
      VECTOR->allocator.pFree(VECTOR->ppArray, VECTOR->allocator.pMeta);  \
    }                                                                     \
    VECTOR->ppArray  = NEW_ARRAY;                                         \
    VECTOR->capacity = NEW_SIZE;                                          \
  }                                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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
 * FUNCTIONS
 ******************************************************************************/
S_Vector* VectorCreate(S_VectorAllocator allocator,
                       void*             pInitData,
                       const size_t      kSize,
                       E_Return*         pError)
{
  S_Vector* pVector;
  size_t    i;

  if (allocator.pMalloc != NULL && allocator.pFree != NULL)
  {
    *pError = NO_ERROR;

    pVector = allocator.pMalloc(sizeof(S_Vector), allocator.pMeta);
    if (pVector != NULL)
    {
      /* Allocate the data */
      pVector->ppArray = NULL;
      if (kSize != 0)
      {
        pVector->ppArray = allocator.pMalloc(kSize * sizeof(void*),
                                             allocator.pMeta);
        if (pVector->ppArray == NULL)
        {
          allocator.pFree(pVector, allocator.pMeta);
          pVector = NULL;
          *pError = ERR_NO_MEMORY;
        }
      }

      if (*pError == NO_ERROR)
      {
        /* Initialize the data */
        for (i = 0; i < kSize; ++i)
        {
          pVector->ppArray[i] = pInitData;
        }

        /* Initialize the attributes */
        pVector->allocator = allocator;
        pVector->size      = kSize;
        pVector->capacity  = kSize;
      }
    }
    else
    {
      *pError = ERR_NO_MEMORY;
    }
  }
  else
  {
    pVector = NULL;
    *pError = ERR_INVALID_PARAMETER;
  }

  return pVector;
}

E_Return VectorDestroy(S_Vector* pVector)
{
  E_Return retCode;

  if (pVector != NULL)
  {
    /* Release the data */
    if (pVector->ppArray != NULL)
    {
      pVector->allocator.pFree(pVector->ppArray, pVector->allocator.pMeta);
    }

    /* Reset the attributes */
    pVector->ppArray   = NULL;
    pVector->size      = 0;
    pVector->capacity  = 0;

    /* Free vector structure */
    pVector->allocator.pFree(pVector, pVector->allocator.pMeta);

    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorClear(S_Vector* pVector)
{
  E_Return retCode;

  if (pVector != NULL)
  {
    pVector->size = 0;
    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

S_Vector* VectorCopy(const S_Vector* pSrc, E_Return* pError)
{
  S_Vector* pCopy;

  if (pSrc != NULL)
  {
    pCopy = VectorCreate(pSrc->allocator, NULL, pSrc->capacity, pError);
    if (pCopy != NULL && *pError == NO_ERROR)
    {
      /* Here we do not need to copy the entire array, just the part that contains
       * valid data as size <= capacity.
       */
      pCopy->size = pSrc->size;
      memcpy(pCopy->ppArray, pSrc->ppArray, pSrc->size * sizeof(void*));
    }
  }
  else
  {
    pCopy   = NULL;
    *pError = ERR_INVALID_PARAMETER;
  }

  return pCopy;
}

E_Return VectorSrink(S_Vector* pVector)
{
  E_Return retCode;
  void*    pNewArray;

  if (pVector != NULL)
  {
    retCode = NO_ERROR;

    /* Only resize if the capacity is different than the size */
    if (pVector->capacity > pVector->size)
    {
      if (pVector->size != 0)
      {
        /* Allocate new array */
        pNewArray = pVector->allocator.pMalloc(pVector->size * sizeof(void*),
                                               pVector->allocator.pMeta);
        if (pNewArray != NULL)
        {
          /* Copy array */
          memcpy(pNewArray, pVector->ppArray, pVector->size * sizeof(void*));

          /* Free old array */
          pVector->allocator.pFree(pVector->ppArray, pVector->allocator.pMeta);
          pVector->ppArray  = pNewArray;
          pVector->capacity = pVector->size;
        }
        else
        {
          retCode = ERR_NO_MEMORY;
        }
      }
      else
      {
        /* Free all memory */
        pVector->allocator.pFree(pVector->ppArray, pVector->allocator.pMeta);
        pVector->ppArray  = NULL;
        pVector->capacity = 0;
      }
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorResize(S_Vector* pVector, const size_t kSize)
{
  E_Return retCode;
  void*    pNewArray;

  if (pVector != NULL)
  {
    retCode = NO_ERROR;

    /* Only resize if the capacity is different than the capacity */
    if (pVector->capacity < kSize)
    {
      if (kSize != 0)
      {
        /* Allocate new array */
        pNewArray = pVector->allocator.pMalloc(kSize * sizeof(void*),
                                               pVector->allocator.pMeta);
        if (pNewArray != NULL)
        {
          /* Copy array */
          memcpy(pNewArray,
                 pVector->ppArray,
                 MAX(pVector->size, kSize) * sizeof(void*));

          /* Free old array */
          pVector->allocator.pFree(pVector->ppArray, pVector->allocator.pMeta);
          pVector->ppArray  = pNewArray;
          pVector->capacity = kSize;
          pVector->size     = kSize;
        }
        else
        {
          retCode = ERR_NO_MEMORY;
        }
      }
      else
      {
        /* Free all memory */
        pVector->allocator.pFree(pVector->ppArray, pVector->allocator.pMeta);
        pVector->ppArray  = NULL;
        pVector->capacity = 0;
        pVector->size     = 0;
      }
    }
    else
    {
      pVector->size = kSize;
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorInsert(S_Vector* pVector, void* pData, const size_t kPosition)
{
  E_Return retCode;
  size_t   newSize;
  size_t   i;
  void**   ppNewArray;

  if (pVector != NULL && kPosition < pVector->size)
  {
    /* First, check if we should update the capacity of the vector */
    GROW_VECTOR_SIZE(pVector, newSize, ppNewArray);

    /* Move the old data and insert the new data */
    for (i = pVector->size; i > kPosition; --i)
    {
      pVector->ppArray[i] = pVector->ppArray[i - 1];
    }
    pVector->ppArray[kPosition] = pData;
    ++pVector->size;

    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorPush(S_Vector* pVector, void* pData)
{
  E_Return retCode;
  size_t   newSize;
  void**   ppNewArray;

  if (pVector != NULL)
  {
    /* First, check if we should update the capacity of the vector */
    GROW_VECTOR_SIZE(pVector, newSize, ppNewArray);

    /* Insert the new data */
    pVector->ppArray[pVector->size++] = pData;

    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorPop(S_Vector* pVector, void** ppData)
{
  E_Return retCode;

  if (pVector != NULL && ppData != NULL)
  {
    if (pVector->size != 0)
    {
      /* Return the last data */
      *ppData  = pVector->ppArray[--pVector->size];
      retCode = NO_ERROR;
    }
    else
    {
      retCode = ERR_EXCEEDED_LIMIT;
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorGet(const S_Vector* kpVector,
                   const size_t    kPosition,
                   void**          ppData)
{
  E_Return retCode;

  if (kpVector != NULL && ppData != NULL)
  {
    if (kpVector->size > kPosition)
    {
      /* Return the last data */
      *ppData  = kpVector->ppArray[kPosition];
      retCode = NO_ERROR;
    }
    else
    {
      retCode = ERR_EXCEEDED_LIMIT;
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VectorSet(S_Vector* pVector, const size_t kPosition, void* pData)
{
  E_Return retCode;

  if (pVector != NULL)
  {
    if (pVector->size > kPosition)
    {
      /* Return the last data */
      pVector->ppArray[kPosition] = pData;
      retCode                     = NO_ERROR;
    }
    else
    {
      retCode = ERR_EXCEEDED_LIMIT;
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

/************************************ EOF *************************************/