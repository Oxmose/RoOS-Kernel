/*******************************************************************************
 * @file USTARFS.c
 *
 * @see USTARFS.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/07/2024
 *
 * @version 2.0
 *
 * @brief Kernel's USTAR filesystem driver.
 *
 * @details Kernel's USTAR filesystem driver. Defines the functions and
 * structures used by the kernel to manage USTAR partitions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <IOCTL.h>
#include <Panic.h>
#include <stdint.h>
#include <VirtualFS.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelMutex.h>

/* Configuration files */
#include <config.h>

/* Unit test header TODO */
#include <TestFramework.h>

/* Header file */
#include <USTARFS.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Module name */
#define MODULE_NAME "USTAR"

/** @brief USTAR magic value. */
#define USTAR_MAGIC "ustar "
/** @brief USTAR maximal filename length. */
#define USTAR_FILENAME_MAX_LENGTH 100
/** @brief USTAR block size. */
#define USTAR_BLOCK_SIZE 512
/** @brief USTAR file size maximal length. */
#define USTAR_FSIZE_FIELD_LENGTH 12
/** @brief USTAR last edit maximal length. */
#define USTAR_LEDIT_FIELD_LENGTH 12
/** @brief USTAR file user ID maximal length. */
#define USTAR_UID_FIELD_LENGTH 8
/** @brief USTAR file mode maximal length. */
#define USTAR_MODE_FIELD_LENGTH 8
/** @brief USTAR file prefix maximal length. */
#define USTAR_PREFIX_NAME_LENGTH 155

/** @brief USTAR User read permission bitmask. */
#define T_UREAD  0x100
/** @brief USTAR User write permission bitmask. */
#define T_UWRITE 0x080
/** @brief USTAR User execute permission bitmask. */
#define T_UEXEC  0x040

/** @brief USTAR Group read permission bitmask. */
#define T_GREAD  0x020
/** @brief USTAR Group write permission bitmask. */
#define T_GWRITE 0x010
/** @brief USTAR Group execute permission bitmask. */
#define T_GEXEC  0x008

/** @brief USTAR Other read permission bitmask. */
#define T_OREAD  0x004
/** @brief USTAR Other write permission bitmask. */
#define T_OWRITE 0x002
/** @brief USTAR Other execute permission bitmask. */
#define T_OEXEC  0x001

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief USTAR header block definition as per USTAR standard.
 */
typedef struct
{
  /** @brief USTAR file name */
  char fileName[USTAR_FILENAME_MAX_LENGTH];
  /** @brief USTAR file mode */
  char mode[8];
  /** @brief USTAR owner user id */
  char useId[8];
  /** @brief USTAR owner group id */
  char groupId[8];
  /** @brief Length of the file in bytes */
  char size[USTAR_FSIZE_FIELD_LENGTH];
  /** @brief Modify time of file */
  char lastEdited[12];
  /** @brief Checksum for header */
  char checksum[8];
  /** @brief Type of file */
  char type;
  /** @brief Name of linked file */
  char linkedFileName[USTAR_FILENAME_MAX_LENGTH];
  /** @brief USTAR magic value */
  char magic[6];
  /** @brief USTAR version */
  char ustarVersion[2];
  /** @brief Owner user name */
  char userName[32];
  /** @brief Owner group name */
  char groupName[32];
  /** @brief Device major number */
  char devMajor[8];
  /** @brief Device minor number */
  char devMinor[8];
  /** @brief Prefix for file name */
  char prefix[USTAR_PREFIX_NAME_LENGTH];
  /** @brief Unused padding */
  char padding[12];
} S_USTARBlock;

/** @brief USTAR mount driver data */
typedef struct
{
  /** @brief Device file descriptor */
  int32_t devFd;
  /** @brief Mount lock */
  S_KernelMutex lock;
} S_USTARMountData;

