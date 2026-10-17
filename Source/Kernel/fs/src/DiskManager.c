/*******************************************************************************
 * @file DiskManager.c
 *
 * @see DiskManager.h
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
 * of medias in /dev/storage and tries to detect viable disks. The disk
 * manager can be added disk table drivers to detect more systems.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <Memory.h>
#include <Critical.h>
#include <VirtualFS.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelMutex.h>
#include <KernelWorker.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <DiskManager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "DISK MGR"

/** @brief Maximum length of a disk path */
#define DISK_MGR_MAX_PATH_LENGTH 64

/** @brief Root path for the disk manager */
#define DISK_MGR_ROOT_PATH "/dev/storage/"

/** @brief Defines the GPT header signature */
#define GPT_SIGNATURE 0x5452415020494645ULL
/** @brief Maximum accepted length for a partition name */
#define GPT_MAX_PART_NAME_LEN 72
/** @brief GPT partition attribute: bootable */
#define GPT_ATTRIBUTE_BOOTABLE 0x4
/** @brief GPT start CRC value */
#define GPT_CRC_START_VAL 0xFFFFFFFF

/** @brief Defines the MBR signature. */
#define MBR_SIGNATURE 0xAA55
/** @brief Maximal number of partitions in an MBR table */
#define MBR_PARTITION_COUNT 4

/** @brief Size of the transfer buffer in bytes (64KB). */
#define TRANSFER_BUFFER_SIZE 0x10000

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Structure representing a disk device. */
typedef struct S_DiskDevice
{
  /** @brief The type of the disk device. */
  E_DiskType type;
  /** @brief The name of the disk device. */
  char name[DISK_MGR_MAX_PATH_LENGTH];
  /** @brief The read operation for the disk device. */
  T_DiskReadOperation pReadOperation;
  /** @brief The write operation for the disk device. */
  T_DiskWriteOperation pWriteOperation;
  /** @brief The I/O control operation for the disk device. */
  T_DiskIOCTLOperation pIOCTLOperation;
  /** @brief The start sector offset */
  uint64_t startSector;
  /** @brief The number of sectors in the disk device. */
  uint64_t numberOfSectors;
  /** @brief The size of each sector in bytes. */
  uint64_t sectorSize;
  /** @brief The driver data for the disk device. */
  void* pDriverData;
  /** @brief The VFS driver for the disk device. */
  T_VFSDriver pDiskDriver;
  /** @brief The number of sub-devices for the disk device. */
  size_t subDeviceCount;
  /** @brief Pointer to the sub-devices of the disk device. */
  struct S_DiskDevice* pSubDevices;
  /** @brief Pointer to the next disk device in the list. */
  struct S_DiskDevice* pNext;
} S_DiskDevice;

/** @brief Structure representing a disk device descriptor. */
typedef struct
{
  /** @brief The disk device. */
  S_DiskDevice* pDevice;
  /** @brief The starting position of the disk in sectors. */
  uint64_t startSector;
  /** @brief The ending position of the disk in sectors. */
  uint64_t endSector;
  /** @brief The size of each sector in bytes */
  uint64_t sectorSize;
  /** @brief The offset in bytes on the disk */
  uint64_t offset;
  /** @brief The flags for the disk device descriptor. */
  uint32_t flags;
} S_DiskDeviceDescriptor;

/** @brief Defines the MBR partition structure. */
typedef struct
{
  /** @brief Drive attributes */
  uint8_t active;
  /** @brief Head Address of partition start */
  uint8_t startHead;
  /** @brief Cylinder Address of partition start */
  uint16_t startSectorCylinder;
  /** @brief Partition Type */
  uint8_t sysId;
  /** @brief Head Address of partition end */
  uint8_t endHead;
  /** @brief Cylinder Address of partition end */
  uint16_t endSectorCylinder;
  /** @brief LBA of partition start */
  uint32_t lbaStart;
  /** @brief Number of sectors in the partition */
  uint32_t size;
} __attribute__((__packed__)) S_PartitionTableEntry;

/** @brief Defines the MBR sector structure. */
typedef struct
{
  /** @brief Boostrap code and data */
  uint8_t bootstrapCode[440];
  /** @brief Unique disk ID */
  uint32_t diskId;
  /** @brief Reserved */
  uint16_t reserved;
  /** @brief Partition Table Entries */
  S_PartitionTableEntry partitions[MBR_PARTITION_COUNT];
  /** @brief Valid bootsector signature */
  uint16_t signature;
} __attribute__((__packed__)) S_MBRData;

