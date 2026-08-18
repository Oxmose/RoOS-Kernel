/*******************************************************************************
 * @file VirtualFS.c
 *
 * @see VirtualFS.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/07/2026
 *
 * @version 2.0
 *
 * @brief Virtual Filesystem driver.
 *
 * @details Virtual Filesystem driver. This virtual filesystem manages all mount
 * points in roOs, allows pluging various filesystems with the driver API and
 * provides the necessary API to manage file and file-based drivers.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Includes */
#include <Panic.h>
#include <string.h>
#include <Vector.h>
#include <stddef.h>
#include <stdint.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <VirtualFS.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name  */
#define MODULE_NAME "VFS"

/** @brief Number of file descriptor allocated when starting a process. */
#define VFS_INITIAL_FD_COUNT 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Node structure used to keep track of the mounted points */
typedef struct S_VFSNode
{
  /** @brief The mount point path relative to the parent path */
  char pMountPoint[VFS_FILENAME_MAX_LENGTH];

  /** @brief Stores the mount point path length */
  size_t mountPointLength;

  /**
   * @brief The driver to use when accessing files at this mount point, if
   * NULL, this is a transient node to other mount points
   */
  S_FSDriver* pDriver;
  /** @brief The first child node of this node */
  struct S_VFSNode* pFirstChild;
  /** @brief The next sibling node of this node */
  struct S_VFSNode* pNextSibling;
  /** @brief The parent of this node */
  struct S_VFSNode* pParent;
} S_VFSNode;

/** @brief Describes the path parsing states */
typedef enum
{
  /** @brief Last character was a delimiter */
  PARSE_STATE_DELIMITER,
  /** @brief Last character was a self character */
  PARSE_STATE_SELF,
  /** @brief Last character was a back character */
  PARSE_STATE_BACK,
  /** @brief Regular parse state */
  PARSE_STATE_CHARACTER
} E_ParsingState;

/** @brief Defines a VFS file descriptor table */
typedef struct
{
  /** @brief File descriptor table */
  S_Vector* pFDTable;

  /** @brief File descriptor table lock */
  S_KernelSpinlock lock;
} S_FDTable;

/** @brief Stores the shared information for a file descriptor */
typedef struct
{
  /** @brief FD file path */
  char* pFilePath;

  /** @brief FD internal driver file handle */
  void* pFileHandle;

  /** @brief FD file driver */
  S_FSDriver* pDriver;

  /** @brief The share lock */
  S_KernelSpinlock lock;
} S_FileDescriptorShared;

/** @brief Defines the VFS internal file descriptor */
typedef struct
{
  /** @brief The index of the FD in the FD table */
  uint32_t tableId;
  /** @brief The file descriptor shared data */
  S_FileDescriptorShared* pShared;
  /** @brief Mode used when opening the file */
  int openMode;
  /** @brief Flags used when opening the file */
  int openFlags;
} S_FileDescriptor;

/** @brief Defines the generic descriptor for the generic VFS operations */
typedef struct
{
  /** @brief Descriptor mount point */
  S_VFSNode* pMountPt;
  /** @brief Contains the next dir entry */
  S_VFSNode* pNextChildCursor;
} S_VFSGenericFileDescriptor;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the driver manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the driver manager to ensure correctness of
 * execution. Due to the critical nature of the driver manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 * @param[in] IS_PROCESS Tells if the panic comes from a process.
 */
#define VFS_ASSERT(COND, MSG, ERROR, IS_PROCESS) {     \
  if ((COND) == false)                                 \
  {                                                    \
    PANIC(ERROR, MODULE_NAME, MSG, false, IS_PROCESS); \
  }                                                    \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Cleans the path provided as parameter.
 *
 * @details Cleans the path provided as parameter. The function will strip
 * unwanted or redundant character and perform other cleaning tasks.
 *
 * @param[out] pCleanPath A pointer to the memory where to store the cleaned
 * path.
 * @param[in] kpOriginalPath The original path to clean.
 *
 * @return The function returns the size of the clean path. -1 on error.
 */
static ssize_t _CleanPath(char* pCleanPath, const char* kpOriginalPath);

/**
 * @brief Finds a node in the node tree.
 *
 * @details Finds a node in the node tree based on the provided path.
 * If the node is not found, NULL is returned.
 *
 * @param[in] pRoot The root of the tree to start from.
 * @param[in] kpPath The path to access the node.
 * @param[in] kpPathLength The path length.
 * @param[out] pNextToken Next tocken in the path after this node.
 *
 * @return The function return the node associated to the path or NULL if not
 * found.
 */
static S_VFSNode* _FindNode(S_VFSNode*   pRoot,
                            const char*  kpPath,
                            const size_t kPathLength,
                            size_t*      pNextToken);

/**
 * @brief Adds a driver with a given path to the mount point graph.
 *
 * @details Adds a driver with a given path to the mount point graph. The
 * node can be added to already existing mount point, it will be created as a
 * child of the parent node.
 *
 * @param[in] pRoot The root of the mount graph to use.
 * @param[in] kpPath The path of the driver to add.
 * @param[in] pathLen The length of the path
 * @param[in, out] pDriver The driver to add.
 */
static void _AddDriverNode(S_VFSNode*  pRoot,
                           const char* kpPath,
                           size_t      pathLen,
                           S_FSDriver* pDriver);

/**
 * @brief Cleans a node from the mount point graph.
 *
 * @details Cleans a node from the mount point graph. The node will be removed
 * if none of its children implement a driver. This will also clean the
 * children nodes.
 *
 * @param[out] pNode The node to clean.
 *
 * @return The function returns false if the node implements a driver, true
 * otherwise and if cleaned.
 */
static bool _RemoveDriverNode(S_VFSNode* pRoot);

