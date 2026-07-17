/*******************************************************************************
 * @file UHashtable.c
 *
 * @see UHashtable.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.0
 *
 * @brief Unsigned hash table structures.
 *
 * @details Unsigned hash table structures. Hash table are used to dynamically
 * store data, while growing when needed. This type of hash table can store data
 * pointers and values of the size of a pointer.
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
#include <KernelHeap.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <UHashtable.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/**
 * @brief Initial capacity and size of the hashtable.
 *
 * @warning Must be a power of 2.
 */
#define HT_INITIAL_SIZE 16
/** @brief Maximal factor size of the graveyard. */
#define HT_MAX_GRAVEYARD_FACTOR 0.3f
/**
 * @brief Maximal load factor (including graveyard).
 *
 * @warning: Must always be strictly less than 1.0
*/
#define HT_MAX_LOAD_FACTOR 0.7f
/** @brief FNV offset used for the hash function. */
#define FNV_OFFSET_BASIS 14695981039346656037UL
/** @brief Prime used in the FNV hash. */
#define FNV_PRIME 1099511628211UL

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief 64 bits hash function for uintptr_t keys.
 *
 * @details This function computes the 64 bits hash function for uintptr_t keys.
 * The algorithm used is the FNV-1a hash algorithm.
 *
 * @param[in] kKey The key to hash.
 *
 * @return The computed hash is returned.
 */
static inline uint64_t _UHash64(const uintptr_t kKey);

/**
 * @brief Sets the data for a given entry in the unsigned hash table.
 *
 * @details Sets the data for a given entry in the unsigned hash table. If the
 * entry does not exist it is created.
 *
 * @warning This function does not perform any check on the data, the key or the
 * table itself.
 *
 * @param[in,out] pTable The table to set the value.
 * @param[in] kKey The key to associate the data with.
 * @param[in] pData The data to set.
 *
 * @return The error status is returned.
 */
static E_Return _UHashtableSetEntry(S_UHashtable*   pTable,
                                    const uintptr_t kKey,
                                    void*           pData);

/**
 * @brief Replaces an entry in the table, without allocating the entry.
 *
 * @details Replaces an entry in the table, without allocating the entry. The
 * entry beging used it the one given as parameter.
 *
 * @param[out] pTable The table to use.
 * @param[in] pEntry The entry to place.
 */
static void _UHashtableRehashEntry(S_UHashtable*      pTable,
                                   S_UHashtableEntry* pEntry);

/**
 * @brief Rehashes the table and grows the table by a certain factor.
 *
 * @details Rehash the table and grows the table by a certain factor. The growth
 * factor must be greater or equal to 1. In the latter case, only the rehashing
 * is done.
 *
 * @param[out] pTable The table to grow.
 * @param[in] kGrowth The growth factor to use.
 *
 * @return The error status is returned.
 */
static E_Return _UHashtableRehash(S_UHashtable* pTable, const float kGrowth);

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
static inline uint64_t _UHash64(const uintptr_t kKey)
{
  uint64_t hash;
  size_t   i;

  /* We use the FNV-1a method to hash our integer */
  hash = FNV_OFFSET_BASIS;

  for (i = 0; i < sizeof(uintptr_t); ++i)
  {
    hash ^= (uint64_t)((uint8_t)(kKey >> (i * 8)));
    hash *= FNV_PRIME;
  }

  return hash;
}

static E_Return _UHashtableSetEntry(S_UHashtable*   pTable,
                                    const uintptr_t kKey,
                                    void*           pData)
{
  uint64_t hash;
  size_t   entryIdx;
  bool     set;
  E_Return error;

  /* Get the hash and ensure it does not overflow the current capacity */
  hash     = _UHash64(kKey);
  entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

  /* Search for the entry in the table. Note that due to the fact that our
   * load factor should always be under 1.0, there is at least one entry that
   * is NULL. Hence we cannot have an infinite loop here. */
  set = false;
  while (pTable->ppEntries[entryIdx] != NULL &&
        pTable->ppEntries[entryIdx]->isUsed == true)
  {
    /* Found the entry */
    if (pTable->ppEntries[entryIdx]->key == kKey)
    {
      pTable->ppEntries[entryIdx]->pData = pData;
      set = true;
      error = NO_ERROR;
      break;
    }

    /* Increment the index */
    entryIdx = (entryIdx + 1) % pTable->capacity;
  }

  if (set == false)
  {
    /* If we are here, we did not find the data, allocate if needed */
    if (pTable->ppEntries[entryIdx] == NULL)
    {
      pTable->ppEntries[entryIdx] =
        pTable->allocator.pMalloc(sizeof(S_UHashtableEntry));
      if (pTable->ppEntries[entryIdx] != NULL)
      {
        set = true;
      }
      else
      {
        error = ERR_NO_MEMORY;
      }
    }
    else
    {
      /* We found an entry from the graveyard, remove it */
      --pTable->graveyardSize;
      set = true;
    }

    if (set == true)
    {
      ++pTable->size;

      /* Set the data */
      pTable->ppEntries[entryIdx]->key    = kKey;
      pTable->ppEntries[entryIdx]->pData  = pData;
      pTable->ppEntries[entryIdx]->isUsed = true;

      error = NO_ERROR;
    }
  }

  return error;
}