/** @brief Defines the GPT Partition Table Header structure. */
typedef struct
{
  /** @brief GPT Signature */
  uint64_t signature;
  /** @brief GPT 	Revision number of header */
  uint32_t revision;
  /** @brief GPT Header size in little endian */
  uint32_t headerSize;
  /** @brief GPT CRC32 of header (offset +0 to +0x5b) in little endian, with
   * this field zeroed during calculation */
  uint32_t crc32Header;
  /** @brief Reserved; must be zero */
  uint32_t reserved0;
  /** @brief GPT Current LBA */
  uint64_t currentLBA;
  /** @brief GPT Backup LBA */
  uint64_t alternateHeaderLBA;
  /** @brief GPT 	First usable LBA  */
  uint64_t firstUsableBlock;
  /** @brief GPT Last usable LBA  */
  uint64_t lastUsableBlock;
  /** @brief GPT Disk GUID in mixed endian */
  uint8_t guid[16];
  /** @brief GPT Starting LBA of array of partition entries */
  uint64_t partitionArrayLBA;
  /** @brief GPT Number of partition entries in array */
  uint32_t partitionCount;
  /** @brief GPT Size of a single partition entry */
  uint32_t partitionArrayEntrySize;
  /** @brief GPT CRC32 of partition entries array in little endian */
  uint32_t crc32Array;
  /** @brief Reserved; must be zero */
  uint8_t reserved1[420];
} __attribute__((__packed__)) S_GPTTableHeader;

/** @brief Defines the GPT Partition Table Entry structure. */
typedef struct
{
  /** @brief GPT Partition Type GUID in mixed endian */
  uint8_t typeGUID[16];
  /** @brief GPT Partition Unique GUID in mixed endian */
  uint8_t uniqueGUID[16];
  /** @brief Partition's starting LBA. */
  uint64_t startingLBA;
  /** @brief Partition's ending LBA. */
  uint64_t endingLBA;
  /** @brief Partition's attributes */
  uint64_t attributes;
  /** @brief Partition's name */
  uint8_t name[GPT_MAX_PART_NAME_LEN];
  /** @brief Partition's reserved space */
  uint8_t reserved[];
} __attribute__((__packed__)) S_GPTPartitionEntry;



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
#define DISKMGR_ASSERT(COND, MSG, ERROR) {              \
  if ((COND) == false)                                  \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);       \
  }                                                     \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Disk VFS open hook.
 *
 * @details Disk VFS open hook. This function returns a handle to control
 * the VFS disk nodes.
 *
 * @param[in, out] pDiskDevice The disk VFS driver.
 * @param[in] kpPath The path in the VFS mount point table.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _VFSOpen(void*       pDiskDevice,
                      const char* kpPath,
                      int         flags,
                      int         mode);

/**
 * @brief Disk VFS close hook.
 *
 * @details Disk VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDiskDevice The disk VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pDiskDevice, void* pHandle);

/**
 * @brief Disk VFS write hook.
 *
 * @details Disk VFS write hook.
 *
 * @param[in, out] pDiskDevice The disk VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _VFSWrite(void*       pDiskDevice,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count);

/**
 * @brief Disk VFS write hook.
 *
 * @details Disk VFS write hook.
 *
 * @param[in, out] pDiskDevice The disk VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _VFSRead(void*  pDiskDevice,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count);

/**
 * @brief Disk VFS ReadDir hook.
 *
 * @details Disk VFS ReadDir hook. This function performs the ReadDir for the
 * Disk driver.
 *
 * @param[in, out] pDiskDevice The disk VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSReadDir(void*             pDiskDevice,
                           void*             pHandle,
                           S_DirectoryEntry* pDirEntry);

/**
 * @brief Disk VFS IOCTL hook.
 *
 * @details Disk VFS IOCTL hook. This function performs the IOCTL for the
 * Disk driver.
 *
 * @param[in, out] pDiskDevice The disk VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _VFSIOCTL(void*    pDiskDevice,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs);

/**
 * @brief Detaches all partitions from a disk device.
 *
 * @details Detaches all partitions from a disk device and unregisters them
 * from the VFS.
 *
 * @param[in, out] pDevice The disk device to detach partitions from.
 */
static void _DetachPartitions(S_DiskDevice* pDevice);

/**
 * @brief Detects and attaches MBR partitions to a disk device.
 *
 * @details This function detects MBR partitions on the specified disk device
 * and attaches them.
 *
 * @param[in, out] pDevice The disk device to detect and attach MBR partitions
 * to.
 *
 * @return The function returns E_SUCCESS on success and E_FAILURE on error.
 */
static E_Return _DetectAndAttachMBRPartitions(S_DiskDevice* pDevice);

/**
 * @brief Detects and attaches GPT partitions to a disk device.
 *
 * @details This function detects GPT partitions on the specified disk device
 * and attaches them.
 *
 * @param[in, out] pDevice The disk device to detect and attach GPT partitions
 * to.
 *
 * @return The function returns E_SUCCESS on success and E_FAILURE on error.
 */
static E_Return _DetectAndAttachGPTPartitions(S_DiskDevice* pDevice);

/**
 * @brief Adds calculation to the initial CRC32 value.
 *
 * @details Adds calculation to the initial CRC32 value. To start a new CRC
 * calculation, use the ild crc value GPT_CRC_START_VAL.
 *
 * @param[in] kOldCrcValue The old CRC32 value to update.
 * @param[in] kpBuffer The buffer to calculate.
 * @param[in] kSize The size of the buffer to calculate.
 *
 * @return The function returns the newly calculated CRC32 value.
 */
static inline uint32_t _GPTCrcAdd(const uint32_t kOldCrcValue,
                                  const uint8_t* kpBuffer,
                                  const size_t   kSize);

