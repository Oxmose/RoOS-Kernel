/*******************************************************************************
 * @file ProcFS.c
 *
 * @see ProcFS.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <Panic.h>
#include <stdint.h>
#include <VirtualFS.h>
#include <KernelHeap.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <ProcFS.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "PROCFS"

/** @brief Stores the procsfs entry directory name */
#define PROCFS_ROOT_DIR_PATH "/proc"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the type of entry in the procfs */
typedef enum
{
  /** @brief Directory entry */
  PROCFS_ENTRY_DIR,
  /** @brief File entry */
  PROCFS_ENTRY_FILE
} E_ProcFSEntryType;

/** @brief Defines a procfs entry */
typedef struct
{
  /** @brief Type of the entry */
  E_ProcFSEntryType type;
  /** @brief Offset for directories */
  size_t offset;
  /** @brief Keeps open flags information. */
  int32_t openFlags;
  /**
   * @brief Entry structure that contains the file operations and other
   * attributes.
   */
  S_ProcFSDirEntry entryData;
  /** @brief Driver specific data passed to internal VFS functions. */
  void* pExtraData;
  /** @brief File specific data passed to internal VFS functions. */
  void* pFileData;
} S_ProcFSEntry;

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
#define PROCFS_ASSERT(COND, MSG, ERROR) {                 \
  if ((COND) == false)                                     \
  {                                                       \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);         \
  }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Copies a ProcsFS entry handle.
 *
 * @details Copies a ProcsFS entry handle. This will copy the atttribute and
 * make a deep copy of allocated objects.
 *
 * @param[out] pDstEntry The entry buffer that received the copy.
 * @param[in] kpSrcEntry The source entry to copy.
 *
 * @return The function returns the success or error status.
 */
static E_Return _CopyEntryHandle(S_ProcFSEntry*       pDstEntry,
                                 const S_ProcFSEntry* kpSrcEntry);

 /**
 * @brief Opens a ProcFS entry.
 *
 * @details Opens a ProcFS entry. The function will find the entry if it exists
 * and fill the entry structure information.
 *
 * @param[in] pEntryLevel The current entry we are in the hierarchy.
 * @param[in] kpPath The path of the entry to open.
 * @param[in, out] pNextTokenIdx The index of the current token in the path.
 *
 * @return The function returns the found entry or NULL on error.
 */
static S_ProcFSEntry* _GetEntry(S_ProcFSEntry* pEntryLevel,
                                const char*    kpPath,
                                ssize_t*       pNextTokenIdx);

/**
 * @brief ProcFS entries open hook.
 *
 * @details ProcFS entries open hook. This function returns a
 * handle to control the procfs entries entries.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] kpPath The path of the entry to open.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _ProcFSVFSOpen(void*       pDrvCtrl,
                            const char* kpPath,
                            int         flags,
                            int         mode);

/**
 * @brief ProcFS entries close hook.
 *
 * @details ProcFS entries close hook. This function closes a
 * handle that was created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _ProcFSVFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief ProcFS entries write hook.
 *
 * @details ProcFS entries write hook.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _ProcFSVFSWrite(void*       pDrvCtrl,
                               void*       pHandle,
                               const void* kpBuffer,
                               size_t      count);

/**
 * @brief ProcFS entries read hook.
 *
 * @details ProcFS entries read hook.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _ProcFSVFSRead(void*  pDrvCtrl,
                              void*  pHandle,
                              void*  pBuffer,
                              size_t count);

/**
 * @brief ProcFS entries ReadDir hook.
 *
 * @details ProcFS entries ReadDir hook. This function performs
 * the ReadDir for the procfs driver.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _ProcFSVFSReadDir(void*             pDriverData,
                                 void*             pHandle,
                                 S_DirectoryEntry* pDirEntry);

/**
 * @brief ProcFS entries IOCTL hook.
 *
 * @details ProcFS entries IOCTL hook. This function performs
 *  the IOCTL for the procfs driver.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _ProcFSVFSIOCTL(void*    pDriverData,
                               void*    pHandle,
                               uint32_t operation,
                               void*    pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief ProcFS root and directory file operations */