static void _UHashtableRehashEntry(S_UHashtable*      pTable,
                                   S_UHashtableEntry* pEntry)
{
  uint64_t hash;
  bool     set;
  size_t   entryIdx;

  /* Get the hash and ensure it does not overflow the current capacity */
  hash     = _UHash64(pEntry->key);
  entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

  /* Search for the entry in the table. Note that due to the fact that our
   * load factor should always be under 1.0, there is at least one entry that
   * is NULL. Hence we cannot have an infinite loop here. */
  set = false;
  while (pTable->ppEntries[entryIdx] != NULL &&
        pTable->ppEntries[entryIdx]->isUsed == true)
  {
    /* Found the entry */
    if (pTable->ppEntries[entryIdx]->key == pEntry->key)
    {
      pTable->ppEntries[entryIdx]->pData = pEntry->pData;
      set = true;
      break;
    }

    /* Increment the index */
    entryIdx = (entryIdx + 1) % pTable->capacity;
  }

  if (set == false)
  {
    /* Set the data */
    pTable->ppEntries[entryIdx] = pEntry;
  }
}

static E_Return _UHashtableRehash(S_UHashtable* pTable, const float kGrowth)
{
  size_t              i;
  size_t              newCapacity;
  size_t              oldCapacity;
  S_UHashtableEntry** ppNewEnt;
  S_UHashtableEntry** ppOldEnt;
  E_Return            error;

  newCapacity = (size_t)((float)pTable->capacity * kGrowth);

  /* Check if did not overflow on the size */
  if (newCapacity >= pTable->capacity)
  {
    /* Save old entries */
    ppOldEnt    = pTable->ppEntries;
    oldCapacity = pTable->capacity;

    /* Create a new entry table */
    ppNewEnt = pTable->allocator.pMalloc(sizeof(S_UHashtableEntry*) *
                                         newCapacity);
    if (ppNewEnt != NULL)
    {
      memset(ppNewEnt, 0, sizeof(S_UHashtableEntry*) * newCapacity);
      pTable->capacity  = newCapacity;
      pTable->ppEntries = ppNewEnt;

      /* Rehash the table, removes fragmentation and graveyard. */
      for (i = 0; i < oldCapacity; ++i)
      {
        if (ppOldEnt[i] != NULL)
        {
          /* Check if it was an entry that was used */
          if (ppOldEnt[i]->isUsed == true)
          {
            _UHashtableRehashEntry(pTable, ppOldEnt[i]);
          }
          else
          {
            /* The entry was not used, we can free it */
            pTable->allocator.pFree(ppOldEnt[i]);
          }
        }
      }

      /* Clean data */
      pTable->graveyardSize = 0;
      pTable->allocator.pFree(ppOldEnt);

      error = NO_ERROR;
    }
    else
    {
      error = ERR_NO_MEMORY;
    }
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;;
  }

  return error;
}

S_UHashtable* UHashtableCreate(S_UHashtableAllocator allocator,
                               E_Return*             pError)
{
  S_UHashtable* pTable;

  pTable = NULL;

  if (allocator.pFree != NULL && allocator.pMalloc != NULL)
  {
    /* Initialize the table */
    pTable = allocator.pMalloc(sizeof(S_UHashtable));
    if (pTable != NULL)
    {
      pTable->ppEntries = allocator.pMalloc(sizeof(S_UHashtableEntry*) *
                                            HT_INITIAL_SIZE);

      if (pTable->ppEntries != NULL)
      {
        memset(pTable->ppEntries,
               0,
               sizeof(S_UHashtableEntry*) * HT_INITIAL_SIZE);
        pTable->allocator     = allocator;
        pTable->capacity      = HT_INITIAL_SIZE;
        pTable->size          = 0;
        pTable->graveyardSize = 0;

        *pError = NO_ERROR;
      }
      else
      {
        allocator.pFree(pTable);
        *pError = ERR_NO_MEMORY;
        pTable  = NULL;
      }
    }
    else
    {
      *pError = ERR_NO_MEMORY;
    }
  }
  else
  {
    *pError = ERR_INVALID_PARAMETER;
  }

  return pTable;
}

