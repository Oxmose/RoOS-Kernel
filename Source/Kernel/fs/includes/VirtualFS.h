/*******************************************************************************
 * @file VirtualFS.h
 *
 * @see VirtualFS.c
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

#ifndef __FS_VIRTUALFS_H_
#define __FS_VIRTUALFS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <CtrlBlock.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the invalid return error for the VFS */
#define VFS_DRIVER_INVALID ((void*)-1)
/** @brief Defines the maximal length of a file name */
#define VFS_FILENAME_MAX_LENGTH 256
/** @brief Defines the maximal length of a path */
#define VFS_PATH_MAX_LENGTH 4096
/** @brief Defines the maximal length of a filesystem name */
#define FS_NAME_LENGTH 32
/** @brief Defines the VFS path node delimiter */
#define VFS_PATH_DELIMITER '/'

/** @brief VFS Permission: Read */
#define VFS_PERMISSION_READ 4
/** @brief VFS Permission: Write */
#define VFS_PERMISSION_WRITE 2
/** @brief VFS Permission: Execute */
#define VFS_PERMISSION_EXEC 1
/** @brief Defines the VFS access permissions for read only */
#define O_RDONLY VFS_PERMISSION_READ
/** @brief Defines the VFS access permissions for read / write */
#define O_RDWR (VFS_PERMISSION_READ | VFS_PERMISSION_WRITE)


/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the file types supported by the VFS */
typedef enum
{
  /** @brief Type: file */
  VFS_FILE_TYPE_FILE = 0,
  /** @brief Type: directory  */
  VFS_FILE_TYPE_DIR  = 1
} E_VFSFileType;

/** @brief Defines the directory entry structure */
typedef struct
{
  /** @brief Directory entry name  */
  char pName[VFS_FILENAME_MAX_LENGTH + 1];
  /** @brief File entry name length */
  uint8_t filenameLength;
  /** @brief Directory entry type */
  E_VFSFileType type;
} S_DirectoryEntry;

/**
 * @brief Defines the function pointer for the open hook function.
 *
 * @details Defines the function pointer for the open hook function. This hook
 * will be called by the VFS once the driver is found based on the file path.
 * The part of the path that leads to the mouting point is stripped from the
 * path provided to this hook.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in] kpPath The path to the file to open. The part of the path that
 * leads to the mouting point is stripped from the path provided to this hook.
 * @param[in] flags The opening flags.
 * @param[in] mode The opening mode.
 *
 * @return The function shall return a handle to the opened file that will later
 * be passed to the driver for the other manipulation functions. This handle is
 * only used by the underlying driver and not modified by the VFS. On error the
 * function shall return -1 casted to void*.
 */
typedef void* (*T_VFSOpen)(void*       pDriverData,
                           const char* kpPath,
                           int32_t     flags,
                           int32_t     mode);

/**
 * @brief Defines the function pointer for the close hook function.
 *
 * @details Defines the function pointer for the close hook function. This hook
 * will be called by the VFS once the driver is found based on the file path.
 * The part of the path that leads to the mouting point is stripped from the
 * path provided to this hook.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileHandle The driver's private file handle returned by
 * open.
 *
 * @return The function shall return 0 when the close operation is successfull,
 * -1 otherwise.
 */
typedef int32_t (*T_VFSClose)(void* pDriverData, void* pFileHandle);

/**
 * @brief Defines the function pointer for the read hook function.
 *
 * @details Defines the function pointer for the read hook function. This hook
 * will be called by the VFS once the driver is found based on the internal file
 * descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileHandle The driver's private file handle returned by
 * open.
 * @param[out] pBuffer The buffer receiving the bytes read.
 * @param[in] count The maximal number of bytes to read.
 *
 * @return The function shall return the number of byte read into the buffer or
 * -1 on error.
 */
typedef ssize_t (*T_VFSRead)(void*  pDriverData,
                             void*  pFileHandle,
                             void*  pBuffer,
                             size_t count);

/**
 * @brief Defines the function pointer for the write hook function.
 *
 * @details Defines the function pointer for the write hook function. This hook
 * will be called by the VFS once the driver is found based on the internal file
 * descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileHandle The driver's private file handle returned by
 * open.
 * @param[in] kpBuffer The buffer containing the bytes to write.
 * @param[in] count The maximal number of bytes to write.
 *
 * @return The function shall return the number of byte written from the buffer
 * or -1 on error.
 */
