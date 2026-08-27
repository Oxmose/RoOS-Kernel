/*******************************************************************************
 * @file RAMDisk.c
 *
 * @see RAMDisk.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/07/2024
 *
 * @version 2.0
 *
 * @brief Kernel's ram disk driver.
 *
 * @details Kernel's ram disk driver. Defines the functions and
 * structures used by the kernel to manage manage the ram disk.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <Panic.h>
#include <IOCTL.h>
#include <Memory.h>
#include <stdint.h>
#include <stddef.h>
#include <VirtualFS.h>
#include <KernelHeap.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <KernelMutex.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header TODO */
#include <TestFramework.h>

/* Header file */
#include <RAMDisk.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "RAMDISK"

/** @brief FDT property for regs  */
#define RAMDISK_FDT_REG_PROP "reg"
/** @brief FDT property for device */
#define RAMDISK_FDT_DEVICE_PROP "device"

/** @brief The size in bytes of a RamDisk block */
#define RAMDISK_BLOCK_SIZE 512

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the RamDisk controller structure */
typedef struct
{
  /** @brief Start address in virtual memory */
  void* startVirtAddr;
  /** @brief Size of the ramdisk in bytes */
  size_t size;
  /** @brief The VFS driver associated to the RamDisk */
  T_VFSDriver vfsDriver;
  /** @brief The RamDisk driver lock */
  S_KernelMutex lock;
} S_RAMDiskController;

/**
 * @brief RamDisk file descriptor used to keep track of where to access the
 * ram disk.
 */
typedef struct
{
  /** @brief Access permissions */
  bool isReadOnly;
  /** @brief Current read offset */
  size_t offset;
} S_RAMDiskFileDescriptor;

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
#define RAMDISK_ASSERT(COND, MSG, ERROR) {              \
  if ((COND) == false)                                   \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);       \
  }                                                     \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the RamDisk driver to the system.
 *
 * @details Attaches the RamDisk driver to the system. This function will
 * use the FDT to initialize the RamDisk and retreive the RamDisk parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static E_Return _Attach(const S_FDTNode* pkFdtNode);

/**
 * @brief RamDisk VFS open hook.
 *
 * @details RamDisk VFS open hook. This function returns a handle to control the
 * RamDisk driver through VFS.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] kpPath The path in the RamDisk driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode);

/**
 * @brief RamDisk VFS close hook.
 *
 * @details RamDisk VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief RamDisk VFS read hook.
 *
 * @details RamDisk VFS read hook. This function read a string from the RamDisk
 * volume.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _VFSRead(void*  pDrvCtrl,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count);

/**
 * @brief RamDisk VFS write hook.
 *
 * @details RamDisk VFS write hook. This function writes a string to the RamDisk
 * volume.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
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
 * @brief RamDisk VFS IOCTL hook.
 *
 * @details RamDisk VFS IOCTL hook. This function performs the IOCTL for the
 * RamDisk driver.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
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
 * @brief RamDisk VFS seek hook.
 *
 * @details RamDisk VFS seek hook. This function performs a seek for the
 * RamDisk driver.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
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
 * @brief RamDisk VFS tell hook.
 *
 * @details RamDisk VFS tell hook. This function performs a tell for the
 * RamDisk driver.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the tell operation.
 *
 * @return The function returns the offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _VFSTell(void* pDriverData, void* pHandle, void* pArgs);

/**
 * @brief Sets the simulated LBA to the file descriptor.
 *
 * @details Sets the simulated LBA to the file descriptor. The cursor is
 * positionned based on the simulated sector size.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] lba The LBA value to set
 */
static ssize_t _SetLBA(S_RAMDiskController*     pCtrl,
                       S_RAMDiskFileDescriptor* pDesc,
                       uint64_t                 lba);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief RamDisk driver instance. */