E_Return UHashtableDestroy(S_UHashtable* pTable)
{
  size_t   i;
  E_Return error;

  if (pTable != NULL && pTable->ppEntries != NULL)
  {
    /* Free the memory used by the table */
    for (i = 0; i < pTable->capacity; ++i)
    {
      if (pTable->ppEntries[i] != NULL)
      {
        pTable->allocator.pFree(pTable->ppEntries[i]);
      }
    }
    pTable->allocator.pFree(pTable->ppEntries);

    /* Reset attributes */
    pTable->size          = 0;
    pTable->capacity      = 0;
    pTable->graveyardSize = 0;
    pTable->ppEntries     = NULL;

    /* Free memory */
    pTable->allocator.pFree(pTable);

    error = NO_ERROR;
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

E_Return UHashtableGet(const S_UHashtable* pTable,
                       const uintptr_t     kKey,
                       void**              ppData)
{
  uint64_t hash;
  size_t   entryIdx;
  E_Return error;

  if (pTable != NULL && ppData != NULL && pTable->ppEntries != NULL)
  {
    /* Get the hash and ensure it does not overflow the current capacity */
    hash     = _UHash64(kKey);
    entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

    error = ERR_NOT_FOUND;

    /* Search for the entry in the table. Note that due to the fact that our
    * load factor should always be under 1.0, there is at least one entry that
    * is NULL. Hence we cannot have an infinite loop here. */
    while (pTable->ppEntries[entryIdx] != NULL)
    {
      /* Found the entry */
      if (pTable->ppEntries[entryIdx]->key == kKey &&
        pTable->ppEntries[entryIdx]->isUsed == true)
      {
        *ppData = pTable->ppEntries[entryIdx]->pData;
        error = NO_ERROR;
        break;
      }

      /* Increment the index */
      entryIdx = (entryIdx + 1) % pTable->capacity;
    }
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

E_Return UHashtableSet(S_UHashtable*   pTable,
                       const uintptr_t kKey,
                       void*           pData)
{
  E_Return err;

  if (pTable != NULL && pTable->ppEntries != NULL)
  {
    /* Check if the current load is under the threshold, double the size */
    if ((size_t)((float)pTable->capacity * HT_MAX_LOAD_FACTOR) <=
        pTable->size + pTable->graveyardSize)
    {
      err = _UHashtableRehash(pTable, 2);
    }
    else
    {
      err = NO_ERROR;
    }

    if (err == NO_ERROR)
    {
      /* Insert the entry */
      err = _UHashtableSetEntry(pTable, kKey, pData);
    }
  }
  else
  {
    err = ERR_INVALID_PARAMETER;
  }

  return err;
}

E_Return UHashtableRemove(S_UHashtable*   pTable,
                          const uintptr_t kKey,
                          void**          ppData)
{
  uint64_t    hash;
  size_t      entryIdx;
  E_Return err;

  if (pTable != NULL && pTable->ppEntries != NULL)
  {
    /* Get the hash and ensure it does not overflow the current capacity */
    hash     = _UHash64(kKey);
    entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

    /* Check if the graveyard load is under the threshold */
    if ((float)pTable->capacity * HT_MAX_GRAVEYARD_FACTOR <
        pTable->graveyardSize)
    {
      /* Rehash with a growth factor or 1 to keep the same capacity */
      err = _UHashtableRehash(pTable, 1);
    }
    else
    {
      err = NO_ERROR;
    }

    if (err == NO_ERROR)
    {
      err = ERR_NOT_FOUND;

      /* Search for the entry in the table. Note that due to the fact that our
       * load factor should always be under 1.0, there is at least one entry
       * that is NULL. Hence we cannot have an infinite loop here. */
      while (pTable->ppEntries[entryIdx] != NULL)
      {
        /* Found the entry */
        if (pTable->ppEntries[entryIdx]->key == kKey &&
           pTable->ppEntries[entryIdx]->isUsed == true)
        {
          if (ppData != NULL)
          {
            *ppData = pTable->ppEntries[entryIdx]->pData;
          }
          pTable->ppEntries[entryIdx]->isUsed = false;

          ++pTable->graveyardSize;
          --pTable->size;

          err = NO_ERROR;
          break;
        }

        /* Increment the index */
        entryIdx = (entryIdx + 1) % pTable->capacity;
      }
    }
  }
  else
  {
    err = ERR_INVALID_PARAMETER;
  }

  return err;
}

/************************************ EOF *************************************/