/** @brief USTAR file types */
typedef enum
{
  /** @brief Normal file */
  FILE = 0,
  /** @brief Hard link */
  HARD_LINK = 1,
  /** @brief Symbolic link */
  SYM_LINK = 2,
  /** @brief Character device */
  CHAR_DEV = 3,
  /** @brief Block device */
  BLOCK_DEV = 4,
  /** @brief Directory */
  DIRECTORY = 5,
  /** @brief Named pipe (FIFO) */
  NAMED_PIPE = 6
} E_USTARFileType;

/** @brief USTAR internal file descriptor */
typedef struct
{
  /** @brief Current offset in the file */
  ssize_t offset;
  /** @brief File start offset in the device */
  ssize_t devFdOffset;
  /** @brief Size of the file */
  size_t fileSize;
  /** @brief Type of file */
  E_USTARFileType type;
  /** @brief File name */
  char name[USTAR_FILENAME_MAX_LENGTH];
} S_USTARFileDescriptor;

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
#define USTAR_ASSERT(COND, MSG, ERROR) {                \
  if ((COND) == false)                                   \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);       \
  }                                                     \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Mount function for the filesystem.
 *
 * @details Mount function for the filesystem. This function will link a
 * directory to the associated device and use the filesystem to access it.
 *
 * @param[in] kpPath The path of the directory to mount to.
 * @param[in] kpDevPath The path of the device to mount.
 * @param[in, out] pDriverMountData Data generated by the driver when during
 * the mount operation.
 *
 * @return The function returns the success or error status.
 */
static E_Return _Mount(const char* kpPath,
                       const char* kpDevPath,
                       void**      pDriverMountData);

/**
 * @brief Unmount function for the filesystem.
 *
 * @details Unmount function for the filesystem. This function will unlink a
 * directory to the associated device.
 *
 * @param[in, out] pDriverMountData The driver data generated when mounting
 * the filesystem.
 *
 * @return The function returns the success or error status.
 */
static E_Return _Unmount(void* pDriverMountData);

/**
 * @brief USTAR VFS open hook.
 *
 * @details USTAR VFS open hook. This function returns a handle to control the
 * ustar driver through VFS.
 *
 * @param[in, out] pDrvCtrl The ustar driver that was registered in the VFS.
 * @param[in] kpPath The path in the ustar driver mount point.
 * @param[in] flags The open flags.
 * @param[in] mode File open mode.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode);

/**
 * @brief USTAR VFS close hook.
 *
 * @details USTAR VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The ustar driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief USTAR VFS read hook.
 *
 * @details USTAR VFS read hook. This function reads from a USTAR file.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _VFSRead(void*  pDrvCtrl,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count);

/**
 * @brief USTAR VFS write hook.
 *
 * @details USTAR VFS write hook. This functio writes a file in the USTAR
 * partition.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _VFSWrite(void*       pDrvCtrl,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count);

/**
 * @brief USTAR VFS IOCTL hook.
 *
 * @details USTAR VFS IOCTL hook. This function performs the IOCTL for the USTAR
 * driver.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _VFSIOCTL(void*    pDriverData,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs);

/**
 * @brief USTAR VFS ReadDir hook.
 *
 * @details USTAR VFS ReadDir hook. This function performs the ReadDir for the
 * USTAR driver.
 *
 * @param[in, out] pDrvCtrl The ustar VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSReadDir(void*             pDriverData,
                           void*             pHandle,
                           S_DirectoryEntry* pDirEntry);

/**
 * @brief USTAR VFS seek hook.
 *
 * @details USTAR VFS seek hook. This function performs a seek for the
 * USTAR driver.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the seek operation.
 *
 * @return The function returns the new offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _VFSSeek(void*                 pDriverData,
                        void*                 pHandle,
                        S_SeekIOCTLArguments* pArgs);

/**
 * @brief USTAR VFS tell hook.
 *
 * @details USTAR VFS tell hook. This function performs a tell for the
 * USTAR driver.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the tell operation.
 *
 * @return The function returns the offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _VFSTell(void* pDriverData, void* pHandle, void* pArgs);

/**
 * @brief Fills the USTAR block with the next file in the partition.
 *
 * @details Fills the USTAR block with the next file in the partition. This
 * file might not be in the same folder as the previous file.
 *
 * @param[in] devFd The USTAR partition file descriptor.
 * @param[in,out] pBlock The filer header's block to use to get the next header.
 * This will be filled with the next header once the operation completed.
 * @param[in,out] pBlockId The block ID of the current file. This will be filled
 * with the block ID of the next file in the partition.
 */
