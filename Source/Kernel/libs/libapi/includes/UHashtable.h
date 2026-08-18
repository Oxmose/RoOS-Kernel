/*******************************************************************************
 * @file UHashtable.h
 *
 * @see UHashtable.c
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
 * @warning This implementation is not thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_UHASHTABLE_H_
#define __LIB_UHASHTABLE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <KernelHeap.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Unsigned hashtable allocator structure. */
typedef struct
{
  /**
   * @brief The memory allocation function used by the allocator.
   *
   * @param[in] kSize The number of byte to allocate.
   *
   * @return A pointer to the start address of the allocated memory is returned.
   */
  void*(*pMalloc)(const size_t kSize);
  /**
   * @brief The memory free function used by the allocator.
   *
   * @param[out] ptr The start address of the memory to free.
   */
  void(*pFree)(void* ptr);
} S_UHashtableAllocator;

/** @brief Unsigned hash table entry structure. */
typedef struct
{
  /** @brief Pointer sized key, contains a unsigned integer of the size of a
   * pointer.
   */
  uintptr_t key;
  /** @brief Data associated to the key. */
  void* pData;
  /** @brief Tells if the entry is used or not. */
  bool isUsed;
} S_UHashtableEntry;

/** @brief Unsigned hash table structure. */
typedef struct
{
  /** @brief Hash table allocator. */
  S_UHashtableAllocator allocator;
  /** @brief Hash table entries. */
  S_UHashtableEntry** ppEntries;
  /** @brief Current hash table capacity. */
  size_t capacity;
  /** @brief Current hash table size. */
  size_t size;
  /** @brief Contains the number of deleted item still in the table. */
  size_t graveyardSize;
} S_UHashtable;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Create an allocator structure.
 *
 * @param[in] MALLOC The memory allocation function used by the allocator.
 * @param[in] FREE The memory free function used by the alloctor.
 */
#define UHASHTABLE_ALLOCATOR(MALLOC, FREE) (S_UHashtableAllocator){MALLOC, FREE}

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
/**
 * @brief Creates a new unsigned hash table.
 *
 * @details Creates a new unsigned hash table. The hash table will be
 * initialized and its memory allocated. It should be destroyed once the hash
 * table is no longer in use.
 *
 * @param[in] allocator The allocator to be used when allocating and freeing the
 * hash table.
 * @param[out] pError The buffer to return the error status.
 *
 * @return A pointer to the newly created table is returned.
 */
S_UHashtable* UHashtableCreate(S_UHashtableAllocator allocator,
                               E_Return*             pError);

/**
 * @brief Destroys an unsigned hash table.
 *
 * @details Destroys an unsigned hash table. The memory used by the table is
 * released. It is the responsability of the user to free the memory used by the
 * data contained in the hash table.
 *
 * @param[out] pTable The unsigned hash table to destroy.
 *
 * @return The error status is returned.
 */
E_Return UHashtableDestroy(S_UHashtable* pTable);

/**
 * @brief Returns the value attached to the key provided in parameters.
 *
 * @details Returns the value attached to the key provided in parameters. If the
 * key is not in the table, OS_ERR_INCORRECT_VALUE is returned.
 *
 * @param[in] pTable The table to search.
 * @param[in] kKey The key to search.
 * @param[out] ppData The data buffer to receive the data associated to the key.
 *
 * @return The error status is returned.
 */
E_Return UHashtableGet(const S_UHashtable* pTable,
                       const uintptr_t     kKey,
                       void**              ppData);

/**
 * @brief Sets a value in the hash table.
 *
 * @details Sets a value in the hash table. If the value already exists, the
 * previous value is overwritten, otherwise, a new entry is created.
 *
 * @param[in,out] pTable The table to set the data into.
 * @param[in] kKey The key to associate to the data.
 * @param[in] pData The data to set in the table.
 *
 * @return The error status is returned.
 */
E_Return UHashtableSet(S_UHashtable* pTable, const uintptr_t kKey, void* pData);

/**
 * @brief Removes an entry from the table.
 *
 * @details Removes an entry from the table. This function returns the value
 * attached to the key provided in parameters. If the
 * key is not in the table, OS_ERR_INCORRECT_VALUE is returned.
 *
 * @param[in] pTable The table to remove the entry in.
 * @param[in] kKey The key to remove.
 * @param[out] ppData The data buffer to receive the data associated to the key.
 * This parameter can be set to NULL is the user does not want to retreive the
 * removed data.
 *
 * @return The error status is returned.
 */
E_Return UHashtableRemove(S_UHashtable*   pTable,
                          const uintptr_t kKey,
                          void**          ppData);

#endif /* #ifndef __LIB_UHASHTABLE_H_ */

/************************************ EOF *************************************/