static S_Driver sRAMDISKDriver =
{
  .pName         = "RamDisk Driver",
  .pDescription  = "RamDisk Driver roOs.",
  .pCompatible   = "roOs,roOs-ramdisk",
  .pVersion      = "2.0",
  .pDriverAttach = _Attach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static E_Return _Attach(const S_FDTNode* pkFdtNode)
{
  const uintptr_t*     kpPtrProp;
  const char*          kpStrProp;
  size_t               propLen;
  S_RAMDiskController* pCtrl;
  E_Return             retCode;
  E_Return             error;

  /* Create the driver controller structure */
  pCtrl = KMalloc(sizeof(S_RAMDiskController), KMALLOC_FREE_POOL);
  memset(pCtrl, 0, sizeof(S_RAMDiskController));

  retCode = KernelMutexInit(&pCtrl->lock,
                            KMUTEX_FLAG_QUEUING_PRIO |
                            KMUTEX_FLAG_PRIO_ELEVATION);
  if (retCode == NO_ERROR)
  {
    /* Get the registers, giving the base physical address and size */
    kpPtrProp = FDTGetProp(pkFdtNode, RAMDISK_FDT_REG_PROP, &propLen);
    if (kpPtrProp != NULL && propLen == sizeof(uintptr_t) * 2)
    {
#ifdef ARCH_64_BITS
      pCtrl->size = FDTTOCPU64(*(kpPtrProp + 1));
      pCtrl->startVirtAddr = (void*)FDTTOCPU64(*kpPtrProp);
#else
      pCtrl->size = FDTTOCPU32(*(kpPtrProp + 1));
      pCtrl->startVirtAddr = (void*)FDTTOCPU32(*kpPtrProp);
#endif

      /* Map the ramdisk in memory */
      pCtrl->startVirtAddr = MemoryKernelMap(pCtrl->startVirtAddr,
                                              pCtrl->size,
                                              MEMMGR_MAP_KERNEL |
                                              MEMMGR_MAP_RW     |
                                              MEMMGR_MAP_HARDWARE,
                                              &retCode);
      if (retCode == NO_ERROR)
      {
        /* Get the device path */
        kpStrProp = FDTGetProp(pkFdtNode, RAMDISK_FDT_DEVICE_PROP, &propLen);
        if (kpStrProp != NULL && propLen  != 0)
        {
          /* Register the driver */
          pCtrl->vfsDriver = RegisterVFSDriver(kpStrProp,
                                               pCtrl,
                                               _VFSOpen,
                                               _VFSClose,
                                               _VFSRead,
                                               _VFSWrite,
                                               NULL,
                                               _VFSIOCTL);
          if (pCtrl->vfsDriver == VFS_DRIVER_INVALID)
          {
            retCode = ERR_INVALID_VALUE;
            error = MemoryKernelUnmap(pCtrl->startVirtAddr, pCtrl->size);
            RAMDISK_ASSERT(error == NO_ERROR, "Failed to unmap RamDisk", error);
            error = KernelMutexDestroy(&pCtrl->lock);
            RAMDISK_ASSERT(error == NO_ERROR, "Failed to destroy mutex", error);
            KFree(pCtrl);
          }
        }
        else
        {
          retCode = ERR_INVALID_VALUE;
          error = MemoryKernelUnmap(pCtrl->startVirtAddr, pCtrl->size);
          RAMDISK_ASSERT(error == NO_ERROR, "Failed to unmap RamDisk", error);
          error = KernelMutexDestroy(&pCtrl->lock);
          RAMDISK_ASSERT(error == NO_ERROR, "Failed to destroy mutex", error);
          KFree(pCtrl);
        }
      }
      else
      {
        error = KernelMutexDestroy(&pCtrl->lock);
        RAMDISK_ASSERT(error == NO_ERROR, "Failed to destroy mutex", error);
        KFree(pCtrl);
      }
    }
    else
    {
      retCode = ERR_INVALID_VALUE;
      error = KernelMutexDestroy(&pCtrl->lock);
      RAMDISK_ASSERT(error == NO_ERROR, "Failed to destroy mutex", error);
      KFree(pCtrl);
    }
  }
  else
  {
    KFree(pCtrl);
  }

  return retCode;
}

static void* _VFSOpen(void*       pDrvCtrl,
                      const char* kpPath,
                      int         flags,
                      int         mode)
{
  S_RAMDiskFileDescriptor* pDesc;

  (void)mode;

  if (pDrvCtrl != NULL &&
     ((*kpPath == VFS_PATH_DELIMITER && *(kpPath + 1) == 0) || *kpPath == 0))
  {
    pDesc = KMallocUser(sizeof(S_RAMDiskFileDescriptor), NULL);
    if (pDesc != NULL)
    {
      memset(pDesc, 0, sizeof(S_RAMDiskFileDescriptor));
      pDesc->isReadOnly = (flags & O_RDWR) != O_RDWR;
    }
    else
    {
      pDesc = (void*)-1;
    }
  }
  else
  {
    pDesc = (void*)-1;
  }

  return pDesc;
}

static int32_t _VFSClose(void* pDrvCtrl, void* pHandle)
{
  int32_t retVal;

  (void)pDrvCtrl;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    KFreeUser(pHandle, NULL);
    retVal = 0;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _VFSRead(void*  pDrvCtrl,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count)
{
  ssize_t                  maxRead;
  S_RAMDiskFileDescriptor* pDesc;
  S_RAMDiskController*     pCtrl;
  E_Return                 error;

  if (pHandle != NULL && pHandle != (void*)-1 && pBuffer != NULL)
  {
    pCtrl = pDrvCtrl;
    pDesc = pHandle;

    error = KernelMutexLock(&pCtrl->lock);
    if (error == NO_ERROR)
    {
      if (pDesc->offset <= pCtrl->size)
      {
        maxRead = MIN(count, pCtrl->size - pDesc->offset);
        memcpy(pBuffer, (char*)pCtrl->startVirtAddr + pDesc->offset, maxRead);
      }
      else
      {
        maxRead = 0;
      }
      pDesc->offset += maxRead;

      error = KernelMutexUnlock(&pCtrl->lock);
      RAMDISK_ASSERT(error == NO_ERROR, "Failed to unlock mutex", error);
    }
    else
    {
      maxRead = -1;
    }
  }
  else
  {
    maxRead = -1;
  }

  return maxRead;
}

static ssize_t _VFSWrite(void*       pDrvCtrl,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count)
{
  ssize_t                  maxWrite;
  S_RAMDiskFileDescriptor* pDesc;
  S_RAMDiskController*     pCtrl;
  E_Return                 error;

  if (pHandle != NULL && pHandle != (void*)-1 && kpBuffer != NULL)
  {
    pCtrl = pDrvCtrl;
    pDesc = pHandle;

    error = KernelMutexLock(&pCtrl->lock);
    if (error == NO_ERROR)
    {
      if (pDesc->offset <= pCtrl->size)
      {
        maxWrite = MIN(count, pCtrl->size - pDesc->offset);
        memcpy((char*)pCtrl->startVirtAddr + pDesc->offset, kpBuffer, maxWrite);
      }
      else
      {
        maxWrite = 0;
      }
      pDesc->offset += maxWrite;

      error = KernelMutexUnlock(&pCtrl->lock);
      RAMDISK_ASSERT(error == NO_ERROR, "Failed to unlock mutex", error);
    }
    else
    {
      maxWrite = -1;
    }
  }
  else
  {
    maxWrite = -1;
  }

  return maxWrite;
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
    case VFS_IOCTL_DEV_GET_SECTOR_SIZE:
      retVal = RAMDISK_BLOCK_SIZE;
      break;
    case VFS_IOCTL_DEV_SET_LBA:
      retVal = _SetLBA(pDriverData, pHandle, *(uint64_t*)pArgs);
      break;
    case VFS_IOCTL_FILE_TELL:
      retVal = _VFSTell(pDriverData, pHandle, pArgs);
      break;
    default:
      retVal = -1;
  }

  return retVal;
}

static ssize_t _VFSSeek(void*                 pDriverData,
                        void*                 pHandle,
                        S_SeekIOCTLArguments* pArgs)
{
  S_RAMDiskFileDescriptor* pDesc;
  S_RAMDiskController*     pCtrl;
  ssize_t                  retVal;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pDesc = pHandle;
    pCtrl = pDriverData;

    if (pArgs->direction == SEEK_SET)
    {
      pDesc->offset = pArgs->offset;
      retVal = pDesc->offset;
    }
    else if (pArgs->direction == SEEK_CUR)
    {
      pDesc->offset += pArgs->offset;
      retVal = pDesc->offset;
    }
    else if (pArgs->direction == SEEK_END)
    {
      pDesc->offset = pCtrl->size + pArgs->offset;
      retVal = pDesc->offset;
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

static ssize_t _VFSTell(void* pDriverData, void* pHandle, void* pArgs)
{
  S_RAMDiskFileDescriptor* pDesc;
  ssize_t                  retVal;

  (void)pArgs;
  (void)pDriverData;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pDesc = pHandle;
    retVal = pDesc->offset;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _SetLBA(S_RAMDiskController*     pCtrl,
                       S_RAMDiskFileDescriptor* pDesc,
                       uint64_t                 lba)
{
  ssize_t retVal;

  (void)pCtrl;

  if (pDesc != NULL && pDesc != (void*)-1)
  {
    pDesc->offset = RAMDISK_BLOCK_SIZE * lba;
    retVal = pDesc->offset;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sRAMDISKDriver);

/************************************ EOF *************************************/