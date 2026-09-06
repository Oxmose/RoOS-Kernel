/*******************************************************************************
 * @file RAMDiskTest.c
 *
 * @brief RAMDisk driver integration tests.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

#include <string.h>
#include <stdint.h>
#include <IOCTL.h>
#include <VirtualFS.h>
#include <TestFramework.h>

#define RAMDISK_TEST_DEVICE "/dev/storage/ramdisk0"

void RAMDiskTest(void)
{
  S_DirectoryEntry dirEntry;
  S_SeekIOCTLArguments seekArgs;
  uint8_t original[16];
  uint8_t pattern[16];
  uint8_t buffer[16];
  int32_t fd;
  ssize_t retVal;
  size_t index;

  for (index = 0; index < sizeof(pattern); ++index)
  {
    pattern[index] = (uint8_t)(0xA0 + index);
  }

  fd = VFSOpen(RAMDISK_TEST_DEVICE, O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_RAMDISK(0), fd >= 0, 0, fd,
                        TEST_RAMDISK_ENABLED);
  if (fd >= 0)
  {
    retVal = VFSRead(fd, NULL, sizeof(buffer));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(1), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, NULL, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(2), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSReaddir(fd, &dirEntry);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(3), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_DEV_GET_SECTOR_SIZE, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(4), retVal == 512, 512, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, pattern, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(5), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(6), VFSClose(fd) == 0, 1, 0,
                          TEST_RAMDISK_ENABLED);
  }

  fd = VFSOpen(RAMDISK_TEST_DEVICE, O_RDWR, 0);
  TEST_POINT_ASSERT_INT(TEST_RAMDISK(7), fd >= 0, 0, fd,
                        TEST_RAMDISK_ENABLED);
  if (fd >= 0)
  {
    retVal = VFSRead(fd, original, sizeof(original));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(8), retVal == sizeof(original),
                          sizeof(original), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, buffer, 0);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(9), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, NULL, sizeof(buffer));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(10), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, NULL, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(11), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, buffer, 0);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(12), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSReaddir(fd, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(13), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(14), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, pattern, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(15), retVal == sizeof(pattern),
                          sizeof(pattern), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_TELL, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(16), retVal == sizeof(pattern),
                          sizeof(pattern), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_DEV_SET_LBA, &(uint64_t){1});
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(17), retVal == 512, 512, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &(S_SeekIOCTLArguments){
      .direction = SEEK_SET, .offset = 0});
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(18), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, original, sizeof(original));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(19), retVal == sizeof(original),
                          sizeof(original), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &(S_SeekIOCTLArguments){
      .direction = SEEK_END, .offset = 0});
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(20), retVal >= 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, buffer, sizeof(buffer));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(21), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, pattern, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(22), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = (1ULL << 40);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_DWORD(TEST_RAMDISK(23), retVal == (1ULL << 40),
                          (1ULL << 40), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, buffer, sizeof(buffer));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(24), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, pattern, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(25), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    seekArgs.direction = SEEK_END;
    seekArgs.offset = 0;
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(26), retVal >= 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_TELL, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(27), retVal >= 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, buffer, sizeof(buffer));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(28), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = (ssize_t)retVal;
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(29), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, buffer, sizeof(buffer) * 4);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(30), retVal >= 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    seekArgs.direction = (E_SeekDirection)99;
    seekArgs.offset = 0;
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(31), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, 0xDEADBEEF, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(32), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(33), VFSClose(fd) == 0, 1, 0,
                          TEST_RAMDISK_ENABLED);
  }

  TEST_POINT_ASSERT_INT(TEST_RAMDISK(34),
                        VFSOpen(RAMDISK_TEST_DEVICE "/invalid", O_RDONLY, 0)
                        == -1, -1, 0, TEST_RAMDISK_ENABLED);
  TEST_FRAMEWORK_END();
}

#endif