/**
 * @brief VFS memory allocation function.
 *
 * @details VFS memory allocation function. Used to allocate internal structures
 * like the FD tables, etc.
 *
 * @param[in] kSize The size in bytes to allocate.
 *
 * @return A pointer to the allocated region is returned, otherwise, NULL.
 */
static void* _VFSAllocUser(const size_t kSize);

/**
 * @brief VFS memory free function.
 *
 * @details VFS memory free function. Used to release internal structures
 * like the FD tables, etc.
 *
 * @param[in] ptr The pointer to the memory to free.
 */
static void _VFSFreeUser(void* ptr);

/**
 * @brief Creates a new file descriptor.
 *
 * @details Creates a new file descriptor. The descriptor is populated with the
 * data given as parameter.
 *
 * @param[in, out] pTable The process file descriptor table.
 * @param[in] pDriver The driver to associate to the file descriptor.
 * @param[in] pFileHandle The file handle generated by the underlying driver.
 * @param[in] kpPath The absolute path of the file corresponding to the fd.
 * @param[in] kFlags The flags used when opening the fd.
 * @param[in] kMode The mode used when opening the fd.
 *
 * @return The function returns the value of the newly created file descriptor.
 * On error the function returns -1.
 */
static int32_t _CreateFileDescriptor(S_FDTable*  pTable,
                                     S_FSDriver* pDriver,
                                     void*       pFileHandle,
                                     const char* kpPath,
                                     const int   kFlags,
                                     const int   kMode);

/**
 * @brief Destroys a file descriptor to the free fd table.
 *
 * @details Destroys a file descriptor to the free fd table. The resources
 * associated to the file descritptor are freed.
 *
 * @param[in, out] pTable The process file descriptor table.
 * @param[in] kFD The generic file descriptor to use.
 */
static void _DestroyFileDescriptor(S_FDTable* pTable, const int32_t kFD);

/**
 * @brief Gets the file internal file descriptor associated to a generic file
 * descriptor.
 *
 * @details Gets the file internal file descriptor associated to a generic file
 * descriptor.
 *
 * @param[in, out] pTable The process file descriptor table.
 * @param[in] kFd The generic file descriptor to use.
 * @param[out] ppInternalFd A buffer to the internal file descriptor pointer to
 * retrieve.
 *
 * @return The function returns the success or error state.
 */
static E_Return _GetFileDescriptor(S_FDTable*         pTable,
                                   const int32_t      kFd,
                                   S_FileDescriptor** ppInternalFd);

/**
 * @brief Generic VFS open hook.
 *
 * @details Generic VFS open hook. This function returns a handle to control the
 * VFS generic nodes.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] kpPath The path in the VFS mount point table.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _GenericOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode);

/**
 * @brief Generic VFS close hook.
 *
 * @details Generic VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _GenericClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief Generic VFS write hook.
 *
 * @details Generic VFS write hook.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _GenericWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count);

/**
 * @brief Generic VFS read hook.
 *
 * @details Generic VFS read hook.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _GenericRead(void*  pDrvCtrl,
                            void*  pHandle,
                            void*  pBuffer,
                            size_t count);

/**
 * @brief Generic VFS ReadDir hook.
 *
 * @details Generic VFS ReadDir hook. This function performs the ReadDir for the
 * Generic driver.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _GenericReadDir(void*             pDriverData,
                               void*             pHandle,
                               S_DirectoryEntry* pDirEntry);

/**
 * @brief Generic VFS IOCTL hook.
 *
 * @details Generic VFS IOCTL hook. This function performs the IOCTL for the
 * Generic driver.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _GenericIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the registered kernel fs table */