typedef ssize_t (*T_VFSWrite)(void*       pDriverData,
                              void*       pFileHandle,
                              const void* kpBuffer,
                              size_t      count);

/**
 * @brief Defines the function pointer for the readdir hook function.
 *
 * @details Defines the function pointer for the readdir hook function. This
 * hook will be called by the VFS once the driver is found based on the internal
 * file descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileHandle The driver's private file handle returned by
 * open.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function shall return 0 on reaching the end of the directory,
 * 1 success or -1 on error.
 */
typedef int32_t (*T_VFSReadDir)(void*             pDriverData,
                                void*             pFileHandle,
                                S_DirectoryEntry* pDirEntry);

/**
 * @brief Defines the function pointer for the ioctl hook function.
 *
 * @details Defines the function pointer for the ioctl hook function. This
 * hook will be called by the VFS once the driver is found based on the internal
 * file descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileHandle The driver's private file handle returned by
 * open.
 * @param[in] operation The directory IOCTL operation idnetifier to execute.
 * @param[in, out] pArgs Optional IOCTL parameters.
 *
 * @return The function shall return whatever value required to be returned.
 */
typedef ssize_t (*T_VFSIOCTL)(void*    pDriverData,
                              void*    pFileHandle,
                              uint32_t operation,
                              void*    pArgs);

/** @brief Defines the VFS driver handle */
typedef void* T_VFSDriver;