static S_ProcFSFileOperations sProcFsFops =
{
  .pOpen    = NULL,
  .pClose   = NULL,
  .pRead    = NULL,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief ProcFS root entry. */
static S_ProcFSEntry sProcFsRootEntry =
{
  .pExtraData  = NULL,
  .pFileData   = NULL,
  .type        = PROCFS_ENTRY_DIR,
  .offset      = 0,
  .openFlags   = O_RDONLY,
  .entryData   =
  {
    .name    = "\0",
    .mode    = 0444,
    .fops    = &sProcFsFops,
    .pParent = NULL,
    .pNext   = NULL,
    .pSubDir = NULL,
    .pData   = &sProcFsRootEntry
  }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static E_Return _CopyEntryHandle(S_ProcFSEntry*       pDstEntry,
                                 const S_ProcFSEntry* kpSrcEntry)
{
  size_t   nameLen;
  E_Return returnVal;

  /* Copy the entry before replacing the borrowed name with an owned copy. */
  memcpy(pDstEntry, kpSrcEntry, sizeof(S_ProcFSEntry));

  nameLen = strnlen(kpSrcEntry->entryData.name, VFS_PATH_MAX_LENGTH);
  pDstEntry->entryData.name = KMallocUser(nameLen + 1, ALIGN_1_BYTE, NULL);
  if (pDstEntry->entryData.name != NULL)
  {
    memcpy(pDstEntry->entryData.name, kpSrcEntry->entryData.name, nameLen);
    pDstEntry->entryData.name[nameLen] = 0;

    /* Update the offset */
    pDstEntry->offset = 0;
    returnVal = NO_ERROR;
  }
  else
  {
    returnVal = ERR_NO_MEMORY;
  }

  return returnVal;
}

static S_ProcFSEntry* _GetEntry(S_ProcFSEntry* pEntryLevel,
                                const char*    kpPath,
                                ssize_t*       pNextTokenIdx)
{
  S_ProcFSEntry* pNextEntry;
  size_t         length;


  if (kpPath[0] != '/' && pEntryLevel != &sProcFsRootEntry)
  {
    /* Get the next token */
    length = strnlen(kpPath, VFS_PATH_MAX_LENGTH);
    *pNextTokenIdx = VFSGetNextPathTokenPosition(kpPath, length);

    if (*pNextTokenIdx > 0)
    {
      length = *pNextTokenIdx - 1;
    }

    /* Check if the token is valid */
    if (length > 0 && pEntryLevel != NULL)
    {
      /* Compare with current level */
      if (strnlen(pEntryLevel->entryData.name, VFS_FILENAME_MAX_LENGTH) ==
          length &&
          strncmp(pEntryLevel->entryData.name, kpPath, length) == 0)
      {
        /* Found the entry, check if this is a file or directory */
        if (pEntryLevel->type == PROCFS_ENTRY_DIR)
        {
          /* Check if we are at the end of the path */
          if (*(kpPath + length) != 0 &&
              *(kpPath + length + 1) != 0)
          {
            /* Continue searching */
            if (pEntryLevel->entryData.pSubDir != NULL)
            {
              pNextEntry = (S_ProcFSEntry*)
                            pEntryLevel->entryData.pSubDir->pData;
              pNextEntry = _GetEntry(pNextEntry,
                                      kpPath + *pNextTokenIdx,
                                      pNextTokenIdx);
            }
            else
            {
              pNextEntry = NULL;
            }
          }
          else
          {
            pNextEntry = pEntryLevel;
          }
        }
        else
        {
          pNextEntry = pEntryLevel;
        }
      }
      else
      {
        /* Go to next entry */
        if (pEntryLevel->entryData.pNext != NULL)
        {
          pNextEntry = (S_ProcFSEntry*)
                        pEntryLevel->entryData.pNext->pData;
          pNextEntry = _GetEntry(pNextEntry, kpPath, pNextTokenIdx);
        }
        else
        {
          pNextEntry = NULL;
        }
      }
    }
    else
    {
      pNextEntry = NULL;
    }
  }
  else
  {
    if (sProcFsRootEntry.entryData.pSubDir != NULL)
    {
      pNextEntry = (S_ProcFSEntry*)sProcFsRootEntry.entryData.pSubDir->pData;
      if (kpPath[0] == '/')
      {
        pNextEntry = _GetEntry(pNextEntry, kpPath + 1, pNextTokenIdx);
      }
      else
      {
        pNextEntry = _GetEntry(pNextEntry, kpPath, pNextTokenIdx);
      }
    }
    else
    {
      pNextEntry = NULL;
    }
  }
  return pNextEntry;
}

static void* _ProcFSVFSOpen(void*       pDrvCtrl,
                            const char* kpPath,
                            int         flags,
                            int         mode)
{
  S_ProcFSEntry*       pEntry;
  const S_ProcFSEntry* kpSourceEntry;
  ssize_t              nextTokenIdx;
  E_Return             returnVal;
  void*                pFileHandle;

  (void)pDrvCtrl;

  /* Allocate the new structure */
  pEntry = KMallocUser(sizeof(S_ProcFSEntry), ALIGN_ADDRESS, NULL);
  if (pEntry != NULL)
  {
    /* Check if we want to open an entry of a directory directory */
    if (kpPath[0] != 0)
    {
      /* Get the entry */
      nextTokenIdx = 0;

      /* Nothing to find */
      if (sProcFsRootEntry.entryData.pSubDir != NULL)
      {
        kpSourceEntry = _GetEntry(sProcFsRootEntry.entryData.pSubDir->pData,
                                  kpPath,
                                  &nextTokenIdx);
        if (nextTokenIdx < 0)
        {
          nextTokenIdx = strnlen(kpPath, VFS_PATH_MAX_LENGTH) + 1;
        }
      }
      else
      {
        kpSourceEntry = NULL;
      }

      if (kpSourceEntry != NULL)
      {
        /* If this is a file, it needs to be opened by the underlying
         * driver.
         */
        if (kpSourceEntry->type == PROCFS_ENTRY_FILE)
        {
          /* Open the entry */
          if ((kpSourceEntry->entryData.mode & mode) == mode &&
              kpSourceEntry->entryData.fops->pOpen != NULL)
          {
            /* Open using the underlying driver */
            pFileHandle = kpSourceEntry->entryData.fops->pOpen(
              kpSourceEntry->pExtraData,
              kpPath + nextTokenIdx - 1,
              flags,
              mode);

            if (pFileHandle != (void*)-1)
            {
              /* Copy the data */
              returnVal = _CopyEntryHandle(pEntry, kpSourceEntry);

              if (returnVal != NO_ERROR)
              {
                if (kpSourceEntry->entryData.fops->pClose != NULL)
                {
                  kpSourceEntry->entryData.fops->pClose(
                    kpSourceEntry->pExtraData,
                    pFileHandle);
                }

                KFreeUser(pEntry, NULL);
                pEntry = (void*)-1;
              }
              else
              {
                /* Update the file data */
                pEntry->entryData.mode = mode;
                pEntry->openFlags      = flags;
                pEntry->pFileData = pFileHandle;
              }
            }
            else
            {
              KFreeUser(pEntry, NULL);
              pEntry = pFileHandle;
            }
          }
          else
          {
            KFreeUser(pEntry, NULL);
            pEntry = (void*)-1;
          }
        }
        else if (flags == O_RDONLY)
        {
          /* Copy the entry */
          returnVal = _CopyEntryHandle(pEntry, kpSourceEntry);

          /* Check return value */
          if (returnVal != NO_ERROR)
          {
            KFreeUser(pEntry, NULL);
            pEntry = (void*)-1;
          }
        }
        else
        {
          KFreeUser(pEntry, NULL);
          pEntry = (void*)-1;
        }
      }
      else
      {
        KFreeUser(pEntry, NULL);
        pEntry = (void*)-1;
      }
    }
    else
    {
      if (flags == O_RDONLY)
      {
        /* This is the root directory */
        returnVal = _CopyEntryHandle(pEntry, &sProcFsRootEntry);

        /* Check return value */
        if (returnVal != NO_ERROR)
        {
          KFreeUser(pEntry, NULL);
          pEntry = (void*)-1;
        }
      }
      else
      {
        KFreeUser(pEntry, NULL);
        pEntry = (void*)-1;
      }
    }
  }
  else
  {
    pEntry = (void*)-1;
  }

  return pEntry;
}

static int32_t _ProcFSVFSClose(void* pDrvCtrl, void* pHandle)
{
  S_ProcFSEntry* pEntry;
  int32_t        retVal;

  (void)pDrvCtrl;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pEntry = (S_ProcFSEntry*)pHandle;

    /* Call close if entry */
    if (pEntry->type == PROCFS_ENTRY_FILE)
    {
      if (pEntry->entryData.fops->pClose != NULL)
      {
        retVal = pEntry->entryData.fops->pClose(pEntry->pExtraData,
                                                pEntry->pFileData);
      }
      else
      {
        retVal = 0;
      }
    }
    else
    {
      retVal = 0;
    }
  }
  else
  {
    retVal = -1;
  }

  if (retVal == 0)
  {
    /* Release name memory */
    KFreeUser(pEntry->entryData.name, NULL);
    /* Release the handle */
    KFreeUser(pHandle, NULL);
  }

  return retVal;
}

static ssize_t _ProcFSVFSWrite(void*       pDrvCtrl,
                               void*       pHandle,
                               const void* kpBuffer,
                               size_t      count)
{
  S_ProcFSEntry* pEntry;
  ssize_t        retVal;

  (void)pDrvCtrl;

  if (pHandle != NULL && kpBuffer != NULL && pHandle != (void*)-1)
  {
    pEntry = (S_ProcFSEntry*)pHandle;

    /* Check the write capability */
    if (pEntry->entryData.fops->pWrite != NULL &&
        (pEntry->openFlags & O_RDWR) == O_RDWR)
    {
      retVal = pEntry->entryData.fops->pWrite(pEntry->pExtraData,
                                              pEntry->pFileData,
                                              kpBuffer,
                                              count);
    }
    else
    {
      retVal = -1;
    }
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _ProcFSVFSRead(void*  pDrvCtrl,
                              void*  pHandle,
                              void*  pBuffer,
                              size_t count)
{
  S_ProcFSEntry* pEntry;
  ssize_t        retVal;

  (void)pDrvCtrl;

  if (pHandle != NULL && pBuffer != NULL && pHandle != (void*)-1)
  {
    pEntry = (S_ProcFSEntry*)pHandle;

    /* Check the read capability */
    if (pEntry->entryData.fops->pRead != NULL &&
        (pEntry->openFlags & O_RDONLY) == O_RDONLY)
    {
      retVal = pEntry->entryData.fops->pRead(pEntry->pExtraData,
                                             pEntry->pFileData,
                                             pBuffer,
                                             count);
    }
    else
    {
      retVal = -1;
    }
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static int32_t _ProcFSVFSReadDir(void*             pDriverData,
                                 void*             pHandle,
                                 S_DirectoryEntry* pDirEntry)
{
  size_t            nameLen;
  size_t            i;
  S_ProcFSEntry*    pEntry;
  S_ProcFSDirEntry* pNextDirEntry;
  int32_t           retVal;

  (void)pDriverData;

  if (pHandle != NULL && pDirEntry != NULL && pHandle != (void*)-1)
  {
    pEntry = (S_ProcFSEntry*)pHandle;

    /* Check the read capability */
    if (pEntry->type == PROCFS_ENTRY_DIR)
    {
      pDirEntry->type = VFS_FILE_TYPE_DIR;

      if (pEntry->offset == 0)
      {
        /* Return current */
        pDirEntry->pName[0] = '.';
        pDirEntry->pName[1] = 0;
        pDirEntry->type = VFS_FILE_TYPE_DIR;
        ++pEntry->offset;
        retVal = 1;
      }
      else if (pEntry->offset == 1)
      {
        /* Return parent */
        pDirEntry->pName[0] = '.';
        pDirEntry->pName[1] = '.';
        pDirEntry->pName[2] = 0;

        ++pEntry->offset;
        if (pEntry->entryData.pSubDir == NULL)
        {
          retVal = 0;
        }
        else
        {
          retVal = 1;
        }
      }
      else
      {
        pNextDirEntry = pEntry->entryData.pSubDir;
        for (i = 2; i < pEntry->offset && pNextDirEntry != NULL; ++i)
        {
          pNextDirEntry = pNextDirEntry->pNext;
        }

        if (pNextDirEntry != NULL)
        {
          ++pEntry->offset;

          nameLen = strnlen(pNextDirEntry->name, VFS_FILENAME_MAX_LENGTH);
          memcpy(pDirEntry->pName,
                 pNextDirEntry->name,
                 MIN(sizeof(pDirEntry->pName) - 1, nameLen));
          pDirEntry->pName[MIN(sizeof(pDirEntry->pName) - 1, nameLen)] = 0;

          if (pNextDirEntry->pNext == NULL)
          {
            retVal = 0;
          }
          else
          {
            retVal = 1;
          }
        }
        else
        {
          retVal = -1;
        }
      }
    }
    else if (pEntry->entryData.fops->pReadDir != NULL)
    {
      retVal =  pEntry->entryData.fops->pReadDir(pEntry->pExtraData,
                                                 pEntry->pFileData,
                                                 pDirEntry);
    }
    else
    {
      retVal = -1;
    }
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _ProcFSVFSIOCTL(void*    pDriverData,
                               void*    pHandle,
                               uint32_t operation,
                               void*    pArgs)
{
  S_ProcFSEntry* pEntry;
  ssize_t        retVal;

  (void)pDriverData;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pEntry = (S_ProcFSEntry*)pHandle;

    /* Check the write capability */
    if (pEntry->entryData.fops->pIOCTL != NULL &&
        (pEntry->openFlags & O_RDWR) == O_RDWR)
    {
      retVal = pEntry->entryData.fops->pIOCTL(pEntry->pExtraData,
                                              pEntry->pFileData,
                                              operation,
                                              pArgs);
    }
    else
    {
      retVal = -1;
    }
  }
  else
  {
    retVal = -1;
  }

  /* Not supported */
  return retVal;
}

void ProcFSInit(void)
{
  T_VFSDriver procFsDriver;

  /* Register the driver */
  procFsDriver = RegisterVFSDriver(PROCFS_ROOT_DIR_PATH,
                                   NULL,
                                   _ProcFSVFSOpen,
                                   _ProcFSVFSClose,
                                   _ProcFSVFSRead,
                                   _ProcFSVFSWrite,
                                   _ProcFSVFSReadDir,
                                   _ProcFSVFSIOCTL);

  PROCFS_ASSERT(procFsDriver != VFS_DRIVER_INVALID,
                "Failed to initialize the ProcFS driver.",
                ERR_INVALID_VALUE);
}

E_Return ProcFSCreateDir(const char*        kpName,
                         const char*        kpParentPath,
                         S_ProcFSDirEntry** ppDirectory)
{
  S_ProcFSEntry*     pParent;
  S_ProcFSEntry*     pNewDirectory;
  S_ProcFSDirEntry*  pDirEntry;
  S_ProcFSDirEntry** pEmplaceDirEntry;
  ssize_t            nextTokenIdx;
  size_t             nameLen;
  int32_t            cmpRet;
  E_Return           returnVal;

  if (ppDirectory != NULL && kpName != NULL && kpName[0] != 0)
  {
    *ppDirectory = NULL;

    /* Search for parent */
    if (kpParentPath != NULL)
    {
      pParent = _GetEntry(&sProcFsRootEntry, kpParentPath, &nextTokenIdx);
    }
    else
    {
      pParent = &sProcFsRootEntry;
    }

    if (pParent != NULL)
    {
      returnVal = NO_ERROR;

      if (pParent->type != PROCFS_ENTRY_DIR)
      {
        returnVal = ERR_UNAUTHORIZED_ACTION;
      }

      /* Prepare the path */
      for (nameLen = 0; returnVal == NO_ERROR && kpName[nameLen] != 0;
           ++nameLen)
      {
        /* Check there is no delimiter in the path */
        if (kpName[nameLen] == VFS_PATH_DELIMITER)
        {
          returnVal = ERR_INVALID_PARAMETER;
          break;
        }
      }

      if (returnVal == NO_ERROR)
      {
        /* Search in parent if entry exists */
        pDirEntry        = pParent->entryData.pSubDir;
        pEmplaceDirEntry = &pParent->entryData.pSubDir;
        while (pDirEntry != NULL)
        {
          cmpRet = strncmp(pDirEntry->name, kpName, nameLen);

          /* If exists, return error */
            if (cmpRet == 0 &&
                strnlen(pDirEntry->name, VFS_FILENAME_MAX_LENGTH) == nameLen)
          {
            returnVal = ERR_UNAUTHORIZED_ACTION;
          }
          else if (cmpRet < 0)
          {
            /* Get the last entry that is lexicographicaly less */
            pEmplaceDirEntry = &pDirEntry->pNext;
          }
          else
          {
            break;
          }

          pDirEntry = pDirEntry->pNext;
        }

        if (returnVal == NO_ERROR)
        {
          /* Create the entry */
          pNewDirectory = KMalloc(sizeof(S_ProcFSEntry),
                                  ALIGN_ADDRESS,
                                  KMALLOC_FREE_POOL);
          pNewDirectory->entryData.name = KMalloc(nameLen + 1,
                                                  ALIGN_ADDRESS,
                                                  KMALLOC_FREE_POOL);

          /* Populate entry data */
          memcpy(pNewDirectory->entryData.name, kpName, nameLen);
          pNewDirectory->entryData.name[nameLen] = 0;
          pNewDirectory->type            = PROCFS_ENTRY_DIR;
          pNewDirectory->openFlags       = O_RDONLY;
          pNewDirectory->offset          = 0;
          pNewDirectory->pExtraData      = NULL;
          pNewDirectory->pFileData       = NULL;
          pNewDirectory->entryData.fops  = &sProcFsFops;
          pNewDirectory->entryData.mode  = 0444;
          pNewDirectory->entryData.pData = pNewDirectory;

          /* Apply link */
          pNewDirectory->entryData.pNext = *pEmplaceDirEntry;
          *pEmplaceDirEntry = &pNewDirectory->entryData;

          pNewDirectory->entryData.pParent = &pParent->entryData;
          pNewDirectory->entryData.pSubDir = NULL;

          /* Create copy to send to user */
          *ppDirectory = &pNewDirectory->entryData;
        }
      }
    }
    else
    {
      returnVal = ERR_NOT_FOUND;
    }
  }
  else
  {
    returnVal = ERR_INVALID_PARAMETER;
  }

  return returnVal;
}

E_Return ProcFSRemoveDir(S_ProcFSDirEntry** ppDirectory)
{
  S_ProcFSDirEntry*  pEntry;
  S_ProcFSDirEntry*  pParent;
  S_ProcFSDirEntry*  pPrevious;
  S_ProcFSDirEntry** ppRemovePlace;
  E_Return           returnVal;

  if (ppDirectory != NULL && *ppDirectory != NULL)
  {
    pEntry = *ppDirectory;

    /* Check if directory and empty */
    if (((S_ProcFSEntry*)pEntry->pData)->type == PROCFS_ENTRY_DIR &&
        pEntry->pSubDir == NULL)
    {
      pParent = pEntry->pParent;
      ppRemovePlace = &pParent->pSubDir;
      pPrevious = pParent->pSubDir;

      /* Find the link */
      while (pPrevious != NULL)
      {
        if (pPrevious == pEntry)
        {
            break;
        }
        ppRemovePlace = &pPrevious->pNext;
        pPrevious = pPrevious->pNext;
      }

      if (pPrevious != NULL)
      {
        /* Unlink */
        *ppRemovePlace = pEntry->pNext;

        /* Clean the entry */
        KFree(pEntry->name);
        KFree(pEntry->pData);

        *ppDirectory = NULL;

        returnVal = NO_ERROR;
      }
      else
      {
        returnVal = ERR_NOT_FOUND;
      }
    }
    else
    {
      returnVal = ERR_UNAUTHORIZED_ACTION;
    }
  }
  else
  {
    returnVal = ERR_INVALID_PARAMETER;
  }

  return returnVal;
}

E_Return ProcFSCreateEntry(const char*             kpName,
                           const uint32_t          kMode,
                           S_ProcFSDirEntry*       pParent,
                           S_ProcFSFileOperations* pFops,
                           void*                   pExtraData,
                           S_ProcFSDirEntry**      pEntry)
{
  size_t             nameLen;
  int32_t            cmpRet;
  S_ProcFSDirEntry*  pCursor;
  S_ProcFSDirEntry** ppEmplace;
  S_ProcFSEntry*     pNewEntry;
  E_Return           returnVal;

  /* Check the inputs */
  if (kpName != NULL && pEntry != NULL && pFops != NULL && kpName[0] != 0)
  {
    returnVal = NO_ERROR;

    /* Prepare the path */
    for (nameLen = 0; kpName[nameLen] != 0; ++nameLen)
    {
      /* Check there is no delimiter in the path */
      if (kpName[nameLen] == VFS_PATH_DELIMITER)
      {
        returnVal = ERR_INVALID_PARAMETER;
        break;
      }
    }

    if (returnVal == NO_ERROR)
    {
      /* Get root if needed */
      if (pParent == NULL)
      {
        pParent = &sProcFsRootEntry.entryData;
      }

      if (((S_ProcFSEntry*)pParent->pData)->type != PROCFS_ENTRY_DIR)
      {
        returnVal = ERR_UNAUTHORIZED_ACTION;
      }

      /* Search of the parent already has an entry with the same name */
      ppEmplace = &pParent->pSubDir;
      pCursor = pParent->pSubDir;
      while (returnVal == NO_ERROR && pCursor != NULL)
      {
        cmpRet = strncmp(pCursor->name, kpName, nameLen);

        /* If exists, return error */
        if (cmpRet == 0 &&
          strnlen(pCursor->name, VFS_FILENAME_MAX_LENGTH) == nameLen)
        {
          returnVal = ERR_UNAUTHORIZED_ACTION;
          break;
        }
        else if (cmpRet < 0)
        {
          /* Get the last entry that is lexicographicaly less */
          ppEmplace = &pCursor->pNext;
        }
        else
        {
          break;
        }

        pCursor = pCursor->pNext;
      }

      if (returnVal == NO_ERROR)
      {
        /* Create the entry */
        pNewEntry = KMalloc(sizeof(S_ProcFSEntry),
                            ALIGN_ADDRESS,
                            KMALLOC_FREE_POOL);
        pNewEntry->entryData.name = KMalloc(nameLen + 1,
                                    ALIGN_ADDRESS,
                                    KMALLOC_FREE_POOL);

        /* Populate entry data */
        memcpy(pNewEntry->entryData.name, kpName, nameLen);
        pNewEntry->entryData.name[nameLen] = 0;
        pNewEntry->type = PROCFS_ENTRY_FILE;
        pNewEntry->offset          = 0;
        pNewEntry->pExtraData      = pExtraData;
        pNewEntry->pFileData       = NULL;
        pNewEntry->openFlags       = O_RDWR;
        pNewEntry->entryData.fops  = pFops;
        pNewEntry->entryData.mode  = kMode;
        pNewEntry->entryData.pData = pNewEntry;

        /* Apply link */
        pNewEntry->entryData.pNext = *ppEmplace;
        *ppEmplace = &pNewEntry->entryData;

        pNewEntry->entryData.pParent = pParent;
        pNewEntry->entryData.pSubDir = NULL;

        /* Create copy to send to user */
        *pEntry = &pNewEntry->entryData;
      }
    }
  }
  else
  {
    returnVal = ERR_INVALID_PARAMETER;
  }

  return returnVal;
}

E_Return ProcFSRemoveEntry(const char*       kpName,
                           S_ProcFSDirEntry* pParent)
{
  size_t             nameLen;
  int32_t            cmpRet;
  S_ProcFSDirEntry*  pCursor;
  S_ProcFSDirEntry** ppRemove;
  E_Return           returnVal;

  /* Check the inputs */
  if (kpName != NULL && kpName[0] != 0)
  {
    if (pParent == NULL)
    {
      pParent = &sProcFsRootEntry.entryData;
    }

    returnVal = NO_ERROR;

    /* Prepare the path */
    for (nameLen = 0; kpName[nameLen] != 0; ++nameLen)
    {
      /* Check there is no delimiter in the path */
      if (kpName[nameLen] == VFS_PATH_DELIMITER)
      {
        returnVal = ERR_INVALID_PARAMETER;
        break;
      }
    }

    if (returnVal == NO_ERROR)
    {
      pCursor = pParent->pSubDir;
      ppRemove = &pParent->pSubDir;
      while (pCursor != NULL)
      {
        cmpRet = strncmp(pCursor->name, kpName, nameLen);

        /* If exists, return error */
        if (cmpRet == 0 &&
          strnlen(pCursor->name, VFS_FILENAME_MAX_LENGTH) == nameLen)
        {
            break;
        }

        ppRemove = &pCursor->pNext;
        pCursor = pCursor->pNext;
      }

      /* Return if not found */
      if (pCursor != NULL)
      {
        /* Check if directory */
        if (((S_ProcFSEntry*)pCursor->pData)->type == PROCFS_ENTRY_FILE)
        {
          /* Remove link */
          *ppRemove = pCursor->pNext;

          /* Free memory */
          KFree(pCursor->name);
          KFree(pCursor->pData);
        }
        else
        {
          returnVal = ERR_UNAUTHORIZED_ACTION;
        }
      }
      else
      {
        returnVal = ERR_NOT_FOUND;
      }
    }
  }
  else
  {
    returnVal = ERR_INVALID_PARAMETER;
  }

  return returnVal;
}
/************************************ EOF *************************************/