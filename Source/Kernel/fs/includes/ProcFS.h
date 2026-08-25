/*******************************************************************************
 * @file ProcFS.h
 *
 * @see ProcFS.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 13/10/2025
 *
 * @version 1.0
 *
 * @brief Kernel's process filesystem driver.
 *
 * @details Kernel's process filesystem driver. Defines the functions and
 * structures used by the kernel to manage the procfs entries.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __FS_PROCFS_H_
#define __FS_PROCFS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <VirtualFS.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the file operations for a procfs entry. */
typedef struct
{
  /** @brief FS open function, see T_VFSOpen type for more information */
  T_VFSOpen pOpen;
  /** @brief FS close function, see T_VFSClose type for more information */
  T_VFSClose pClose;
  /** @brief FS read function, see T_VFSRead type for more information */
  T_VFSRead pRead;
  /** @brief FS write function, see T_VFSWrite type for more information */
  T_VFSWrite pWrite;
  /** @brief FS read dir function, see T_VFSReadDir type for more information */
  T_VFSReadDir pReadDir;
  /** @brief FS ioctl function, see T_VFSIOCTL type for more information */
  T_VFSIOCTL pIOCTL;
} S_ProcFSFileOperations;

/** @brief Defines how a procfs entry is represented in the kernel. */
typedef struct S_ProcFSDirEntry
{
  /** @brief Name of the procfs entry. */
  char* name;
  /** @brief Open mode of the procfs entry. */
  int32_t mode;
  /** @brief File operations registered for this procfs entry. */
  S_ProcFSFileOperations* fops;
  /**
   * @brief Parent of the procfs entry. Can be NULL is the entry is located
   * at the root of the procfs.
   */
  struct S_ProcFSDirEntry* pParent;
  /** @brief Next entry in the procfs entry directory. */
  struct S_ProcFSDirEntry* pNext;
  /**
   * @brief Sub directories of the procfs entry in case the entry is a
   * directory.
   */
  struct S_ProcFSDirEntry* pSubDir;
  /** @brief ProcFS drivers internal data. */
  void* pData;
} S_ProcFSDirEntry;

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
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Initialize the procfs driver.
 *
 * @details Initializes the procfs driver. Create the root entry in the
 * filesystem.
 */
void ProcFSInit(void);

/**
 * @brief Creates a procfs directory.
 *
 * @details Creates a procfs directory. A procfs directory has no file operation
 * and has for only purpose to gather other directories or entries.
 *
 * @param[in] kpName The name of the directory to create.
 * @param[in] kpParentName The path of the parent directory to this entry. If
 * NULL, the new directory will be created at the root of the procfs.
 * @param[out] ppDirectory The structure created for the directory is filled
 * in this buffer. The value can be NULL in case of error.
 *
 * @return The function returns the error or success status.
 */
E_Return ProcFSCreateDir(const char*        kpName,
                         const char*        kpParentPath,
                         S_ProcFSDirEntry** ppDirectory);

/**
 * @brief Removes a procfs directory.
 *
 * @details Removes a procfs directory. The directory must be empty to be
 * removed.
 *
 * @param[in, out] ppDirectory The directory structure of the directory to
 * remove.
 *
 * @return The function returns the error or success status.
 */
E_Return ProcFSRemoveDir(S_ProcFSDirEntry** ppDirectory);

/**
 * @brief Creates a new procfs entry.
 *
 * @details Creates a new procfs entry. The entry is created within the
 * specified directory and uses the file operations provided by the user.
 *
 * @param[in] kpName The name of the new procfs entry.
 * @param[in] kMode The open mode of the new procfs entry.
 * @param[in] pParent The structure of the parent procfs entry. If NULL, the
 * new entry will be created at the root of the procfs.
 * @param[in] pFops File operations used by the new procfs entry.
 * @param[in] pExtraData Extra data to be used by the procfs entry. This data is
 * passed to the file operations when they are called.
 * @param[out] pEntry The structure created for the entry is filled in this
 * buffer. The value can be NULL in case of error.
 *
 * @return The function returns the error or success status.
 */
E_Return ProcFSCreateEntry(const char*             kpName,
                           const uint32_t          kMode,
                           S_ProcFSDirEntry*       pParent,
                           S_ProcFSFileOperations* pFops,
                           void*                   pExtraData,
                           S_ProcFSDirEntry**      pEntry);

/**
 * @brief Removes an existing procfs entry.
 *
 * @details Removes an existing procfs entry. All file descriptors used in this
 * entry will return invalid codes once the entry is removed.
 *
 * @param[in] kpName The name of the procfs entry to remove.
 * @param[in] pParent The structure of the parent directory of the procfs entry
 * to remove. If NULL, the parent is considered the root of the procfs.
 *
 * @return The function returns the error or success status.
 */
E_Return ProcFSRemoveEntry(const char*       kpName,
                           S_ProcFSDirEntry* pParent);

#endif /* #ifndef __FS_PROCFS_H_ */

/************************************ EOF *************************************/