/**
 * @brief Ends the calculation to the CRC32 value.
 *
 * @details Ends the calculation to the CRC32 value. The function returns the
 * actual CRC32 value after computation.
 *
 * @param[in] kOldCrcValue The old CRC32 value to update.
 *
 * @return The function returns the newly calculated final CRC32 value.
 */
static inline uint32_t _GPTCrcEnd(const uint32_t kOldCrcValue);

/**
 * @brief Detects and attaches partitions to a disk device.
 *
 * @details This function detects partitions on the specified disk device
 * and attaches them. It first attempts to detect MBR partitions, and if that
 * fails, it attempts to detect GPT partitions.
 *
 * @param[in, out] pArgs The disk device to detect and attach partitions to.
 */
static void _DetectAndAttachPartitions(void* pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Allocated indices lock */
S_KernelSpinlock sDisksAllocatedIndicesLock = KERNEL_SPINLOCK_INIT_VALUE;
/** @brief The indices of the allocated disk types. */
static uint32_t sDisksAllocatedIndices[DISK_TYPE_COUNT] = {0};
/** @brief The prefices of the allocated disk types. */
static const char* sDisksAllocatedPrefices[DISK_TYPE_COUNT] =
{
  "hd", //DISK_TYPE_IDE,
  "sd", //DISK_TYPE_AHCI,
  "fd", //DISK_TYPE_FLOPPY,
  "nvme", //DISK_TYPE_NVME,
  "tape", //DISK_TYPE_TAPE,
  "removable" //DISK_TYPE_REMOVABLE_MEDIA
};
/** @brief The list of all disk devices. */
S_DiskDevice* sDisksList = NULL;
/** @brief Disk manager lock */
S_KernelSpinlock sDisksManagerLock = KERNEL_SPINLOCK_INIT_VALUE;
/** @brief Stores the CRC32 table for the GPT checksum calculation */
static uint32_t skCrc32Table[256] =
{
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/


/************************************ EOF *************************************/
static void* _VFSOpen(void*       pDiskDevice,
                      const char* kpPath,
                      int         flags,
                      int         mode)
{
  S_DiskDevice*           pDevice;
  S_DiskDeviceDescriptor* pDesc;

  (void)mode;

  pDevice = pDiskDevice;

  if (pDiskDevice != NULL &&
     ((*kpPath == VFS_PATH_DELIMITER && *(kpPath + 1) == 0) || *kpPath == 0))
  {
    pDesc = KMallocUser(sizeof(S_DiskDeviceDescriptor), NULL);
    if (pDesc != NULL)
    {
      pDesc->flags       = flags;
      pDesc->pDevice     = pDevice;
      pDesc->startSector = pDevice->startSector;
      pDesc->sectorSize  = pDevice->sectorSize;
      pDesc->endSector   = pDevice->startSector + pDevice->numberOfSectors - 1;
      pDesc->offset      = 0;
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

static int32_t _VFSClose(void* pDiskDevice, void* pHandle)
{
  int32_t retVal;

  (void)pDiskDevice;

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

static ssize_t _VFSWrite(void*       pDiskDevice,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count)
{
  S_DiskDevice*           pDevice;
  S_DiskDeviceDescriptor* pDesc;
  ssize_t                 retVal;
  size_t                  sectorCount;
  E_Return                error;
  size_t                  firstByteOffset;
  size_t                  lastByteOffset;
  size_t                  firstSector;
  size_t                  totalSize;
  size_t                  currentSectorCount;
  size_t                  bytesToWrite;
  ssize_t                 writeSize;
  size_t                  mappingSize;
  void*                   pEndBuffer;
  void*                   pTransferBuffer;

  pDevice = pDiskDevice;
  pDesc = pHandle;

  if (pHandle != NULL && pHandle != (void*)-1 && (pDesc->flags & O_WRONLY) != 0)
  {
    /* Calculate the offset for the first sector, last sector and total size */
    firstSector     = pDesc->startSector + pDesc->offset / pDesc->sectorSize;
    firstByteOffset = pDesc->offset % pDesc->sectorSize;
    totalSize       = firstByteOffset + count;
    sectorCount     = totalSize / pDesc->sectorSize;
    lastByteOffset  = totalSize % pDesc->sectorSize;
    if (lastByteOffset != 0)
    {
      ++sectorCount;
    }
    else
    {
      lastByteOffset = 0;
    }

    if (firstSector + sectorCount - 1 <= pDesc->endSector)
    {
      /* Allocate the transfer buffer */
      mappingSize = MIN(sectorCount * pDesc->sectorSize, TRANSFER_BUFFER_SIZE);
      mappingSize = ALIGN_UP(mappingSize, KERNEL_PAGE_SIZE);
      pTransferBuffer = MemoryKernelAllocate(mappingSize,
                                             MEMMGR_MAP_KERNEL |
                                             MEMMGR_MAP_RW |
                                             MEMMGR_MAP_CACHE_DISABLED,
                                             &error);
      if (error == NO_ERROR)
      {
        writeSize = 0;

        while (sectorCount > 0)
        {
          currentSectorCount = MIN(sectorCount,
                                  TRANSFER_BUFFER_SIZE / pDesc->sectorSize);
          /* If the first sector is partial: read, modify, and write it back */
          if (firstByteOffset != 0)
          {
            retVal = pDevice->pReadOperation(pDevice->pDriverData,
                                              pTransferBuffer,
                                              firstSector,
                                              1);
            if (retVal == 1)
            {
              bytesToWrite = MIN(count, pDesc->sectorSize - firstByteOffset);
              memcpy((uint8_t*)pTransferBuffer + firstByteOffset,
                    kpBuffer,
                    bytesToWrite);
              CPUMemoryFenceRelease();
              retVal = pDevice->pWriteOperation(pDevice->pDriverData,
                                                pTransferBuffer,
                                                firstSector,
                                                1);
              if (retVal == 1)
              {
                writeSize = bytesToWrite;
                kpBuffer  = (const uint8_t*)kpBuffer + bytesToWrite;
                count     -= bytesToWrite;
                firstByteOffset = 0;
                --currentSectorCount;
                --sectorCount;
                ++firstSector;

                pDesc->offset += bytesToWrite;
              }
              else
              {
                break;
              }
            }
            else
            {
              break;
            }
          }

          if (currentSectorCount > 0)
          {
            retVal = 1;
            /* If the last sector is partial: read, modify, and write it back */
            if (lastByteOffset != 0 &&
                currentSectorCount <= TRANSFER_BUFFER_SIZE / pDesc->sectorSize)
            {
              pEndBuffer = (uint8_t*)pTransferBuffer +
                            (currentSectorCount - 1) *
                            pDesc->sectorSize;

              retVal = pDevice->pReadOperation(pDevice->pDriverData,
                                                pEndBuffer,
                                                firstSector +
                                                  currentSectorCount -
                                                  1,
                                                1);
            }
            if (retVal == 1)
            {
              bytesToWrite = currentSectorCount * pDesc->sectorSize;
              if (bytesToWrite > count)
              {
                bytesToWrite = count;
              }
              memcpy(pTransferBuffer, kpBuffer, bytesToWrite);
              CPUMemoryFenceRelease();
              retVal = pDevice->pWriteOperation(pDevice->pDriverData,
                                                pTransferBuffer,
                                                firstSector,
                                                currentSectorCount);
              if (retVal == (ssize_t)currentSectorCount)
              {
                writeSize   += bytesToWrite;
                kpBuffer    = (const uint8_t*)kpBuffer + bytesToWrite;
                count       -= bytesToWrite;
                sectorCount -= currentSectorCount;
                firstSector += currentSectorCount;

                pDesc->offset += bytesToWrite;
              }
              else
              {
                break;
              }
            }
            else
            {
              break;
            }
          }
        }
        retVal = writeSize;

        error = MemoryKernelFree(pTransferBuffer, mappingSize);
        DISKMGR_ASSERT(error == NO_ERROR,
                       "Failed to free transfer buffer",
                       error);
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

static ssize_t _VFSRead(void*  pDiskDevice,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count)
{
  S_DiskDevice*           pDevice;
  S_DiskDeviceDescriptor* pDesc;
  ssize_t                 retVal;
  size_t                  sectorCount;
  E_Return                error;
  size_t                  firstByteOffset;
  size_t                  firstSector;
  size_t                  totalSize;
  size_t                  currentSectorCount;
  size_t                  bytesToRead;
  ssize_t                 readSize;
  size_t                  mappingSize;
  void*                   pTransferBuffer;

  pDevice = pDiskDevice;
  pDesc = pHandle;

  if (pHandle != NULL && pHandle != (void*)-1 && (pDesc->flags & O_RDONLY) != 0)
  {
    /* Calculate the offset for the first sector, last sector and total size */
    firstSector     = pDesc->startSector + pDesc->offset / pDesc->sectorSize;
    firstByteOffset = pDesc->offset % pDesc->sectorSize;
    totalSize       = firstByteOffset + count;
    sectorCount     = totalSize / pDesc->sectorSize;
    if (totalSize % pDesc->sectorSize != 0)
    {
      ++sectorCount;
    }

    if (firstSector + sectorCount - 1 <= pDesc->endSector)
    {
      /* Allocate the transfer buffer */
      mappingSize = MIN(sectorCount * pDesc->sectorSize, TRANSFER_BUFFER_SIZE);
      mappingSize = ALIGN_UP(mappingSize, KERNEL_PAGE_SIZE);
      pTransferBuffer = MemoryKernelAllocate(mappingSize,
                                             MEMMGR_MAP_KERNEL |
                                             MEMMGR_MAP_RW |
                                             MEMMGR_MAP_CACHE_DISABLED,
                                             &error);

      if (error == NO_ERROR)
      {
        readSize = 0;

        while (sectorCount > 0)
        {
          currentSectorCount = MIN(sectorCount,
                                  TRANSFER_BUFFER_SIZE / pDesc->sectorSize);
          retVal = pDevice->pReadOperation(pDevice->pDriverData,
                                           pTransferBuffer,
                                           firstSector,
                                           currentSectorCount);
          if (retVal == (ssize_t)currentSectorCount)
          {
            bytesToRead = currentSectorCount * pDesc->sectorSize -
                          firstByteOffset;
            bytesToRead = MIN(bytesToRead, count);
            memcpy(pBuffer,
                  (uint8_t*)pTransferBuffer + firstByteOffset,
                  bytesToRead);

            readSize        += bytesToRead;
            pBuffer         = (uint8_t*)pBuffer + bytesToRead;
            count           -= bytesToRead;
            firstByteOffset = 0;
            sectorCount     -= currentSectorCount;
            firstSector     += currentSectorCount;

            pDesc->offset   += bytesToRead;
          }
          else
          {
            readSize = -1;
            break;
          }
        }
        retVal = readSize;

        error = MemoryKernelFree(pTransferBuffer, mappingSize);
        DISKMGR_ASSERT(error == NO_ERROR,
                       "Failed to free transfer buffer",
                       error);
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

static int32_t _VFSReadDir(void*             pDiskDevice,
                           void*             pHandle,
                           S_DirectoryEntry* pDirEntry)
{
  (void)pDiskDevice;
  (void)pHandle;
  (void)pDirEntry;

  return -1;
}

static ssize_t _VFSIOCTL(void*    pDiskDevice,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs)
{
  S_DiskDevice*           pDevice;
  ssize_t                 retVal;
  S_DiskDeviceDescriptor* pDesc;

  pDevice = pDiskDevice;
  pDesc = (S_DiskDeviceDescriptor*)pHandle;

  if (pDesc != NULL && pDesc != (void*)-1 && (pDesc->flags & O_RDWR) == O_RDWR)
  {
    retVal = pDevice->pIOCTLOperation(pDevice->pDriverData,
                                      operation,
                                      pArgs);
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static E_Return _DetectAndAttachMBRPartitions(S_DiskDevice* pDevice)
{
  E_Return      retVal;
  T_VFSDriver   diskDriver;
  S_MBRData*    pMBR;
  uint32_t      partId;
  ssize_t       secRead;
  S_DiskDevice* pPartitionDevice;
  size_t        sectorCount;
  size_t        mappingSize;
  void*         pTransferBuffer;

  /* Read the MBR */
  sectorCount = sizeof(S_MBRData) / pDevice->sectorSize;
  if (sizeof(S_MBRData) % pDevice->sectorSize != 0)
  {
    ++sectorCount;
  }
  mappingSize = ALIGN_UP(sectorCount * pDevice->sectorSize, KERNEL_PAGE_SIZE);
  pTransferBuffer = MemoryKernelAllocate(mappingSize,
                                         MEMMGR_MAP_KERNEL |
                                         MEMMGR_MAP_RW |
                                         MEMMGR_MAP_CACHE_DISABLED,
                                         &retVal);
  if (retVal == NO_ERROR)
  {
    secRead = pDevice->pReadOperation(pDevice->pDriverData,
                                      pTransferBuffer,
                                      0,
                                      sectorCount);

    if (secRead == (ssize_t)sectorCount)
    {
      pMBR = (S_MBRData*)pTransferBuffer;
      if (pMBR->signature == MBR_SIGNATURE)
      {
        retVal = NO_ERROR;
        for (partId = 0; partId < MBR_PARTITION_COUNT; ++partId)
        {
          /* Check if the entry is unused */
          if (pMBR->partitions[partId].sysId != 0 &&
              pMBR->partitions[partId].sysId != 0xEE)
          {
            /* Create a new partition device */
            pPartitionDevice = KMalloc(sizeof(S_DiskDevice), KMALLOC_FREE_POOL);
            /* Fill the partition device structure */
            pPartitionDevice->type = pDevice->type;
            snprintf(pPartitionDevice->name,
                    DISK_MGR_MAX_PATH_LENGTH,
                    "%s%d",
                    pDevice->name,
                    partId + 1);
            pPartitionDevice->pReadOperation = pDevice->pReadOperation;
            pPartitionDevice->pWriteOperation = pDevice->pWriteOperation;
            pPartitionDevice->pIOCTLOperation = pDevice->pIOCTLOperation;
            pPartitionDevice->pDriverData = pDevice->pDriverData;
            pPartitionDevice->subDeviceCount = 0;
            pPartitionDevice->pSubDevices = NULL;
            pPartitionDevice->startSector = pMBR->partitions[partId].lbaStart;
            pPartitionDevice->numberOfSectors = pMBR->partitions[partId].size;
            pPartitionDevice->sectorSize = pDevice->sectorSize;
            diskDriver = RegisterVFSDriver(pPartitionDevice->name,
                                            pPartitionDevice,
                                            _VFSOpen,
                                            _VFSClose,
                                            _VFSRead,
                                            _VFSWrite,
                                            _VFSReadDir,
                                            _VFSIOCTL);
            if (diskDriver != VFS_DRIVER_INVALID)
            {
              /* Add the partition device to the list of sub-devices */
              KERNEL_LOCK(sDisksManagerLock);
              pPartitionDevice->pNext = pDevice->pSubDevices;
              pDevice->pSubDevices = pPartitionDevice;
              ++(pDevice->subDeviceCount);
              KERNEL_UNLOCK(sDisksManagerLock);
            }
            else
            {
              KFree(pPartitionDevice);
              retVal = ERR_UNAUTHORIZED_ACTION;
              break;
            }
          }
        }
      }
    }

    retVal = MemoryKernelFree(pTransferBuffer, mappingSize);
    DISKMGR_ASSERT(retVal == NO_ERROR,
                    "Failed to free transfer buffer",
                    retVal);
  }
  else
  {
    retVal = ERR_IO_INVALID;
  }

  return retVal;
}

static E_Return _DetectAndAttachGPTPartitions(S_DiskDevice* pDevice)
{
  E_Return             retVal;
  E_Return             error;
  T_VFSDriver          diskDriver;
  S_GPTTableHeader     pGPTHeader;
  S_GPTPartitionEntry* pGPTEntry;
  uint32_t             crc32Val;
  uint32_t             savedCrc;
  uint32_t             partId;
  ssize_t              secRead;
  S_DiskDevice*        pPartitionDevice;
  uint32_t             partitionTableSize;
  uint8_t*             pHeaderBuffer;
  uint8_t*             pEntryBuffer;
  size_t               sectorCount;
  uint32_t             i;
  size_t               mappingSize[2];

  /* Read the GPT header */
  sectorCount = sizeof(S_GPTTableHeader) / pDevice->sectorSize;
  if (sizeof(S_GPTTableHeader) % pDevice->sectorSize != 0)
  {
    ++sectorCount;
  }
  mappingSize[0] = ALIGN_UP(sectorCount * pDevice->sectorSize, KERNEL_PAGE_SIZE);
  pHeaderBuffer = MemoryKernelAllocate(mappingSize[0],
                                       MEMMGR_MAP_KERNEL |
                                       MEMMGR_MAP_RW |
                                       MEMMGR_MAP_CACHE_DISABLED,
                                       &retVal);

  if (retVal == NO_ERROR)
  {
    secRead = pDevice->pReadOperation(pDevice->pDriverData,
                                      pHeaderBuffer,
                                      1,
                                      sectorCount);
    if (secRead == (ssize_t)sectorCount)
    {
      memcpy(&pGPTHeader, pHeaderBuffer, sizeof(S_GPTTableHeader));
      if (pGPTHeader.signature == GPT_SIGNATURE)
      {
        /* Compute the header CRC */
        savedCrc = pGPTHeader.crc32Header;
        pGPTHeader.crc32Header = 0;
        crc32Val = _GPTCrcAdd(GPT_CRC_START_VAL,
                              (uint8_t*)&pGPTHeader,
                              (uint8_t*)&pGPTHeader.reserved1 -
                              (uint8_t*)&pGPTHeader);
        crc32Val = _GPTCrcEnd(crc32Val);
        if(crc32Val == savedCrc &&
           pGPTHeader.partitionArrayEntrySize >= 128 &&
           pGPTHeader.partitionCount <= 1024)
        {
          KERNEL_DEBUG(DISK_MGR_DEBUG_ENABLED,
                      MODULE_NAME,
                      "GPT header CRC32 matched: %x vs %x",
                      savedCrc,
                      crc32Val);


          partitionTableSize = pGPTHeader.partitionCount *
                               pGPTHeader.partitionArrayEntrySize;

          sectorCount = partitionTableSize / pDevice->sectorSize;
          if (partitionTableSize % pDevice->sectorSize != 0)
          {
            ++sectorCount;
          }
          mappingSize[1] = ALIGN_UP(sectorCount * pDevice->sectorSize,
                                    KERNEL_PAGE_SIZE);
          pEntryBuffer = MemoryKernelAllocate(mappingSize[1],
                                               MEMMGR_MAP_KERNEL |
                                               MEMMGR_MAP_RW |
                                               MEMMGR_MAP_CACHE_DISABLED,
                                               &retVal);
          if (retVal == NO_ERROR)
          {
            secRead = pDevice->pReadOperation(pDevice->pDriverData,
                                              pEntryBuffer,
                                              pGPTHeader.partitionArrayLBA,
                                              sectorCount);

            if (secRead == (ssize_t)sectorCount)
            {
              for (partId = 0; partId < pGPTHeader.partitionCount; ++partId)
              {
                pGPTEntry = (S_GPTPartitionEntry*)(pEntryBuffer +
                                                   partId *
                                        pGPTHeader.partitionArrayEntrySize);
                if (pGPTEntry->startingLBA != 0 && pGPTEntry->endingLBA != 0)
                {
                  /* Found a valid partition */
                  for (i = 0; i < 16; ++i)
                  {
                    if (pGPTEntry->typeGUID[i] != 0)
                    {
                      break;
                    }
                  }
                  if (i != 16)
                  {
                    /* Create a new partition device */
                    pPartitionDevice = KMalloc(sizeof(S_DiskDevice),
                                                KMALLOC_FREE_POOL);
                    /* Fill the partition device structure */
                    pPartitionDevice->type = pDevice->type;
                    snprintf(pPartitionDevice->name,
                            DISK_MGR_MAX_PATH_LENGTH,
                            "%s%d",
                            pDevice->name,
                            partId + 1);
                    pPartitionDevice->pReadOperation = pDevice->pReadOperation;
                    pPartitionDevice->pWriteOperation = pDevice->pWriteOperation;
                    pPartitionDevice->pIOCTLOperation = pDevice->pIOCTLOperation;
                    pPartitionDevice->pDriverData = pDevice->pDriverData;
                    pPartitionDevice->subDeviceCount = 0;
                    pPartitionDevice->pSubDevices = NULL;
                    pPartitionDevice->startSector = pGPTEntry->startingLBA;
                    pPartitionDevice->numberOfSectors =
                      (pGPTEntry->endingLBA - pGPTEntry->startingLBA) + 1;
                    pPartitionDevice->sectorSize = pDevice->sectorSize;
                    diskDriver = RegisterVFSDriver(pPartitionDevice->name,
                                                    pPartitionDevice,
                                                    _VFSOpen,
                                                    _VFSClose,
                                                    _VFSRead,
                                                    _VFSWrite,
                                                    _VFSReadDir,
                                                    _VFSIOCTL);
                    if (diskDriver != VFS_DRIVER_INVALID)
                    {
                      /* Add the partition device to the list of sub-devices */
                      KERNEL_LOCK(sDisksManagerLock);
                      pPartitionDevice->pNext = pDevice->pSubDevices;
                      pDevice->pSubDevices = pPartitionDevice;
                      ++(pDevice->subDeviceCount);
                      KERNEL_UNLOCK(sDisksManagerLock);
                    }
                    else
                    {
                      KFree(pPartitionDevice);
                      retVal = ERR_UNAUTHORIZED_ACTION;
                      break;
                    }
                  }
                }
              }
            }
            else
            {
              retVal = ERR_IO_INVALID;
            }
            error = MemoryKernelFree(pEntryBuffer, mappingSize[1]);
            DISKMGR_ASSERT(error == NO_ERROR,
                           "Failed to free transfer buffer",
                          error);
          }
        }
        else
        {
          retVal = ERR_INVALID_VALUE;
        }
      }
    }
    else
    {
      retVal = ERR_IO_INVALID;
    }
    error = MemoryKernelFree(pHeaderBuffer, mappingSize[0]);
    DISKMGR_ASSERT(error == NO_ERROR,
                   "Failed to free transfer buffer",
                   error);
  }
  return retVal;
}

static inline uint32_t _GPTCrcAdd(uint32_t       oldCrcValue,
                                  const uint8_t* kpBuffer,
                                  const size_t   kSize)
{
  size_t i;
  for(i = 0; i < kSize; ++i)
  {
      oldCrcValue = skCrc32Table[(oldCrcValue ^ *(kpBuffer + i)) & 0xFF] ^
                    (oldCrcValue >> 8);
  }

  return oldCrcValue;
}

static inline uint32_t _GPTCrcEnd(const uint32_t kOldCrcValue)
{
  return kOldCrcValue ^ 0xFFFFFFFF;
}

static void _DetectAndAttachPartitions(void* pArgs)
{
  E_Return      retVal;
  S_DiskDevice* pDevice;

  pDevice = (S_DiskDevice*)pArgs;

  if (pDevice != NULL)
  {
    /* Detect and attach partitions */
    retVal = _DetectAndAttachMBRPartitions(pDevice);
    if (retVal == NO_ERROR)
    {
      _DetectAndAttachGPTPartitions(pDevice);
    }
  }
}

static void _DetachPartitions(S_DiskDevice* pDevice)
{
  S_DiskDevice* pSubDevice;
  S_DiskDevice* pNextSubDevice;
  E_Return      error;

  pSubDevice = pDevice->pSubDevices;
  while (pSubDevice != NULL)
  {
    pNextSubDevice = pSubDevice->pNext;

    /* Unregister the partition */
    error = UnregisterVFSDriver(pSubDevice->pDiskDriver);
    DISKMGR_ASSERT(error == NO_ERROR,
                   "Failed to unregister partition",
                   ERR_UNAUTHORIZED_ACTION);

    /* Free the partition structure */
    KFree(pSubDevice);

    pSubDevice = pNextSubDevice;
  }

  pDevice->pSubDevices = NULL;
  pDevice->subDeviceCount = 0;
}

void DiskManagerInit(void)
{
  /* Nothing to do */
}

E_Return DiskManagerAddDevice(const E_DiskType           kDeviceType,
                              T_DiskReadOperation        pReadOperation,
                              T_DiskWriteOperation       pWriteOperation,
                              T_DiskIOCTLOperation       pIOCTLOperation,
                              T_DiskSectorSizeOperation  pSectorSizeOperation,
                              T_DiskSectorCountOperation pSectorCountOperation,
                              void*                      pDriverData,
                              T_VFSDriver*               pDiskDriver)
{
  E_Return           retVal;
  uint32_t           index;
  uint32_t           prefixLength;
  uint32_t           pathLength;
  T_VFSDriver        diskDriver;
  S_DiskDevice*      pNewDevice;
  char               tmp;
  char*              startPrefix;
  size_t             sufixLength;

  /* Check parameters */
  if (kDeviceType < DISK_TYPE_COUNT &&
      pReadOperation != NULL &&
      pWriteOperation != NULL &&
      pIOCTLOperation != NULL &&
      pSectorSizeOperation != NULL &&
      pSectorCountOperation != NULL)
  {
    KERNEL_LOCK(sDisksAllocatedIndicesLock);

    /* Create the new device */
    pNewDevice = KMalloc(sizeof(S_DiskDevice), KMALLOC_FREE_POOL);

    index = sDisksAllocatedIndices[kDeviceType]++;

    /* Search for any partition in the device */
    KERNEL_UNLOCK(sDisksAllocatedIndicesLock);

    /* Construct the device path */
    pathLength = strnlen(DISK_MGR_ROOT_PATH, 32);
    prefixLength = strnlen(sDisksAllocatedPrefices[kDeviceType], 16);
    memcpy(pNewDevice->name, DISK_MGR_ROOT_PATH, DISK_MGR_MAX_PATH_LENGTH);
    memcpy(pNewDevice->name + pathLength,
           sDisksAllocatedPrefices[kDeviceType],
           prefixLength);
    pathLength += prefixLength;

    startPrefix = pNewDevice->name + pathLength;
    sufixLength = 0;
    while (index > 25 && pathLength < DISK_MGR_MAX_PATH_LENGTH - 13)
    {
      pNewDevice->name[pathLength] = 'a' + (index % 26);
      index /= 26;
      --index;
      ++pathLength;
      ++sufixLength;
    }
    pNewDevice->name[pathLength] = 'a' + (index % 26);
    ++pathLength;
    pNewDevice->name[pathLength] = 0;

    for (index = 0; index < sufixLength / 2; ++index)
    {
      tmp = *startPrefix;
      *startPrefix = pNewDevice->name[sufixLength - index - 1];
      pNewDevice->name[sufixLength - index - 1] = tmp;
      ++startPrefix;
    }


    /* Create a new device structure */
    pNewDevice->type                  = kDeviceType;
    pNewDevice->pReadOperation        = pReadOperation;
    pNewDevice->pWriteOperation       = pWriteOperation;
    pNewDevice->pIOCTLOperation       = pIOCTLOperation;
    pNewDevice->startSector           = 0;
    pNewDevice->numberOfSectors       = pSectorCountOperation(pDriverData);
    pNewDevice->sectorSize            = pSectorSizeOperation(pDriverData);
    pNewDevice->pDriverData           = pDriverData;
    pNewDevice->pSubDevices           = NULL;
    pNewDevice->subDeviceCount        = 0;

    /* Register the new device */
    diskDriver = RegisterVFSDriver(pNewDevice->name,
                                    pNewDevice,
                                    _VFSOpen,
                                    _VFSClose,
                                    _VFSRead,
                                    _VFSWrite,
                                    _VFSReadDir,
                                    _VFSIOCTL);
    if (diskDriver != VFS_DRIVER_INVALID)
    {
      /* Detect and attach partitions delayed */
      retVal = KWorkerSubmitTask(_DetectAndAttachPartitions,
                                  pNewDevice,
                                  NULL);
      if (retVal == NO_ERROR)
      {
        KERNEL_LOCK(sDisksManagerLock);
        *pDiskDriver = diskDriver;
        pNewDevice->pDiskDriver = diskDriver;
        pNewDevice->pNext = sDisksList;
        sDisksList = pNewDevice;
        KERNEL_UNLOCK(sDisksManagerLock);
      }
      else
      {
        KFree(pNewDevice);
      }
    }
    else
    {
      KFree(pNewDevice);
      retVal = ERR_UNAUTHORIZED_ACTION;
    }
  }
  else
  {
    retVal = ERR_INVALID_PARAMETER;
  }

  return retVal;
}

E_Return DiskManagerRemoveDevice(T_VFSDriver diskDriver)
{
  S_DiskDevice* pDevice;
  S_DiskDevice* pPrevDevice;
  E_Return      error;

  error = ERR_NOT_FOUND;
  KERNEL_LOCK(sDisksManagerLock);
  pDevice = sDisksList;
  while (pDevice != NULL)
  {
    if (pDevice->pDiskDriver == diskDriver)
    {
      /* Unregister the device */
      error = UnregisterVFSDriver(pDevice->pDiskDriver);

      if (error == NO_ERROR)
      {
        /* Unregister the partitions */
        _DetachPartitions(pDevice);

        /* Remove the device from the list */
        if (sDisksList == pDevice)
        {
          sDisksList = pDevice->pNext;
        }
        else
        {
          pPrevDevice = sDisksList;
          while (pPrevDevice != NULL && pPrevDevice->pNext != pDevice)
          {
            pPrevDevice = pPrevDevice->pNext;
          }
          if (pPrevDevice != NULL)
          {
            pPrevDevice->pNext = pDevice->pNext;
          }
        }

        /* Free the device structure */
        pDevice->pDiskDriver = VFS_DRIVER_INVALID;
        KFree(pDevice);
      }
      break;
    }
    pDevice = pDevice->pNext;
  }
  KERNEL_UNLOCK(sDisksManagerLock);

  return error;
}