static void _GetNextFile(int32_t       devFd,
                         S_USTARBlock* pBlock,
                         uint32_t*     pBlockId);

/**
 * @brief Checks if a USTAR header block is valid.
 *
 * @details Checks if a USTAR header block is valid. The integrity of the header
 * is validated with the checksum, magix and other values contained in the
 * header block.
 *
 * @param[in] kpBlock The header block to check.
 *
 * @return NO_ERROR is retuned is the block is valid. Otherwise, an error is
 * returned.
 */
inline static E_Return _CheckBlock(const S_USTARBlock* kpBlock);

/**
 * @brief Translates the octal value giben as parameter to its decimal value.
 *
 * @details Translates the octal value giben as parameter to its decimal value.
 *
 * @param[in] kpOct The octal value to translated.
 * @param[in] size The size of the buffer that contains the octal value.
 *
 * @return The decimal value of the octal value stored in the string is
 * returned.
 */
inline static uint32_t _Oct2uint(const char* kpOct, size_t size);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief USTAR driver instance. */
static S_FSDriver sUSTARDriver =
{
  .pName         = "ustar",
  .pMount        = _Mount,
  .pUnmount      = _Unmount,
  .pOpen         = _VFSOpen,
  .pClose        = _VFSClose,
  .pRead         = _VFSRead,
  .pWrite        = _VFSWrite,
  .pReadDir      = _VFSReadDir,
  .pIOCTL        = _VFSIOCTL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static E_Return _Mount(const char* kpPath,
                       const char* kpDevPath,
                       void**      pDriverMountData)
{
  (void)kpPath;
  ssize_t              readSize;
  int32_t              retCode;
  S_USTARMountData*    pData;
  S_USTARBlock         currentBlock;
  S_SeekIOCTLArguments seekArgs;
  E_Return             err;

  pData = KMalloc(sizeof(S_USTARMountData), KMALLOC_FREE_POOL);
  if (pData != NULL)
  {
    /* Open the device file descriptor */
    pData->devFd = VFSOpen(kpDevPath, O_RDWR, 0);
    if (pData->devFd >= 0)
    {
      /* Read the first 512 bytes (USTAR block) */
      seekArgs.direction = SEEK_SET;
      seekArgs.offset = 0;
      retCode = VFSIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
      if (retCode >= 0)
      {
        readSize = VFSRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
        if (readSize == USTAR_BLOCK_SIZE)
        {
           /* Check if USTAR */
          err = _CheckBlock(&currentBlock);
          if (err == NO_ERROR)
          {
            err = KernelMutexInit(&pData->lock, KMUTEX_FLAG_QUEUING_PRIO);
            if (err != NO_ERROR)
            {
              KFree(pData);
            }
            else
            {
              *pDriverMountData = pData;
            }
          }
          else
          {
            KFree(pData);
          }
        }
        else
        {
          KFree(pData);
          err = ERR_INVALID_VALUE;
        }
      }
      else
      {
        KFree(pData);
        err = ERR_INVALID_VALUE;
      }
    }
    else
    {
      KFree(pData);
      err = ERR_INVALID_VALUE;
    }
  }
  else
  {
    err = ERR_NO_MEMORY;
  }

  return err;

  return NO_ERROR;
}

static E_Return _Unmount(void* pDriverMountData)
{
  S_USTARMountData*    pData;
  int32_t              retCode;
  ssize_t              readSize;
  S_USTARBlock         currentBlock;
  S_SeekIOCTLArguments seekArgs;
  E_Return             err;

  if (pDriverMountData != NULL)
  {
    pData = (S_USTARMountData*)pDriverMountData;

    /* Read the first 512 bytes (USTAR block) */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retCode = VFSIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if (retCode >= 0)
    {
      readSize = VFSRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
      if (readSize == USTAR_BLOCK_SIZE)
      {
        /* Check if USTAR */
        err = _CheckBlock(&currentBlock);
        if (err == NO_ERROR)
        {
          /* Close the file descriptor */
          retCode = VFSClose(pData->devFd);
          if (retCode >= 0)
          {
            KFree(pData);
          }
          else
          {
            err = ERR_INVALID_VALUE;
          }
        }
      }
      else
      {
        err = ERR_INVALID_VALUE;
      }
    }
    else
    {
      err = ERR_INVALID_VALUE;
    }
  }
  else
  {
    err = ERR_INVALID_PARAMETER;
  }

  return err;
}

static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode)
{
  S_USTARMountData*      pData;
  S_USTARBlock           currentBlock;
  bool                   found;
  E_Return               err;
  uint32_t               blockId;
  S_SeekIOCTLArguments   seekArgs;
  int32_t                retCode;
  ssize_t                readSize;
  S_USTARFileDescriptor* pFileDesc;
  size_t                 pathLen;
  size_t                 fileLen;

  (void)mode;

  if (pDrvCtrl != NULL && kpPath != NULL && flags == O_RDONLY)
  {
    /* If we do not open the root */
    if (*kpPath != 0)
    {
      /* USTAR max path is 100 character */
      if (strnlen(kpPath, VFS_PATH_MAX_LENGTH) <= USTAR_FILENAME_MAX_LENGTH)
      {
        pData = (S_USTARMountData*)pDrvCtrl;

        err = KernelMutexLock(&pData->lock);
        if (err == NO_ERROR)
        {
          /* Read the first 512 bytes (USTAR block) */
          seekArgs.direction = SEEK_SET;
          seekArgs.offset = 0;
          retCode = VFSIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
          if (retCode >= 0)
          {
            readSize = VFSRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
            if (readSize == USTAR_BLOCK_SIZE)
            {
              err = _CheckBlock(&currentBlock);
              if (err == NO_ERROR)
              {
                found   = false;
                blockId = 0;

                /* Search for the file, if first filename character is NULL,
                 * we reached the end of the search.
                 */
                pathLen = strnlen(kpPath, VFS_PATH_MAX_LENGTH);
                while (currentBlock.fileName[0] != 0)
                {
                  /* If the current file is a directory */
                  fileLen = strnlen(currentBlock.fileName,
                                    USTAR_FILENAME_MAX_LENGTH);
                  if (currentBlock.fileName[fileLen - 1] == VFS_PATH_DELIMITER)
                  {
                    if ((kpPath[pathLen - 1] != VFS_PATH_DELIMITER) &&
                        (pathLen == fileLen - 1))
                    {
                      if (strncmp(kpPath, currentBlock.fileName, pathLen) == 0)
                      {
                        found = true;
                        err   = _CheckBlock(&currentBlock);
                        break;
                      }
                    }
                  }
                  else if (strncmp(kpPath,
                                  currentBlock.fileName,
                                  USTAR_FILENAME_MAX_LENGTH) == 0)
                  {
                      found = true;
                      err   = _CheckBlock(&currentBlock);
                      break;
                  }
                  _GetNextFile(pData->devFd, &currentBlock, &blockId);
                }

                if (found == true && err == NO_ERROR)
                {
                  /* Create the file descriptor */
                  pFileDesc = KMallocUser(sizeof(S_USTARFileDescriptor), NULL);
                  if (pFileDesc != NULL)
                  {
                    /* Setup file descriptor */
                    pFileDesc->offset = 0;
                    pFileDesc->type = currentBlock.type - '0';
                    pFileDesc->devFdOffset = VFSIOCTL(pData->devFd,
                                                      VFS_IOCTL_FILE_TELL,
                                                      NULL);
                    if (pFileDesc->devFdOffset >= 0)
                    {
                      fileLen = strnlen(currentBlock.fileName,
                                        USTAR_FILENAME_MAX_LENGTH);
                      memcpy(pFileDesc->name, currentBlock.fileName, fileLen);
                      pFileDesc->name[fileLen] = 0;

                      /* Get the file data */
                      pFileDesc->fileSize = _Oct2uint(currentBlock.size,
                                                      USTAR_FSIZE_FIELD_LENGTH);
                    }
                    else
                    {
                      KFreeUser(pFileDesc, NULL);
                      pFileDesc = (void*)-1;
                    }
                  }
                  else
                  {
                    pFileDesc = (void*)-1;
                  }
                }
                else
                {
                  pFileDesc = (void*)-1;
                }
              }
              else
              {
                pFileDesc = (void*)-1;
              }
            }
            else
            {
              pFileDesc = (void*)-1;
            }
          }
          else
          {
            pFileDesc = (void*)-1;
          }

          err = KernelMutexUnlock(&pData->lock);
          USTAR_ASSERT(err == NO_ERROR,  "Failed to unlock mutex", err);
        }
        else
        {
          pFileDesc = (void*)-1;
        }
      }
      else
      {
        pFileDesc = (void*)-1;
      }
    }
    else
    {
      /* Create the file descriptor */
      pFileDesc = KMallocUser(sizeof(S_USTARFileDescriptor), NULL);
      if (pFileDesc != NULL)
      {
        /* Setup file descriptor */
        pFileDesc->offset = 0;
        pFileDesc->type = DIRECTORY;
        pFileDesc->devFdOffset = 0;
        pFileDesc->fileSize = 0;
        pFileDesc->name[0] = 0;
      }
      else
      {
        pFileDesc = (void*)-1;
      }
    }
  }
  else
  {
    pFileDesc = (void*)-1;
  }

  return pFileDesc;
}

