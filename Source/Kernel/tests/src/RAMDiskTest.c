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
    retVal = VFSIOCTL(fd, VFS_IOCTL_DEV_GET_SECTOR_SIZE, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(2), retVal == 512, 512, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, pattern, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(3), retVal == -1, -1, retVal,
                          TEST_RAMDISK_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(4), VFSClose(fd) == 0, 1, 0,
                          TEST_RAMDISK_ENABLED);
  }

  fd = VFSOpen(RAMDISK_TEST_DEVICE, O_RDWR, 0);
  TEST_POINT_ASSERT_INT(TEST_RAMDISK(5), fd >= 0, 0, fd,
                        TEST_RAMDISK_ENABLED);
  if (fd >= 0)
  {
    retVal = VFSRead(fd, original, sizeof(original));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(6), retVal == sizeof(original),
                          sizeof(original), retVal, TEST_RAMDISK_ENABLED);
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(7), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, pattern, sizeof(pattern));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(8), retVal == sizeof(pattern),
                          sizeof(pattern), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_TELL, NULL);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(9), retVal == sizeof(pattern),
                          sizeof(pattern), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_DEV_SET_LBA, &(uint64_t){1});
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(10), retVal == 512, 512, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &(S_SeekIOCTLArguments){
      .direction = SEEK_SET, .offset = 0});
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(11), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSWrite(fd, original, sizeof(original));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(12), retVal == sizeof(original),
                          sizeof(original), retVal, TEST_RAMDISK_ENABLED);
    retVal = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &(S_SeekIOCTLArguments){
      .direction = SEEK_END, .offset = 0});
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(13), retVal >= 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    retVal = VFSRead(fd, buffer, sizeof(buffer));
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(14), retVal == 0, 0, retVal,
                          TEST_RAMDISK_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_RAMDISK(15), VFSClose(fd) == 0, 1, 0,
                          TEST_RAMDISK_ENABLED);
  }

  TEST_POINT_ASSERT_INT(TEST_RAMDISK(16),
                        VFSOpen(RAMDISK_TEST_DEVICE "/invalid", O_RDONLY, 0)
                        == -1, -1, 0, TEST_RAMDISK_ENABLED);
  TEST_FRAMEWORK_END();
}

#endif