/** @brief Defines a filesystem driver */
typedef struct
{
  /** @brief The driver's internal data handle. */
  void* pDriverData;

  /** @brief Stores the FS driver name */
  const char* pName;

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
  E_Return (*pMount)(const char* kpPath,
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
  E_Return (*pUnmount)(void* pDriverMountData);

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

  /** @brief Stores the VFS node handle. */
  void* pNode;
} S_FSDriver;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Registers a new filesystem driver.
 *
 * @details Registers a new driver in the kernel's filesystem driver table.
 *
 * @param[in] DRIVER The driver to add to the filesystem driver table.
 */
#define VFS_REG_FS(DRIVER)                                                 \
  S_FSDriver* DRVENT_##DRIVER __attribute__ ((section (".roos_fs_tbl"))) = \
    &DRIVER;

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
 * @brief Initializes the VFS driver.
 *
 * @details Initializes the VFS driver. Creates the entry mount point and
 * allocates the resources used by the VFS. On error, the initialization
 * generates a kernel panic.
 */
void VirtualFileSystemInit(void);

/**
 * @brief Initializes the file descriptor table for a given process.
 *
 * @details Initializes the file descriptor table for a given process. This
 * function will create the new table for the process and allocate the
 * resources.
 *
 * @param[out] pProcess The process for which the file descriptor table shall
 * be initialized.
 *
 * @return The function returns the success or error status.
 */
E_Return CreateProcessFDTable(S_KernelProcess* pProcess);

/**
 * @brief Destroys the file descriptor table for a given process.
 *
 * @details Destroys the file descriptor table for a given process. This
 * function will remove the new table for the process and release the
 * resources.
 *
 * @param[out] pProcess The process for which the file descriptor table shall
 * be detroyed.
 */
void DestroyProcessFDTable(S_KernelProcess* pProcess);

/**
 * @brief Registers a new driver in the VFS for the given path.
 *
 * @details Registers a new driver in the VFS for the given path.
 * When registering, the driver can provide a private handle to its internal
 * data through the parameter pDriverData.
 *
 * @param[in] kpPath The root path handled by the driver.
 * @param[in] pDriverData The handle to the private data used by the driver.
 * This handle will be passed as parameter in the hook functions.
 * @param[in] pOpen The open function hook pointer.
 * @param[in] pClose The close function hook pointer.
 * @param[in] pRead The read function hook pointer.
 * @param[in] pWrite The write function hook pointer.
 * @param[in] pReadDir The readdir function hook pointer.
 * @param[in] pIOCTL The ioctl function hook pointer.
 *
 * @return On success the VFS driver handle is returned, otherwise
 * VFS_DRIVER_INVALID is returned on error.
 */
T_VFSDriver RegisterVFSDriver(const char*  kpPath,
                              void*        pDriverData,
                              T_VFSOpen    pOpen,
                              T_VFSClose   pClose,
                              T_VFSRead    pRead,
                              T_VFSWrite   pWrite,
                              T_VFSReadDir pReadDir,
                              T_VFSIOCTL   pIOCTL);

/**
 * @brief Unregisters a registered VFS driver using its handle.
 *
 * @details Unregisters a registered VFS driver using its handle. The handle was
 * returned when registering the driver.
 *
 * @param[out] driver The VFS driver handle. When successfully
 * unregistered, the pointer handle is set to VFS_DRIVER_INVALID.
 *
 * @return The function returns the success or error state.
 */
E_Return UnregisterDriver(T_VFSDriver driver);

/**
 * @brief Opens and possibly create a file.
 *
 * @details Opens and possibly create a file. The function opens a file
 * specified by its path. If the file does not exist and the flags are set to
 * O_CREAT, the file will be created.
 *
 * @param[in] kpPath The path to the file to open.
 * @param[in] flags The opening flags.
 * @param[in] mode The opening mode.
 *
 * @return The function returns a file descriptor pointing to the opened file.
 * On error, the function returns -1.
 */
int32_t VFSOpen(const char* kpPath, int32_t flags, int32_t mode);

/**
 * @brief Closes an opened file.
 *
 * @details Closes an opened file. The function releases the resources
 * allocated to the file in the system.
 *
 * @param[in] fd The file descriptor of the file to close.
 *
 * @return The function returns 0 on success.
 * On error, the function returns -1.
 */
int32_t VFSClose(int32_t fd);

/**
 * @brief Reads bytes from a file.
 *
 * @details Reads bytes from a file. The function returns the number of bytes
 * read from the file.
 *
 * @param[in] fd The file descriptor of the file to read.
 * @param[out] pBuffer The buffer receiving the bytes to read.
 * @param[in] count The maximal number of bytes to read.
 *
 * @return The function returns the number of byte read into the buffer or
 * -1 on error.
 */
ssize_t VFSRead(int32_t fd, void* pBuffer, size_t count);

/**
 * @brief Writes bytes to a file.
 *
 * @details Writes bytes to a file. The function returns the number of bytes
 * written to the file.
 *
 * @param[in] fd The file descriptor of the file to write.
 * @param[out] kpBuffer The buffer containing the bytes to write.
 * @param[in] count The maximal number of bytes to write.
 *
 * @return The function returns the number of byte written into the file or
 * a negative value on error.
 */
ssize_t VFSWrite(int32_t fd, const void* kpBuffer, size_t count);

/**
 * @brief Reads a directory entry.
 *
 * @details Reads a directory entry. The function fills a pointer to a
 * dirent structure representing the next directory entry in the directory
 * stream pointed to by pDirEntry. It returns 0 on reaching the end of the
 * directory stream or -1 if an error occurred.
 *
 * @param[in] fd The file descriptor of the file to use.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on reaching the end of the
 * directory stream or -1 if an error occurred. Otherwise the function returns
 * 1.
 */
int32_t VFSReaddir(int32_t fd, S_DirectoryEntry* pDirEntry);

/**
 * @brief Performs an IOCTL command on a file.
 *
 * @details Performs an IOCTL command on a file. The function sends the IOCTL to
 * the underlying driver to be processed.
 *
 * @param[in] fd The file descriptor of the file to use.
 * @param[in] operation The IOCTL command to send.
 * @param[in, out] pArgs Optional IOCTL parameters.
 *
 * @return The function return whatever value required to be returned by the
 * IOCTL command.
 */
int32_t VFSIOCTL(int32_t fd, uint32_t operation, void* pArgs);

/**
 * @brief Mount function for the VFS.
 *
 * @details Mount function for the VFS. This function will link a
 * directory to the associated device and use the filesystem to access it.
 *
 * @param[in] kpPath The path of the directory to mount to.
 * @param[in] kpDevPath The path of the device to mount.
 * @param[in] kpFsName The filesystem name to use. If set to NULL, the VFS will
 * try to mount with each registered filesystem until one successfuly mounts the
 * device.
 *
 * @return The function returns the success or error status.
 */
E_Return VFSMount(const char* kpPath,
                  const char* kpDevPath,
                  const char* kpFsName);

/**
 * @brief Unmount function for the VFS.
 *
 * @details Unmount function for the VFS. This function will unlink a
 * directory to the associated device.
 *
 * @param[in] kpPath The path of the directory to unmount or the device to
 * unmount.
 *
 * @return The function returns the success or error status.
 */
E_Return VFSUnmount(const char* kpPath);

/**
 * @brief Gets the index before the next delimiter in the path.
 *
 * @details Gets the index before the next delimiter in the path.
 *
 * @param[in] kpStr The string to use.
 * @param[in] kStrSize The size of the string.
 *
 * @return The function returns the index before the next delimiter in the path.
 * -1 is returned when the function reached the end of the path.
 */
ssize_t VFSGetNextPathTokenPosition(const char* kpStr, const size_t kStrSize);

/*******************************************************************************
 * SYSCALL HANDLERS
 ******************************************************************************/


/**
 * @brief Handles the VFS open system call.
 *
 * @param[in] pParam0 User-space path.
 * @param[in] pParam1 Open flags.
 * @param[in] pParam2 Open mode.
 * @param[in] pParam3 Unused parameter.
 * @param[in] pParam4 Unused parameter.
 *
 * @return The opened file descriptor or -1 on error.
 */
int32_t SyscallVFSOpen(void* pParam0,
                       void* pParam1,
                       void* pParam2,
                       void* pParam3,
                       void* pParam4);

/**
 * @brief Handles the VFS close system call.
 *
 * @param[in] pParam0 File descriptor.
 * @param[in] pParam1 Unused parameter.
 * @param[in] pParam2 Unused parameter.
 * @param[in] pParam3 Unused parameter.
 * @param[in] pParam4 Unused parameter.
 *
 * @return 0 on success or -1 on error.
 */
int32_t SyscallVFSClose(void* pParam0,
                        void* pParam1,
                        void* pParam2,
                        void* pParam3,
                        void* pParam4);

/**
 * @brief Handles the VFS read system call.
 *
 * @param[in] pParam0 File descriptor.
 * @param[out] pParam1 User-space buffer.
 * @param[in] pParam2 Number of bytes to read.
 * @param[in] pParam3 Unused parameter.
 * @param[in] pParam4 Unused parameter.
 *
 * @return The number of bytes read or -1 on error.
 */
int32_t SyscallVFSRead(void* pParam0,
                       void* pParam1,
                       void* pParam2,
                       void* pParam3,
                       void* pParam4);

/**
 * @brief Handles the VFS directory read system call.
 *
 * @param[in] pParam0 File descriptor.
 * @param[out] pParam1 User-space directory entry.
 * @param[in] pParam2 Unused parameter.
 * @param[in] pParam3 Unused parameter.
 * @param[in] pParam4 Unused parameter.
 *
 * @return 1 on success, 0 at the end of the directory, or -1 on error.
 */
int32_t SyscallVFSReadDir(void* pParam0,
                          void* pParam1,
                          void* pParam2,
                          void* pParam3,
                          void* pParam4);

/**
 * @brief Handles the VFS write system call.
 *
 * @param[in] pParam0 File descriptor.
 * @param[in] pParam1 User-space buffer.
 * @param[in] pParam2 Number of bytes to write.
 * @param[in] pParam3 Unused parameter.
 * @param[in] pParam4 Unused parameter.
 *
 * @return The number of bytes written or -1 on error.
 */
int32_t SyscallVFSWrite(void* pParam0,
                        void* pParam1,
                        void* pParam2,
                        void* pParam3,
                        void* pParam4);

/**
 * @brief Handles the VFS IOCTL system call.
 *
 * @param[in] pParam0 File descriptor.
 * @param[in] pParam1 IOCTL operation.
 * @param[in, out] pParam2 User-space IOCTL arguments.
 * @param[in] pParam3 Unused parameter.
 * @param[in] pParam4 Unused parameter.
 *
 * @return The value returned by the IOCTL operation.
 */
int32_t SyscallVFSIOCTL(void* pParam0,
                        void* pParam1,
                        void* pParam2,
                        void* pParam3,
                        void* pParam4);


#endif /* #ifndef __FS_VIRTUALFS_H_ */

/************************************ EOF *************************************/