static int32_t _VFSClose(void* pDrvCtrl, void* pHandle)
{
  S_USTARFileDescriptor* pFileDesc;
  int32_t                retCode;

  if (pDrvCtrl != NULL && pHandle != NULL && pHandle != (void*)-1)
  {
    pFileDesc = (S_USTARFileDescriptor*)pHandle;

    /* Free the descriptor */
    pFileDesc->devFdOffset = -1;
    pFileDesc->offset      = -1;
    KFreeUser(pHandle, NULL);

    retCode = 0;
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _VFSRead(void*  pDrvCtrl,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count)
{
  S_USTARFileDescriptor* pFileDesc;
  S_USTARBlock           currentBlock;
  ssize_t                readSize;
  ssize_t                devReadSize;
  ssize_t                dataRead;
  ssize_t                retVal;
  size_t                 blockOffset;
  S_USTARMountData*      pData;
  S_SeekIOCTLArguments   seekArgs;
  E_Return               err;

  if (pDrvCtrl != NULL &&
      pHandle != NULL &&
      pHandle != (void*)-1 &&
      pBuffer != NULL)
  {
    /* Check handle */
    pFileDesc = (S_USTARFileDescriptor*)pHandle;
    if (pFileDesc->devFdOffset >= 0 &&
        pFileDesc->offset >= 0 &&
        pFileDesc->type == FILE)
    {
      pData = (S_USTARMountData*)pDrvCtrl;

      /* Get the maximal size to read */
      readSize = MIN(count, pFileDesc->fileSize - pFileDesc->offset);
      if (readSize > 0)
      {
        /* Set the device position */
        seekArgs.direction = SEEK_SET;
        seekArgs.offset    = pFileDesc->devFdOffset +
                             ((pFileDesc->offset / USTAR_BLOCK_SIZE) *
                             USTAR_BLOCK_SIZE);

        err = KernelMutexLock(&pData->lock);
        if (err == NO_ERROR)
        {
          retVal = VFSIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
          if (retVal >= 0)
          {
            /* Read while we can */
            retVal = 0;
            while (readSize > 0)
            {
              /* Read the current block */
              devReadSize = VFSRead(pData->devFd, &currentBlock,
                                    USTAR_BLOCK_SIZE);
              if (devReadSize == USTAR_BLOCK_SIZE)
              {
                /* Get how much data we should read from the current block */
                blockOffset = pFileDesc->offset % USTAR_BLOCK_SIZE;
                if (blockOffset != 0)
                {
                  dataRead = MIN((ssize_t)(USTAR_BLOCK_SIZE - blockOffset),
                                 readSize);
                }
                else
                {
                  dataRead = MIN(USTAR_BLOCK_SIZE, readSize);
                }

                /* Copy to buffer */
                memcpy((char*)pBuffer + retVal,
                       ((uint8_t*)&currentBlock) + blockOffset,
                       dataRead);

                /* Update offsets */
                readSize -= dataRead;
                retVal   += dataRead;
                pFileDesc->offset += dataRead;
              }
              else
              {
                break;
              }
            }
          }
          else
          {
            retVal = -1;
          }

          err = KernelMutexUnlock(&pData->lock);
          USTAR_ASSERT(err == NO_ERROR, "Failed to unlock acquired mutex", err);
        }
        else
        {
          retVal = -1;
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
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _VFSWrite(void*       pDrvCtrl,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count)
{
  (void)pDrvCtrl;
  (void)pHandle;
  (void)kpBuffer;
  (void)count;

  /* Not supported */
  return -1;
}

static ssize_t _VFSIOCTL(void*    pDriverData,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs)
{
  ssize_t retVal;

  switch(operation)
  {
    case VFS_IOCTL_FILE_SEEK:
      retVal = _VFSSeek(pDriverData, pHandle, pArgs);
      break;
    case VFS_IOCTL_FILE_TELL:
      retVal = _VFSTell(pDriverData, pHandle, pArgs);
      break;
    default:
      retVal = -1;
  }

  return retVal;
}

static int32_t _VFSReadDir(void*             pDriverData,
                           void*             pHandle,
                           S_DirectoryEntry* pDirEntry)
{
  S_USTARMountData*      pData;
  S_USTARBlock           currentBlock;
  uint32_t               foundCount;
  E_Return               err;
  uint32_t               blockId;
  S_SeekIOCTLArguments   seekArgs;
  int32_t                retCode;
  S_USTARFileDescriptor* pFileDesc;
  size_t                 pathSize;
  size_t                 filePathSize;
  ssize_t                readSize;
  ssize_t                firstOffset;
  bool                   found;

  if (pDriverData != NULL && pHandle != NULL && pDirEntry != NULL)
  {
    pFileDesc = pHandle;
    if (pFileDesc->type == DIRECTORY && pFileDesc->offset != -1)
    {
      pData = (S_USTARMountData*)pDriverData;

      err = KernelMutexLock(&pData->lock);
      if (err == NO_ERROR)
      {
        /* Read the first 512 bytes (USTAR block) */
        seekArgs.direction = SEEK_SET;
        seekArgs.offset    = 0;
        retCode = VFSIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
        if (retCode >= 0)
        {
          readSize = VFSRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
          if (readSize == USTAR_BLOCK_SIZE)
          {
            err = _CheckBlock(&currentBlock);
            if (err == NO_ERROR)
            {
              retCode     = 0;
              foundCount  = 0;
              blockId     = 0;
              pathSize    = strnlen(pFileDesc->name, USTAR_FILENAME_MAX_LENGTH);
              firstOffset = pFileDesc->offset;
              /* Search for the file, if first filename character is NULL, we
               * reached the end of the search
               */
              while (currentBlock.fileName[0] != 0)
              {
                err = _CheckBlock(&currentBlock);
                if (err != NO_ERROR)
                {
                  retCode = -1;
                  break;
                }

                /* Check if we are in the root */
                if (pathSize == 0)
                {
                  for (filePathSize = 0;
                      *(currentBlock.fileName + filePathSize) != 0;
                      ++filePathSize)
                  {
                    if (*(currentBlock.fileName + filePathSize) ==
                      VFS_PATH_DELIMITER)
                    {
                      ++filePathSize;
                      break;
                    }
                  }

                  if (*(currentBlock.fileName + filePathSize) == 0)
                  {
                    if (foundCount == pFileDesc->offset)
                    {
                      if (pFileDesc->type == FILE)
                      {
                        pDirEntry->type  = VFS_FILE_TYPE_FILE;
                      }
                      else
                      {
                        pDirEntry->type  = VFS_FILE_TYPE_DIR;
                      }

                      filePathSize = strnlen(currentBlock.fileName,
                                             USTAR_FILENAME_MAX_LENGTH);

                      if (filePathSize - pathSize > VFS_FILENAME_MAX_LENGTH)
                      {
                        retCode = -1;
                        break;
                      }

                      memcpy(pDirEntry->pName,
                            currentBlock.fileName + pathSize,
                            filePathSize - pathSize);
                      pDirEntry->pName[filePathSize - pathSize] = 0;

                      ++pFileDesc->offset;
                      break;
                    }
                    else
                    {
                      ++foundCount;
                    }
                  }
                }
                /* Check if the path is the same */
                else if (strncmp(pFileDesc->name,
                                currentBlock.fileName,
                                pathSize) == 0)
                {
                  /* Check if this is the same folder */
                  if (pathSize != strnlen(currentBlock.fileName,
                                          USTAR_FILENAME_MAX_LENGTH))
                  {
                    /* Check if this is a direct child */
                    found = true;
                    for (filePathSize = pathSize;
                        *(currentBlock.fileName + filePathSize) != 0;
                        ++filePathSize)
                    {
                      if (*(currentBlock.fileName + filePathSize) ==
                          VFS_PATH_DELIMITER &&
                      *(currentBlock.fileName + filePathSize + 1) != 0)
                      {
                        found = false;
                        break;
                      }
                    }

                    /* If this is a folder */
                    if (found == true)
                    {
                      if (foundCount == pFileDesc->offset)
                      {
                        if (pFileDesc->type == FILE)
                        {
                          pDirEntry->type  = VFS_FILE_TYPE_FILE;
                        }
                        else
                        {
                          pDirEntry->type  = VFS_FILE_TYPE_DIR;
                        }

                        filePathSize = strnlen(currentBlock.fileName,
                                               USTAR_FILENAME_MAX_LENGTH);

                        if (filePathSize - pathSize > VFS_FILENAME_MAX_LENGTH)
                        {
                          retCode = -1;
                          break;
                        }
                        memcpy(pDirEntry->pName,
                              currentBlock.fileName + pathSize,
                              filePathSize - pathSize);
                        pDirEntry->pName[filePathSize - pathSize] = 0;

                        ++pFileDesc->offset;
                        break;
                      }
                      else
                      {
                        ++foundCount;
                      }
                    }
                  }
                }
                _GetNextFile(pData->devFd, &currentBlock, &blockId);
              }
            }
            else
            {
              retCode = -1;
            }
          }
          else
          {
            retCode = -1;
          }
        }
        else
        {
          retCode = -1;
        }

        err = KernelMutexUnlock(&pData->lock);
        USTAR_ASSERT(err == NO_ERROR, "Failed to unlock acquired mutex", err);

        /* If we found the the same as the offset */
        if (retCode != -1)
        {
          if (firstOffset != pFileDesc->offset)
          {
            retCode = 1;
          }
          else
          {
            pFileDesc->offset = -1;
            retCode = -1;
          }
        }
      }
      else
      {
        retCode = -1;
      }
    }
    else
    {
      retCode = -1;
    }
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static void _GetNextFile(int32_t       devFd,
                         S_USTARBlock* pBlock,
                         uint32_t*     pBlockId)
{
  E_Return             err;
  uint32_t             size;
  int32_t              retCode;
  ssize_t              readSize;
  S_SeekIOCTLArguments seekArgs;

  /* We loop over all possible empty names (removed files) */
  do
  {
    err = _CheckBlock(pBlock);
    if (err != NO_ERROR)
    {
      /* Block not found */
      pBlock->fileName[0] = 0;
      break;
    }

    /* Next block is current block + file size / block_size */
    size = _Oct2uint(pBlock->size, USTAR_FSIZE_FIELD_LENGTH - 1) +
                                   USTAR_BLOCK_SIZE;

    if (size % USTAR_BLOCK_SIZE != 0)
    {
      size += USTAR_BLOCK_SIZE - (size % USTAR_BLOCK_SIZE);
    }

    *pBlockId = *pBlockId + size / USTAR_BLOCK_SIZE;

    /* Read the first 512 bytes (USTAR block) */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = (size_t)(*pBlockId * USTAR_BLOCK_SIZE);
    retCode = VFSIOCTL(devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);

    if (retCode < 0)
    {
      pBlock->fileName[0] = 0;
      break;
    }

    readSize = VFSRead(devFd, pBlock, USTAR_BLOCK_SIZE);
    if (readSize != USTAR_BLOCK_SIZE)
    {
      pBlock->fileName[0] = 0;
      break;
    }
  } while (pBlock->fileName[0] == 0);
}

inline static E_Return _CheckBlock(const S_USTARBlock* kpBlock)
{
  E_Return err;

  if (strncmp(kpBlock->magic, USTAR_MAGIC, 6) != 0)
  {
    err = ERR_INVALID_VALUE;
  }
  else
  {
    err = NO_ERROR;
  }

  return err;
}

inline static uint32_t _Oct2uint(const char* kpOct, size_t size)
{
  uint32_t out;
  uint32_t i;

  out = 0;
  i   = 0;
  while (i < size && kpOct[i])
  {
    out = (out << 3) | (uint32_t)(kpOct[i++] - '0');
  }

  return out;
}

static ssize_t _VFSSeek(void*                 pDriverData,
                        void*                 pHandle,
                        S_SeekIOCTLArguments* pArgs)
{
  S_USTARFileDescriptor* pFileDesc;
  ssize_t                retVal;

  (void)pDriverData;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pFileDesc = pHandle;

    if (pArgs->direction == SEEK_SET)
    {
      if (pArgs->offset <= pFileDesc->fileSize)
      {
        pFileDesc->offset = pArgs->offset;
      }
    }
    else if (pArgs->direction == SEEK_CUR)
    {
      if (pFileDesc->offset + pArgs->offset <= pFileDesc->fileSize)
      {
        pFileDesc->offset += pArgs->offset;
      }
    }
    else if (pArgs->direction == SEEK_END)
    {
      pFileDesc->offset = pFileDesc->fileSize;
    }

    retVal = pFileDesc->offset;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _VFSTell(void* pDriverData, void* pHandle, void* pArgs)
{
  S_USTARFileDescriptor* pDesc;
  ssize_t                retVal;

  (void)pArgs;
  (void)pDriverData;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pDesc  = pHandle;
    retVal = pDesc->offset;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

/***************************** DRIVER REGISTRATION ****************************/
VFS_REG_FS(sUSTARDriver);

/************************************ EOF *************************************/