/*******************************************************************************
 * @file DiskManager.h
 *
 * @see DiskManager.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 11/10/2026
 *
 * @version 2.0
 *
 * @brief Disk manager module.
 *
 * @details Disk manager module for roOs. The disk manager detects the presence
 * of medias in /dev/storage and tries to detect viable partitions. The disk
 * manager can be added partition table drivers to detect more systems.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __FS_DISKMANAGER_H_
#define __FS_DISKMANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief Defines the read operation for a disk device.
 *
 * @details Defines the read operation for a disk device. This function will be
 * called by the disk manager when a read operation is requested on the device.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[out] pBuffer The buffer receiving the bytes read.
 * @param[in] kSectorStart The starting sector to read from.
 * @param[in] kSectorCount The number of sectors to read.
 *
 * @return The function returns the number of sectors read or a negative value
 * on error.
 */
typedef ssize_t (*T_DiskReadOperation)(void*          pDriverData,
                                       void*          pBuffer,
                                       const uint64_t kSectorStart,
                                       const size_t   kSectorCount);
/**
 * @brief Defines the write operation for a disk device.
 *
 * @details Defines the write operation for a disk device. This function will be
 * called by the disk manager when a write operation is requested on the device.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in] kpBuffer The buffer containing the bytes to write.
 * @param[in] kSectorStart The starting sector to write to.
 * @param[in] kSectorCount The number of sectors to write.
 *
 * @return The function returns the number of sectors written or a negative
 * value on error.
 */
typedef ssize_t (*T_DiskWriteOperation)(void*          pDriverData,
                                        const void*    pBuffer,
                                        const uint64_t kSectorStart,
                                        const size_t   kSectorCount);

/**
 * @brief Defines the IOCTL operation for a disk device.
 *
 * @details Defines the IOCTL operation for a disk device. This function will be
 * called by the disk manager when an IOCTL operation is requested on the
 * device.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in] kCommand The command to execute.
 * @param[in, out] pArg The argument for the command.
 *
 * @return The function returns a non-negative value on success or a negative
 * value on error.
 */
typedef ssize_t (*T_DiskIOCTLOperation)(void*          pDriverData,
                                        const uint32_t kCommand,
                                        void*          pArg);
/**
 * @brief Defines the get sector size operation for a disk device.
 *
 * @details Defines the get sector size operation for a disk device. This
 * function will be called by the disk manager when a get sector size operation
 * is requested on the device.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 *
 * @return The function returns the sector size in bytes or a negative value on
 * error.
 */
typedef ssize_t (*T_DiskSectorSizeOperation)(void* pDriverData);

/**
 * @brief Defines the get sector count operation for a disk device.
 *
 * @details Defines the get sector count operation for a disk device. This
 * function will be called by the disk manager when a get sector count operation
 * is requested on the device.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 *
 * @return The function returns the number of sectors or a negative value on
 * error.
 */
typedef ssize_t (*T_DiskSectorCountOperation)(void* pDriverData);

/** @brief The type of a disk device. */
typedef enum
{
  /** @brief Disk type is a hard disk drive. */
  DISK_TYPE_IDE = 0,
  /** @brief Disk type is a solid state drive. */
  DISK_TYPE_AHCI,
  /** @brief Disk type is a floppy disk drive. */
  DISK_TYPE_FLOPPY,
  /** @brief Disk type is a NVME drive. */
  DISK_TYPE_NVME,
  /** @brief Disk type is a tape drive. */
  DISK_TYPE_TAPE,
  /** @brief Disk type is a removable media. */
  DISK_TYPE_REMOVABLE_MEDIA,
  /** @brief The count of disk types. */
  DISK_TYPE_COUNT
} E_DiskType;

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
 * @brief Initializes the disk manager.
 *
 * @details Initializes the disk manager. Detect and create disk partitions
 * in the system. On error, the initialization generates a kernel panic.
 */
void DiskManagerInit(void);

/**
 * @brief Adds a new disk device to the system.
 *
 * @details Adds a new disk device to the system. This function is used to
 * register a new disk device with the disk manager.
 *
 * @param[in] kDeviceType The type of the device to add.
 * @param[in] pReadOperation The read operation for the device.
 * @param[in] pWriteOperation The write operation for the device.
 * @param[in] pIOCTLOperation The I/O control operation for the device.
 * @param[in] pDriverData The driver data for the device.
 * @param[out] pDiskDriver Pointer to the VFS driver handle for the device. This
 * will be used to un-register the device with the VFS.
 *
 * @return E_Return The result of the operation.
 */
E_Return DiskManagerAddDevice(const E_DiskType           kDeviceType,
                              T_DiskReadOperation        pReadOperation,
                              T_DiskWriteOperation       pWriteOperation,
                              T_DiskIOCTLOperation       pIOCTLOperation,
                              T_DiskSectorSizeOperation  pSectorSizeOperation,
                              T_DiskSectorCountOperation pSectorCountOperation,
                              void*                      pDriverData,
                              T_VFSDriver*               pDiskDriver);

/**
 * @brief Removes a disk device from the system.
 *
 * @details Removes a disk device from the system. This function is used to
 * unregister a disk device from the disk manager. The device will be removed
 * from the list of devices and its resources will be freed. The device will
 * also be unregistered from the VFS.
 *
 * @param[in] diskDriver The VFS driver handle for the device to remove.
 *
 * @return E_Return The result of the operation.
 */
E_Return DiskManagerRemoveDevice(T_VFSDriver diskDriver);

#endif /* #ifndef __FS_DISKMANAGER_H_ */

/************************************ EOF *************************************/