extern uintptr_t _START_FS_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief VFS mount point graph */
static S_VFSNode* spRootPoint = NULL;
/** @brief Kernel file descriptor table lock */
static S_KernelSpinlock sMountPointLock;
/** @brief Generic VFS driver to handle transient nodes */
static S_FSDriver sVFSGenericDriver =
{
  .pDriverData = NULL,
  .pOpen       = _GenericOpen,
  .pClose      = _GenericClose,
  .pRead       = _GenericRead,
  .pWrite      = _GenericWrite,
  .pReadDir    = _GenericReadDir,
  .pIOCTL      = _GenericIOCTL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static ssize_t _CleanPath(char* pCleanPath, const char* kpOriginalPath)
{
  size_t         size;
  ssize_t        newSize;
  size_t         i;
  size_t         lastDelimiter;
  E_ParsingState parseState;

  size    = strnlen(kpOriginalPath, VFS_PATH_MAX_LENGTH * 10);
  newSize = 0;
  i = 0;

  /* Remove trailing delimiter */
  while (size > 0 && kpOriginalPath[size - 1] == VFS_PATH_DELIMITER)
  {
    --size;
  }

  /* Add leading slash */
  if (size > 0 && kpOriginalPath[0] == VFS_PATH_DELIMITER)
  {
    ++i;
  }
  pCleanPath[newSize++] = VFS_PATH_DELIMITER;
  lastDelimiter = 0;

  /* Copy while removing multiple delimiters */
  parseState = PARSE_STATE_DELIMITER;
  for (; i < size; ++i)
  {
    /* Handle tool long file name or too long path */
    if (i - lastDelimiter >= VFS_FILENAME_MAX_LENGTH ||
        newSize >= VFS_PATH_MAX_LENGTH)
    {
      newSize = -1;
      break;
    }

    /* Manage multiple delimiter */
    if (kpOriginalPath[i] == VFS_PATH_DELIMITER)
    {
      if (parseState == PARSE_STATE_DELIMITER)
      {
        continue;
      }
      else if (parseState == PARSE_STATE_SELF)
      {
        parseState = PARSE_STATE_DELIMITER;
        --newSize;
        continue;
      }
      else if (parseState == PARSE_STATE_BACK)
      {
        /* Remove the last directory */
        parseState = PARSE_STATE_DELIMITER;
        newSize -= MIN(newSize - 1, 4);
        while (newSize > 0 && pCleanPath[newSize - 1] != '/')
        {
          --newSize;
        }
        continue;
      }
      else
      {
        parseState = PARSE_STATE_DELIMITER;
        lastDelimiter = newSize;
      }
    }
    else if (kpOriginalPath[i] == '.')
    {
      if (parseState == PARSE_STATE_BACK)
      {
        /* We are not a special sequence anymore, copy the data and continue */
        parseState = PARSE_STATE_CHARACTER;
      }
      else if (parseState == PARSE_STATE_SELF)
      {
        /* Second . character is back */
        parseState = PARSE_STATE_BACK;
      }
      else if (parseState == PARSE_STATE_DELIMITER)
      {
        /* First . character is self */
        parseState = PARSE_STATE_SELF;
      }
    }
    else
    {
      parseState = PARSE_STATE_CHARACTER;
    }

    pCleanPath[newSize++] = kpOriginalPath[i];
  }

  /* Manage end state */
  if (parseState == PARSE_STATE_SELF)
  {
    --newSize;
  }
  else if (parseState == PARSE_STATE_BACK)
  {
    /* Remove the last directory */
    newSize -= MIN(newSize - 1, 4);
    while (newSize > 0 && pCleanPath[newSize - 1] != '/')
    {
      --newSize;
    }
  }

  if (newSize >= 0)
  {
    if (newSize > 1 && pCleanPath[newSize - 1] == '/')
    {
      --newSize;
    }
    pCleanPath[newSize] = 0;
  }
  else
  {
    pCleanPath[0] = 0;
  }

  return newSize;
}

static S_VFSNode* _FindNode(S_VFSNode*   pRoot,
                            const char*  kpPath,
                            const size_t kPathLength,
                            size_t*      pNextToken)
{
  ssize_t    nextToken;
  size_t     tokenSize;
  S_VFSNode* pNode;
  S_VFSNode* pLevelNode;

  *pNextToken = 0;

  if (kPathLength != 0)
  {
    /* Get the next delimiter */
    nextToken = VFSGetNextPathTokenPosition(kpPath, kPathLength);
    if (nextToken <= 0 && (kpPath[0] != VFS_PATH_DELIMITER || kPathLength != 1))
    {
      nextToken = kPathLength;
      tokenSize = kPathLength;
    }
    else if (kpPath[0] == VFS_PATH_DELIMITER && kPathLength == 1)
    {
      nextToken = 1;
      tokenSize = 0;
    }
    else
    {
      tokenSize = nextToken - 1;
    }

    /* Find the next node */
    pLevelNode = pRoot;
    while (pLevelNode != NULL)
    {
      if (tokenSize == pLevelNode->mountPointLength &&
          strncmp(kpPath, pLevelNode->pMountPoint, tokenSize) == 0)
      {
        break;
      }

      /* Go to sibling */
      pLevelNode = pLevelNode->pNextSibling;
    }

    if (pLevelNode != NULL && (size_t)nextToken != kPathLength)
    {
      /* Follow the path to the node */
      pNode = _FindNode(pLevelNode->pFirstChild,
                        kpPath + nextToken,
                        kPathLength - nextToken,
                        pNextToken);
      if (pNode == NULL)
      {
        /* Return the closest match */
        pNode = pLevelNode;
      }
      *pNextToken += nextToken;
    }
    else if (pLevelNode != NULL)
    {
      /* Return the full match */
      pNode = pLevelNode;
      *pNextToken += nextToken;
    }
    else
    {
      pNode = NULL;
    }
  }
  else
  {
    pNode = NULL;
  }

  return pNode;
}

static void _AddDriverNode(S_VFSNode*  pRoot,
                           const char* kpPath,
                           size_t      pathLen,
                           S_FSDriver* pDriver)
{
  ssize_t    nextToken;
  size_t     token;
  S_VFSNode* pNode;
  S_VFSNode* pSibling;
  S_VFSNode* pSaveSibling;

  /* Add intermediate nodes until we reached the end of the path */

  while (pathLen > 0)
  {
    /* Get the next token */
    nextToken = VFSGetNextPathTokenPosition(kpPath, pathLen);
    if (nextToken <= 0)
    {
      token = pathLen + 1;
    }
    else
    {
      token = nextToken;
    }

    /* Create the node */
    pNode = KMalloc(sizeof(S_VFSNode), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
    memcpy(pNode->pMountPoint, kpPath, token - 1);
    pNode->pMountPoint[token - 1] = 0;
    pNode->mountPointLength = token - 1;

    /* Add the driver */
    if (nextToken != -1)
    {
      pNode->pDriver = NULL;
    }
    else
    {
      pNode->pDriver = pDriver;
      pDriver->pNode = pNode;
    }

    /* Link the node in lexicographic order */
    pNode->pFirstChild = NULL;
    pNode->pParent = pRoot;

    if (pRoot->pFirstChild != NULL)
    {
      pSibling = pRoot->pFirstChild;
      pSaveSibling = pSibling;
      while (pSibling != NULL &&
             strcmp(pSibling->pMountPoint, pNode->pMountPoint) < 0)
      {
        pSaveSibling = pSibling;
        pSibling = pSibling->pNextSibling;
      }

      if (pSaveSibling == pRoot->pFirstChild)
      {
        /* Add as first child */
        pRoot->pFirstChild = pNode;
        pNode->pNextSibling = pSaveSibling;
      }
      else
      {
        pNode->pNextSibling = pSaveSibling->pNextSibling;
        pSaveSibling->pNextSibling = pNode;
      }
    }
    else
    {
      pRoot->pFirstChild  = pNode;
      pNode->pNextSibling = NULL;
    }

    /* Update the path */
    pRoot = pNode;

    kpPath  += token;
    pathLen -= MIN(token, pathLen);
  }
}

static bool _RemoveDriverNode(S_VFSNode* pRoot)
{
  bool       cleaned;
  S_VFSNode *pChild;
  S_VFSNode *pSibling;

  /* Check if the node hosts a driver */
  if (pRoot->pDriver == NULL)
  {
    cleaned = true;

    /* Check if all chlidren also host a driver */
    pChild = pRoot->pFirstChild;
    while (pChild != NULL)
    {

      pSibling = pChild->pNextSibling;
      cleaned = _RemoveDriverNode(pChild);
      if (cleaned == false)
      {
        break;
      }

      pChild = pSibling;
    }

    /* If all sub-nodes have been cleaned, clean the current node */
    if (cleaned == true)
    {
      /* Unlink */
      if (pRoot->pParent->pFirstChild != pRoot)
      {
        pChild = pRoot->pParent->pFirstChild;
        while (pChild->pNextSibling != pRoot)
        {
          pChild = pChild->pNextSibling;
        }
        pChild->pNextSibling = pRoot->pNextSibling;
      }
      else
      {
        pRoot->pParent->pFirstChild = pRoot->pNextSibling;
      }

      KFree(pRoot, KMALLOC_FREE_POOL);
    }
  }
  else
  {
    cleaned = false;
  }

  return cleaned;
}

static void* _VFSAllocUser(const size_t kSize)
{
  return KMalloc(kSize, ALIGN_ADDRESS, KMALLOC_PROCESS_HEAP);
}

static void _VFSFreeUser(void* ptr)
{
  return KFree(ptr, KMALLOC_PROCESS_HEAP);
}

static int32_t _CreateFileDescriptor(S_FDTable*  pTable,
                                     S_FSDriver* pDriver,
                                     void*       pFileHandle,
                                     const char* kpPath,
                                     const int   kFlags,
                                     const int   kMode)
{
  size_t             pathLen;
  S_FileDescriptor*  pFD;
  E_Return           retCode;
  int32_t            fdId;
  size_t             i;

  KERNEL_LOCK(pTable->lock);

  fdId  = -1;

  pFD = KMalloc(sizeof(S_FileDescriptor), ALIGN_ADDRESS, KMALLOC_PROCESS_HEAP);
  if (pFD != NULL)
  {
    pFD->pShared = KMalloc(sizeof(S_FileDescriptorShared),
                           ALIGN_ADDRESS,
                           KMALLOC_PROCESS_HEAP);
    if (pFD->pShared != NULL)
    {
      pathLen = strnlen(kpPath, VFS_PATH_MAX_LENGTH);
      pFD->pShared->pFilePath = KMalloc(pathLen + 1,
                                        ALIGN_ADDRESS,
                                        KMALLOC_PROCESS_HEAP);
      if (pFD->pShared->pFilePath != NULL)
      {
        // Find a free slot
        for (i = 0; i < pTable->pFDTable->size; ++i)
        {
          if (pTable->pFDTable->ppArray[i] == NULL)
          {
            break;
          }
        }
        if (i == pTable->pFDTable->size)
        {
          pFD->tableId = pTable->pFDTable->size;
          retCode = VectorPush(pTable->pFDTable, pFD);
        }
        else
        {
          pFD->tableId = i;
          retCode = VectorSet(pTable->pFDTable, i, pFD);
        }

        if (retCode != NO_ERROR)
        {
          KFree(pFD->pShared->pFilePath, KMALLOC_PROCESS_HEAP);
          KFree(pFD->pShared, KMALLOC_PROCESS_HEAP);
          KFree(pFD, KMALLOC_PROCESS_HEAP);
        }
        else
        {
          fdId = pFD->tableId;
        }
      }
      else
      {
        KFree(pFD->pShared, KMALLOC_PROCESS_HEAP);
        KFree(pFD, KMALLOC_PROCESS_HEAP);
      }
    }
    else
    {
      KFree(pFD, KMALLOC_PROCESS_HEAP);
    }
  }

  KERNEL_UNLOCK(pTable->lock);

  if (fdId != -1)
  {
    /* Initialize the shared interface */
    KERNEL_SPINLOCK_INIT(pFD->pShared->lock);
    pFD->openFlags            = kFlags;
    pFD->openMode             = kMode;
    pFD->pShared->pDriver     = pDriver;
    pFD->pShared->pFileHandle = pFileHandle;

    /* Initialize the path */
    memcpy(pFD->pShared->pFilePath, kpPath, pathLen);
    pFD->pShared->pFilePath[pathLen] = 0;
  }

  return fdId;
}

static void _DestroyFileDescriptor(S_FDTable* pTable, const int32_t kFD)
{
  S_FileDescriptor* pFD;
  E_Return          error;

  /* Get the file descriptor */
  error = VectorGet(pTable->pFDTable, kFD, (void**)&pFD);
  VFS_ASSERT(error == NO_ERROR && pFD != NULL,
             "Invalid FD destroy.",
             ERR_INVALID_VALUE,
             true);

  /* Remove the FD and add to free pool */
  error = VectorSet(pTable->pFDTable, kFD, NULL);
  VFS_ASSERT(error == NO_ERROR, "Invalid FD destroy.", ERR_INVALID_VALUE, true);

  /* release the shared data */
  KFree(pFD->pShared->pFilePath, KMALLOC_PROCESS_HEAP);
  KFree(pFD->pShared, KMALLOC_PROCESS_HEAP);
  KFree(pFD, KMALLOC_PROCESS_HEAP);
}

static E_Return _GetFileDescriptor(S_FDTable*         pTable,
                                   const int32_t      kFd,
                                   S_FileDescriptor** ppInternalFd)
{
  E_Return error;

  /* Check that the FD is valid */
  if((size_t)kFd < pTable->pFDTable->size)
  {
    /* Check that the FD is open */
    error = VectorGet(pTable->pFDTable, kFd, (void**)ppInternalFd);
    VFS_ASSERT(error == NO_ERROR, "Invalid FD Get.", error, true);

    if(*ppInternalFd != NULL)
    {
      error = NO_ERROR;
    }
    else
    {
      error = ERR_INVALID_VALUE;
    }
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

static void* _GenericOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode)
{
  S_VFSGenericFileDescriptor* pDesc;

  (void)pDrvCtrl;
  (void)flags;
  (void)mode;

  /* Check if this is an exact node in the mount points */
  if (kpPath[0] == 0)
  {
    pDesc = KMalloc(sizeof(S_VFSGenericFileDescriptor),
                    ALIGN_ADDRESS,
                    KMALLOC_PROCESS_HEAP);
    memset(pDesc, 0, sizeof(S_VFSGenericFileDescriptor));
    pDesc->pMountPt = pDrvCtrl;
  }
  else
  {
    pDesc = (void*)-1;
  }

  return pDesc;
}

static int32_t _GenericClose(void* pDrvCtrl, void* pHandle)
{
  int32_t retVal;

  (void)pDrvCtrl;

  /* Check if it was correctly opened */
  if(pHandle != NULL && pHandle != (void*)-1)
  {
    KFree(pHandle, KMALLOC_PROCESS_HEAP);
    retVal = 0;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _GenericWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count)
{
  (void)pDrvCtrl;
  (void)pHandle;
  (void)kpBuffer;
  (void)count;

  /* Generic write has no effect */
  return -1;
}

static ssize_t _GenericRead(void*  pDrvCtrl,
                               void*  pHandle,
                               void*  pBuffer,
                               size_t count)
{
  (void)pDrvCtrl;
  (void)pHandle;
  (void)pBuffer;
  (void)count;

  /* Generic read has no effect */
  return -1;
}

static int32_t _GenericReadDir(void*             pDriverData,
                               void*             pHandle,
                               S_DirectoryEntry* pDirEntry)
{
  int32_t                     retVal;
  S_VFSGenericFileDescriptor* pDesc;

  (void)pDriverData;

  /* Check if it was correctly opened */
  if(pHandle != NULL && pHandle != (void*)-1)
  {
    pDesc = pHandle;
    if (pDesc->pMountPt != NULL)
    {
      retVal = 1;

      if(pDesc->pNextChildCursor == NULL)
      {
        pDesc->pNextChildCursor = pDesc->pMountPt->pFirstChild;
        if(pDesc->pNextChildCursor == NULL)
        {
          retVal = 0;
        }
      }
      else if(pDesc->pNextChildCursor == (void*)-1)
      {
        retVal = -1;
      }

      if (retVal == 1)
      {
        /* Copy name */
        strncpy(pDirEntry->pName,
                pDesc->pNextChildCursor->pMountPoint,
                VFS_FILENAME_MAX_LENGTH);
        pDirEntry->pName[VFS_FILENAME_MAX_LENGTH] = 0;

        pDirEntry->filenameLength = strnlen(pDirEntry->pName,
                                            VFS_FILENAME_MAX_LENGTH);

        /* Check if the node has children (it is a folder) */
        pDirEntry->type = VFS_FILE_TYPE_DIR;

        /* Check if there is another sibling */
        if(pDesc->pNextChildCursor->pNextSibling != NULL)
        {
          pDesc->pNextChildCursor = pDesc->pNextChildCursor->pNextSibling;
          retVal = 1;
        }
        else
        {
          pDesc->pNextChildCursor = (void*)-1;
          retVal = 0;
        }
      }
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

static ssize_t _GenericIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs)
{
  (void)pDriverData;
  (void)pHandle;
  (void)operation;
  (void)pArgs;

  /* Generic IOCTL has no effect */
  return -1;
}

void VirtualFileSystemInit(void)
{
  /* Initialize the mount point */
  spRootPoint = KMalloc(sizeof(S_VFSNode), ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);
  spRootPoint->mountPointLength = 0;
  spRootPoint->pMountPoint[0]   = 0;
  spRootPoint->pDriver          = NULL;
  spRootPoint->pFirstChild      = NULL;
  spRootPoint->pNextSibling     = NULL;
  spRootPoint->pParent          = NULL;

  KERNEL_SPINLOCK_INIT(sMountPointLock);

  TEST_POINT_FUNCTION_ARGS(VFSCleanPathTest, _CleanPath, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSFindNodeTest, _FindNode, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSAddNodeTest, _AddDriverNode, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSRemoveNodeTest, _RemoveDriverNode, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSRegDriverTest, spRootPoint, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSRemoveDriverTest, spRootPoint, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSCreateFDTest, _CreateFileDescriptor, TEST_VFS_ENABLED);
  TEST_POINT_FUNCTION_ARGS(VFSDestroyFDTest, _DestroyFileDescriptor, TEST_VFS_ENABLED);
}

E_Return CreateProcessFDTable(S_KernelProcess* pProcess)
{
  S_FDTable*         pTable;
  E_Return           retCode;

  /* Allocate the new table */
  pTable = KMalloc(sizeof(S_FDTable), ALIGN_ADDRESS, KMALLOC_PROCESS_HEAP);
  if (pTable != NULL)
  {
    pTable->pFDTable = VectorCreate(VECTOR_ALLOCATOR(_VFSAllocUser,
                                                     _VFSFreeUser),
                                    NULL,
                                    VFS_INITIAL_FD_COUNT,
                                    &retCode);
    if (retCode == NO_ERROR)
    {
      /* Set the table to the process */
      pProcess->pFileDescriptorTable = pTable;
      KERNEL_SPINLOCK_INIT(pTable->lock);
    }
    else
    {
      KFree(pTable, KMALLOC_PROCESS_HEAP);
    }
  }
  else
  {
    retCode = ERR_NO_MEMORY;
  }

  return retCode;
}

void DestroyProcessFDTable(S_KernelProcess* pProcess)
{
  E_Return                retCode;
  size_t                  i;
  S_FDTable*              pTable;
  S_FileDescriptor*       pFD;
  S_FileDescriptorShared* pShared;
  S_FSDriver*             pDriver;

  pTable = pProcess->pFileDescriptorTable;
  /* Release the file descriptors */
  for (i = 0; i < pTable->pFDTable->size; ++i)
  {
    retCode = VectorGet(pTable->pFDTable, i, (void**)&pFD);
    VFS_ASSERT(retCode == NO_ERROR,
               "Failed to get file descriptor",
               retCode,
               false);

    if (pFD != NULL)
    {
      /* Check if we should close the file */
      pShared = pFD->pShared;
      pDriver = pShared->pDriver;
      if (pDriver->pClose != NULL)
      {
        pDriver->pClose(pDriver->pDriverData, pShared->pFileHandle);
      }

      /* Release the shared data */
      KFree(pShared->pFilePath, KMALLOC_PROCESS_HEAP);
      KFree(pShared, KMALLOC_PROCESS_HEAP);
      KFree(pFD, KMALLOC_PROCESS_HEAP);
    }
  }

  /* Release the FD table */
  retCode = VectorDestroy(pTable->pFDTable);
  VFS_ASSERT(retCode == NO_ERROR,
             "Failed to get file descriptor",
             retCode,
             false);

  /* Remove the table */
  KFree(pTable, KMALLOC_PROCESS_HEAP);
}

T_VFSDriver RegisterVFSDriver(const char*  kpPath,
                              void*        pDriverData,
                              T_VFSOpen    pOpen,
                              T_VFSClose   pClose,
                              T_VFSRead    pRead,
                              T_VFSWrite   pWrite,
                              T_VFSReadDir pReadDir,
                              T_VFSIOCTL   pIOCTL)
{
  S_FSDriver* pDriver;
  S_VFSNode*  pNode;
  size_t      pathLen;
  size_t      nextToken;
  char*       pPath;

  /* Allocate the path and clean it */
  pPath = KMalloc(VFS_PATH_MAX_LENGTH, ALIGN_1_BYTE, KMALLOC_FREE_POOL);
  pathLen = _CleanPath(pPath, kpPath);

  if (pathLen > 0)
  {
    KERNEL_LOCK(sMountPointLock);

    /* Search for an existing driver */
    pNode = _FindNode(spRootPoint, pPath, pathLen, &nextToken);
    /* Check that the driver does not exist */
    if (pNode != NULL)
    {
      /* If the node is at the end of the path, ensure it has no driver */
      if (nextToken == pathLen)
      {
        if (pNode->pDriver == NULL)
        {
          pDriver = KMalloc(sizeof(S_FSDriver),
                            ALIGN_ADDRESS,
                            KMALLOC_FREE_POOL);
          pNode->pDriver = pDriver;
          pDriver->pNode = pNode;
        }
        else
        {
          pDriver = VFS_DRIVER_INVALID;
        }
      }
      else
      {
        /* No driver exist, allocate a new one */
        pDriver = KMalloc(sizeof(S_FSDriver), ALIGN_ADDRESS, KMALLOC_FREE_POOL);
        _AddDriverNode(pNode, pPath + nextToken, pathLen - nextToken, pDriver);
      }

      /* Fill the driver structure */
      if (pDriver != VFS_DRIVER_INVALID)
      {
        pDriver->pDriverData = pDriverData;
        pDriver->pOpen       = pOpen;
        pDriver->pClose      = pClose;
        pDriver->pRead       = pRead;
        pDriver->pWrite      = pWrite;
        pDriver->pReadDir    = pReadDir;
        pDriver->pIOCTL      = pIOCTL;
        pDriver->pMount      = NULL;
        pDriver->pUnmount    = NULL;
      }
    }
    else
    {
      pDriver = VFS_DRIVER_INVALID;
    }

    KERNEL_UNLOCK(sMountPointLock);
  }
  else
  {
    pDriver = VFS_DRIVER_INVALID;
  }

  /* Release clean path */
  KFree(pPath, KMALLOC_FREE_POOL);

  return pDriver;
}

E_Return UnregisterDriver(T_VFSDriver driver)
{
  S_VFSNode*  pNode;
  S_FSDriver* pDriverInstance;

  pDriverInstance = (S_FSDriver*)driver;

  KERNEL_LOCK(sMountPointLock);

  /* Unlink the driver */
  pNode = pDriverInstance->pNode;
  pNode->pDriver = NULL;

  /* Release the driver */
  KFree(pDriverInstance, KMALLOC_FREE_POOL);

  /* Clear the mount point */
  _RemoveDriverNode(pNode);

  KERNEL_UNLOCK(sMountPointLock);

  return NO_ERROR;
}

int32_t VFSOpen(const char* kpPath, int32_t flags, int32_t mode)
{
  S_FSDriver* pDriver;
  S_VFSNode*  pNode;
  size_t      pathLen;
  size_t      nextToken;
  char*       pPath;
  int32_t     newFD;
  S_FDTable*  pTable;
  void*       pHandle;
  void*       pDriverData;

  /* Allocate the path and clean it */
  pPath = KMalloc(VFS_PATH_MAX_LENGTH, ALIGN_1_BYTE, KMALLOC_PROCESS_HEAP);
  if (pPath != NULL)
  {
    pathLen = _CleanPath(pPath, kpPath);

    if (pathLen > 0)
    {
      KERNEL_LOCK(sMountPointLock);

      /* Search for the node */
      pNode = _FindNode(spRootPoint, pPath, pathLen, &nextToken);
      /* Check that the driver does not exist */
      if (pNode != NULL)
      {
        /* Get the driver */
        if (pNode->pDriver != NULL)
        {
          /* Handle with the dedicated driver */
          pDriver = pNode->pDriver;
          pDriverData = pDriver->pDriverData;
        }
        else if (nextToken == pathLen)
        {
          /* When the full path is a generic VFS node, handle with generic VFS */
          pDriver = &sVFSGenericDriver;
          pDriverData = pNode;
        }
        else
        {
          /* Not found */
          pDriver = NULL;
        }

        KERNEL_UNLOCK(sMountPointLock);

        if (pDriver != NULL)
        {
          pTable = SchedulerGetCurrentProcess()->pFileDescriptorTable;
          pHandle = pDriver->pOpen(pDriverData,
                                  pPath + nextToken,
                                  flags,
                                  mode);
          if (pHandle != (void*)-1)
          {
            newFD = _CreateFileDescriptor(pTable,
                                          pDriver,
                                          pHandle,
                                          pPath,
                                          flags,
                                          mode);
            if (newFD == -1)
            {
              pDriver->pClose(pDriver->pDriverData, pPath + nextToken);
            }
          }
          else
          {
            newFD = -1;
          }
        }
        else
        {
          newFD = -1;
        }
      }
      else
      {
        KERNEL_UNLOCK(sMountPointLock);
        newFD = -1;
      }
    }
    else
    {
      newFD = -1;
    }

    KFree(pPath, KMALLOC_PROCESS_HEAP);
  }
  else
  {
    newFD = -1;
  }

  return newFD;
}

int32_t VFSClose(int32_t fd)
{
  S_FSDriver*       pDriver;
  S_FDTable*        pTable;
  S_FileDescriptor* pDesc;
  E_Return          error;
  int32_t           retVal;

  pTable = SchedulerGetCurrentProcess()->pFileDescriptorTable;

  KERNEL_LOCK(pTable->lock);

  /* Get the internal file descriptor */
  error = _GetFileDescriptor(pTable, fd, &pDesc);

  KERNEL_UNLOCK(pTable->lock);
  if (error == NO_ERROR)
  {

    KERNEL_LOCK(pDesc->pShared->lock);
    pDriver = pDesc->pShared->pDriver;
    /* Close file only when we are the last to have it opened */
    if (pDriver->pClose != NULL)
    {
      retVal = pDriver->pClose(pDriver->pDriverData,
                                pDesc->pShared->pFileHandle);
    }
    else
    {
      retVal = 0;
    }

    _DestroyFileDescriptor(pTable, fd);

    KERNEL_UNLOCK(pDesc->pShared->lock);
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

ssize_t VFSRead(int32_t fd, void* pBuffer, size_t count)
{
  S_FDTable*        pTable;
  S_FileDescriptor* pDesc;
  E_Return          error;
  int32_t           retVal;
  T_VFSRead         pRead;
  void*             pHandle;
  void*             pDriverData;

  pTable = SchedulerGetCurrentProcess()->pFileDescriptorTable;

  KERNEL_LOCK(pTable->lock);

  /* Get the internal file descriptor */
  error = _GetFileDescriptor(pTable, fd, &pDesc);

  KERNEL_UNLOCK(pTable->lock);
  if (error == NO_ERROR)
  {
    /* Check permissions */
    if ((pDesc->openFlags & VFS_PERMISSION_READ) != 0)
    {
      KERNEL_LOCK(pDesc->pShared->lock);
      pDriverData = pDesc->pShared->pDriver->pDriverData;
      pRead       = pDesc->pShared->pDriver->pRead;
      pHandle     = pDesc->pShared->pFileHandle;
      KERNEL_UNLOCK(pDesc->pShared->lock);

      /* Read the file when available */
      if (pRead != NULL)
      {
        retVal = pRead(pDriverData, pHandle, pBuffer, count);
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
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

ssize_t VFSWrite(int32_t fd, const void* kpBuffer, size_t count)
{
  S_FDTable*        pTable;
  S_FileDescriptor* pDesc;
  E_Return          error;
  int32_t           retVal;
  T_VFSWrite        pWrite;
  void*             pHandle;
  void*             pDriverData;

  pTable = SchedulerGetCurrentProcess()->pFileDescriptorTable;

  KERNEL_LOCK(pTable->lock);

  /* Get the internal file descriptor */
  error = _GetFileDescriptor(pTable, fd, &pDesc);

  KERNEL_UNLOCK(pTable->lock);
  if (error == NO_ERROR)
  {
    /* Check permissions */
    if ((pDesc->openFlags & VFS_PERMISSION_WRITE) != 0)
    {
      KERNEL_LOCK(pDesc->pShared->lock);
      pDriverData = pDesc->pShared->pDriver->pDriverData;
      pWrite      = pDesc->pShared->pDriver->pWrite;
      pHandle     = pDesc->pShared->pFileHandle;
      KERNEL_UNLOCK(pDesc->pShared->lock);

      /* Write the file when available */
      if (pWrite != NULL)
      {
        retVal = pWrite(pDriverData, pHandle, kpBuffer, count);
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
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

int32_t VFSReaddir(int32_t fd, S_DirectoryEntry* pDirEntry)
{
  S_FDTable*        pTable;
  S_FileDescriptor* pDesc;
  E_Return          error;
  int32_t           retVal;
  T_VFSReadDir      pReadDir;
  void*             pHandle;
  void*             pDriverData;

  pTable = SchedulerGetCurrentProcess()->pFileDescriptorTable;

  KERNEL_LOCK(pTable->lock);

  /* Get the internal file descriptor */
  error = _GetFileDescriptor(pTable, fd, &pDesc);

  KERNEL_UNLOCK(pTable->lock);
  if (error == NO_ERROR)
  {
    /* Check permissions */
    if ((pDesc->openFlags & VFS_PERMISSION_READ) != 0)
    {
      KERNEL_LOCK(pDesc->pShared->lock);
      pDriverData = pDesc->pShared->pDriver->pDriverData;
      pReadDir    = pDesc->pShared->pDriver->pReadDir;
      pHandle     = pDesc->pShared->pFileHandle;
      KERNEL_UNLOCK(pDesc->pShared->lock);

      /* Read the directory when available */
      if (pReadDir != NULL)
      {
        retVal = pReadDir(pDriverData, pHandle, pDirEntry);
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
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

int32_t VFSIOCTL(int32_t fd, uint32_t operation, void* pArgs)
{
  S_FDTable*        pTable;
  S_FileDescriptor* pDesc;
  E_Return          error;
  int32_t           retVal;
  T_VFSIOCTL        pIOCTL;
  void*             pHandle;
  void*             pDriverData;

  pTable = SchedulerGetCurrentProcess()->pFileDescriptorTable;

  KERNEL_LOCK(pTable->lock);

  /* Get the internal file descriptor */
  error = _GetFileDescriptor(pTable, fd, &pDesc);

  KERNEL_UNLOCK(pTable->lock);
  if (error == NO_ERROR)
  {
    /* Check permissions */
    if ((pDesc->openFlags & VFS_PERMISSION_READ) != 0)
    {
      KERNEL_LOCK(pDesc->pShared->lock);
      pDriverData = pDesc->pShared->pDriver->pDriverData;
      pIOCTL      = pDesc->pShared->pDriver->pIOCTL;
      pHandle     = pDesc->pShared->pFileHandle;
      KERNEL_UNLOCK(pDesc->pShared->lock);

      /* IOCLT when available */
      if (pIOCTL != NULL)
      {
        retVal = pIOCTL(pDriverData, pHandle, operation, pArgs);
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
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

E_Return VFSMount(const char* kpPath,
                  const char* kpDevPath,
                  const char* kpFsName)
{
  uintptr_t   driverTableCursor;
  S_FSDriver* pDriver;
  E_Return    retCode;
  void*       pDriverMountData;
  S_FSDriver* pNewDriver;
  char*       pPath;

  if (kpFsName != NULL && kpDevPath != NULL && kpPath != NULL)
  {
    /* Allocate the path and clean it */
    pPath = KMalloc(VFS_PATH_MAX_LENGTH, ALIGN_1_BYTE, KMALLOC_FREE_POOL);
    _CleanPath(pPath, kpPath);

    /* Search for a registered driver */
    driverTableCursor = (uintptr_t)&_START_FS_TABLE_ADDR;
    pDriver = *(S_FSDriver**)driverTableCursor;

    while(pDriver != NULL)
    {
      if(strcmp(pDriver->pName, kpFsName) == 0)
      {
        break;
      }
      driverTableCursor += sizeof(uintptr_t);
      pDriver = *(S_FSDriver**)driverTableCursor;
    }

    if (pDriver != NULL)
    {
      /* Mount using the appropriate driver */
      retCode = pDriver->pMount(pPath, kpDevPath, &pDriverMountData);

      if (retCode == NO_ERROR)
      {
        /* Register the driver */
        pNewDriver = RegisterVFSDriver(pPath,
                                       pDriverMountData,
                                       pDriver->pOpen,
                                       pDriver->pClose,
                                       pDriver->pRead,
                                       pDriver->pWrite,
                                       pDriver->pReadDir,
                                       pDriver->pIOCTL);

        if(pNewDriver != VFS_DRIVER_INVALID)
        {
          /* Add the driver data for unmount */
          pNewDriver->pMount   = pDriver->pMount;
          pNewDriver->pUnmount = pDriver->pUnmount;
        }
        else
        {
          retCode = ERR_NOT_SUPPORTED;
        }
      }
    }
    else
    {
      retCode = ERR_NOT_SUPPORTED;
    }

    /* Release clean path */
    KFree(pPath, KMALLOC_FREE_POOL);
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

E_Return VFSUnmount(const char* kpPath)
{
  S_FSDriver* pDriver;
  S_VFSNode*  pNode;
  size_t      pathLen;
  size_t      nextToken;
  char*       pPath;
  E_Return    retCode;

  /* Allocate the path and clean it */
  pPath = KMalloc(VFS_PATH_MAX_LENGTH, ALIGN_1_BYTE, KMALLOC_FREE_POOL);
  pathLen = _CleanPath(pPath, kpPath);

  if (pathLen > 0)
  {
    KERNEL_LOCK(sMountPointLock);

    /* Search for an existing driver */
    pNode = _FindNode(spRootPoint, pPath, pathLen, &nextToken);
    /* Check that the driver does exist */
    if (pNode != NULL && nextToken == pathLen)
    {
      pDriver = pNode->pDriver;
      /* Remove the driver */
      if (pDriver != NULL)
      {
        retCode = pDriver->pUnmount(pDriver->pDriverData);
        if (retCode == NO_ERROR)
        {
          /* Unlink the driver */
          pNode->pDriver = NULL;

          /* Release the driver */
          KFree(pDriver, KMALLOC_FREE_POOL);

          /* Clear the mount point */
          _RemoveDriverNode(pNode);
        }
      }
      else
      {
        retCode = ERR_INVALID_PARAMETER;
      }
    }
    else
    {
      retCode = ERR_INVALID_PARAMETER;
    }

    KERNEL_UNLOCK(sMountPointLock);
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  /* Release clean path */
  KFree(pPath, KMALLOC_FREE_POOL);

  return retCode;
}

ssize_t VFSGetNextPathTokenPosition(const char* kpStr, const size_t kStrSize)
{
  ssize_t nextToken;

  for (nextToken = 0; (size_t)nextToken < kStrSize; ++nextToken)
  {
    if (kpStr[nextToken] == VFS_PATH_DELIMITER)
    {
      ++nextToken;
      break;
    }
  }

  if ((size_t)nextToken == kStrSize)
  {
    nextToken = -1;
  }

  return nextToken;
}

/************************************ EOF *************************************/