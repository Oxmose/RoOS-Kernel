/*******************************************************************************
 * @file USTARFSTest.c
 *
 * @brief USTAR filesystem driver integration tests.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

#include <string.h>
#include <IOCTL.h>
#include <VirtualFS.h>
#include <TestFramework.h>

#define USTAR_TEST_DEVICE "/dev/storage/ramdisk0"
#define USTAR_TEST_MOUNT "/test_ustar"
#define USTAR_LARGE_FILE_SIZE 3080

static void _TestUSTARFile(const char* kpPath,
                           const char* kpPrefix,
                           size_t      fileSize,
                           int32_t     testId)
{
  char buffer[64];
  int32_t fd;
  S_SeekIOCTLArguments seekArgs;
  ssize_t bytesRead;
  size_t prefixSize;
  size_t totalRead;
  size_t requestSize;

  fd = VFSOpen(kpPath, O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_USTARFS(testId), fd >= 0, 0, fd,
                        TEST_USTARFS_ENABLED);
  if (fd >= 0)
  {
    prefixSize = strlen(kpPrefix);
    requestSize = fileSize < prefixSize ? fileSize : prefixSize;
    totalRead = 0;
    if (requestSize > 0)
    {
      bytesRead = VFSRead(fd, buffer, requestSize);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 1),
                            bytesRead == (ssize_t)requestSize,
                            requestSize, bytesRead, TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 2),
                            bytesRead < 0 ||
                            memcmp(buffer, kpPrefix, requestSize) == 0,
                            1, bytesRead >= 0 &&
                            memcmp(buffer, kpPrefix, requestSize) == 0,
                            TEST_USTARFS_ENABLED);
      if (bytesRead > 0)
      {
        totalRead += (size_t)bytesRead;
      }
    }

    while (totalRead < fileSize)
    {
      requestSize = fileSize - totalRead;
      if (requestSize > sizeof(buffer))
      {
        requestSize = sizeof(buffer);
      }
      bytesRead = VFSRead(fd, buffer, requestSize);
      if (bytesRead <= 0)
      {
        break;
      }
      totalRead += (size_t)bytesRead;
    }

    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 3),
                          totalRead == fileSize, fileSize, totalRead,
                          TEST_USTARFS_ENABLED);
    bytesRead = VFSRead(fd, buffer, 1);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 4), bytesRead == 0, 0,
                          bytesRead, TEST_USTARFS_ENABLED);
    bytesRead = VFSIOCTL(fd, VFS_IOCTL_FILE_TELL, NULL);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 5),
                          bytesRead == (ssize_t)fileSize,
                          fileSize, bytesRead, TEST_USTARFS_ENABLED);
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    bytesRead = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 6), bytesRead == 0, 0,
                bytesRead, TEST_USTARFS_ENABLED);
    seekArgs.direction = SEEK_CUR;
    seekArgs.offset = fileSize + 1;
    bytesRead = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 7), bytesRead == 0, 0,
                bytesRead, TEST_USTARFS_ENABLED);
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = fileSize + 1;
    bytesRead = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 8), bytesRead == 0, 0,
                bytesRead, TEST_USTARFS_ENABLED);
    seekArgs.direction = (E_SeekDirection)99;
    seekArgs.offset = 0;
    bytesRead = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 9), bytesRead == 0, 0,
                bytesRead, TEST_USTARFS_ENABLED);
    seekArgs.direction = SEEK_END;
    seekArgs.offset = 0;
    bytesRead = VFSIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 10),
                bytesRead == (ssize_t)fileSize,
                fileSize, bytesRead, TEST_USTARFS_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(testId + 11), VFSClose(fd) == 0, 0, 0,
                TEST_USTARFS_ENABLED);
  }
}

char path[VFS_PATH_MAX_LENGTH];
void USTARFSTest(void)
{
  S_DirectoryEntry entry;
  S_SeekIOCTLArguments seekArgs;
  char buffer[32];
  int32_t dirFd;
  int32_t nestedDirFd;
  int32_t deepDirFd;
  int32_t fileFd;
  int32_t retVal;
  ssize_t bytesRead;
  size_t entryCount;
  size_t nameLength;
  size_t nestedEntryCount;
  size_t deepEntryCount;
  bool foundFile;
  bool foundDirectory;

  retVal = VFSMount(NULL, USTAR_TEST_DEVICE, "ustar");
  TEST_POINT_ASSERT_INT(TEST_USTARFS(0), retVal == ERR_INVALID_PARAMETER,
                        ERR_INVALID_PARAMETER, retVal, TEST_USTARFS_ENABLED);
  retVal = VFSMount(USTAR_TEST_MOUNT, NULL, "ustar");
  TEST_POINT_ASSERT_INT(TEST_USTARFS(1), retVal == ERR_INVALID_PARAMETER,
                        ERR_INVALID_PARAMETER, retVal, TEST_USTARFS_ENABLED);
  retVal = VFSMount(USTAR_TEST_MOUNT, USTAR_TEST_DEVICE, "ustar");
  TEST_POINT_ASSERT_INT(TEST_USTARFS(2), retVal == NO_ERROR, NO_ERROR, retVal,
                        TEST_USTARFS_ENABLED);
  if (retVal == NO_ERROR)
  {
    retVal = VFSMount(USTAR_TEST_MOUNT, USTAR_TEST_DEVICE, "unknown");
    TEST_POINT_ASSERT_INT(TEST_USTARFS(3), retVal != NO_ERROR, -1, retVal,
                          TEST_USTARFS_ENABLED);

    retVal = VFSOpen(USTAR_TEST_MOUNT "/missing", O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(4), retVal == -1, -1, retVal,
                          TEST_USTARFS_ENABLED);
    retVal = VFSOpen(USTAR_TEST_MOUNT, O_RDWR, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(5), retVal == -1, -1, retVal,
                          TEST_USTARFS_ENABLED);
    retVal = VFSOpen(NULL, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(6), retVal == -1, -1, retVal,
                TEST_USTARFS_ENABLED);
    retVal = VFSOpen(USTAR_TEST_MOUNT "/folder1/missing", O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(7), retVal == -1, -1, retVal,
                TEST_USTARFS_ENABLED);
    retVal = VFSOpen(USTAR_TEST_MOUNT "/folder1/anotherfolder/missing",
             O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(8), retVal == -1, -1, retVal,
                TEST_USTARFS_ENABLED);
    retVal = VFSOpen(USTAR_TEST_MOUNT "/folder1/smallfile.txt/",
             O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(9), retVal == -1, -1, retVal,
                TEST_USTARFS_ENABLED);
    retVal = VFSOpen(USTAR_TEST_MOUNT "/folder1/smallfile.txt", O_RDWR, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(10), retVal == -1, -1, retVal,
                TEST_USTARFS_ENABLED);

    _TestUSTARFile(USTAR_TEST_MOUNT "/.roos_init", "INIT=/initrd/init",
             17, 20);
    _TestUSTARFile(USTAR_TEST_MOUNT "/fil1.test", "Coucous Truncate me", 19,
             40);
    _TestUSTARFile(USTAR_TEST_MOUNT "/newfile2.txt",
             "Lorem ipsum dolor sit amet, consect", USTAR_LARGE_FILE_SIZE,
             60);
    _TestUSTARFile(USTAR_TEST_MOUNT "/folder1/myfile.file.txt", "", 0, 80);
    _TestUSTARFile(USTAR_TEST_MOUNT "/folder1/smallfile.txt", "I am smol", 9,
             100);
    _TestUSTARFile(USTAR_TEST_MOUNT "/folder1/newfile3.txt",
             "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
             USTAR_LARGE_FILE_SIZE,
             120);
    _TestUSTARFile(USTAR_TEST_MOUNT
             "/folder1/anotherfolder/myfileinfolder.txt", "", 0, 140);
    _TestUSTARFile(USTAR_TEST_MOUNT
             "/folder1/anotherfolder/myfileinfolder - Copie.txt", "",
             0, 160);

    dirFd = VFSOpen(USTAR_TEST_MOUNT, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(200), dirFd >= 0, 0, dirFd,
                          TEST_USTARFS_ENABLED);
    if (dirFd >= 0)
    {
      entryCount = 0;
      foundFile = false;
      foundDirectory = false;
      retVal = VFSReaddir(dirFd, &entry);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(201), retVal == 1 || retVal == 0,
                            1, retVal, TEST_USTARFS_ENABLED);
      while (retVal == 1)
      {
        TEST_POINT_ASSERT_INT(TEST_USTARFS(208 + entryCount * 50),
                              entry.pName[0] != 0, 1, entry.pName[0] != 0,
                              TEST_USTARFS_ENABLED);

        path[0] = 0;
        strcpy(path, USTAR_TEST_MOUNT);
        strcat(path, "/");
        strcat(path, entry.pName);
        nameLength = strlen(path);
        if (nameLength > 0 && path[nameLength - 1] == VFS_PATH_DELIMITER)
        {
          path[nameLength - 1] = 0;
        }

        fileFd = VFSOpen(path, O_RDONLY, 0);
        TEST_POINT_ASSERT_INT(TEST_USTARFS(209 + entryCount * 50), fileFd >= 0,
                              0, fileFd,
                              TEST_USTARFS_ENABLED);
        if (fileFd >= 0)
        {
          if (entry.type == VFS_FILE_TYPE_FILE)
          {
            foundFile = true;
            bytesRead = VFSRead(fileFd, buffer, 0);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(210 + entryCount * 50),
                                  bytesRead == 0, 0, bytesRead,
                                  TEST_USTARFS_ENABLED);
            bytesRead = VFSRead(fileFd, buffer, sizeof(buffer));
            TEST_POINT_ASSERT_INT(TEST_USTARFS(211 + entryCount * 50),
                                  bytesRead >= 0, 0, bytesRead,
                                  TEST_USTARFS_ENABLED);
            retVal = VFSIOCTL(fileFd, VFS_IOCTL_FILE_TELL, NULL);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(212 + entryCount * 50),
                                  retVal == bytesRead, bytesRead, retVal,
                                  TEST_USTARFS_ENABLED);
            seekArgs.direction = SEEK_SET;
            seekArgs.offset = 0;
            retVal = VFSIOCTL(fileFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(213 + entryCount * 50),
                                  retVal == 0, 0, retVal,
                                  TEST_USTARFS_ENABLED);
            bytesRead = VFSRead(fileFd, buffer, 1);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(214 + entryCount * 50),
                                  bytesRead == 0 || bytesRead == 1, 1, bytesRead,
                                  TEST_USTARFS_ENABLED);
            seekArgs.direction = SEEK_CUR;
            seekArgs.offset = 0;
            retVal = VFSIOCTL(fileFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(215 + entryCount * 50),
                                  retVal >= 0, 0, retVal,
                                  TEST_USTARFS_ENABLED);
            seekArgs.direction = (E_SeekDirection)99;
            seekArgs.offset = 0;
            retVal = VFSIOCTL(fileFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(216 + entryCount * 50),
                                  retVal >= 0, 0, retVal,
                                  TEST_USTARFS_ENABLED);
            retVal = VFSWrite(fileFd, buffer, sizeof(buffer));
            TEST_POINT_ASSERT_INT(TEST_USTARFS(217 + entryCount * 50),
                                  retVal == -1, -1, retVal,
                                  TEST_USTARFS_ENABLED);
          }
          else
          {
            foundDirectory = true;
            retVal = VFSReaddir(fileFd, &entry);
            TEST_POINT_ASSERT_INT(TEST_USTARFS(218 + entryCount * 50),
                                  retVal == 1 || retVal == 0 || retVal == -1,
                                  1, retVal, TEST_USTARFS_ENABLED);
          }
          TEST_POINT_ASSERT_INT(TEST_USTARFS(219 + entryCount * 50),
                                VFSClose(fileFd) == 0, 1, 0,
                                TEST_USTARFS_ENABLED);
        }

        ++entryCount;
        retVal = VFSReaddir(dirFd, &entry);
      }
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1000), retVal == 0 || retVal == -1,
                            0, retVal, TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1001), entryCount > 0, 1, entryCount,
                            TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1002), foundFile || foundDirectory,
                            1, foundFile || foundDirectory,
                            TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1003), VFSReaddir(-1, &entry) == -1,
                            -1, 0, TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1004), VFSReaddir(dirFd, NULL) == -1,
                -1, 0, TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1005), VFSRead(dirFd, buffer, 1) == -1,
                -1, 0, TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1006),
                VFSIOCTL(dirFd, 0xFFFFFFFF, NULL) == -1,
                -1, 0, TEST_USTARFS_ENABLED);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1007), VFSClose(dirFd) == 0, 1, 0,
                            TEST_USTARFS_ENABLED);

      nestedDirFd = VFSOpen(USTAR_TEST_MOUNT "/folder1", O_RDONLY, 0);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1008), nestedDirFd >= 0, 0,
                            nestedDirFd, TEST_USTARFS_ENABLED);
      if (nestedDirFd >= 0)
      {
        nestedEntryCount = 0;
        retVal = VFSReaddir(nestedDirFd, &entry);
        while (retVal == 1)
        {
          TEST_POINT_ASSERT_INT(TEST_USTARFS(1009 + nestedEntryCount),
                                entry.pName[0] != 0, 1,
                                entry.pName[0] != 0, TEST_USTARFS_ENABLED);
          ++nestedEntryCount;
          retVal = VFSReaddir(nestedDirFd, &entry);
        }
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1200), retVal == -1, -1, retVal,
                              TEST_USTARFS_ENABLED);
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1201), nestedEntryCount == 4, 4,
                              nestedEntryCount, TEST_USTARFS_ENABLED);
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1202),
                              VFSReaddir(nestedDirFd, &entry) == -1,
                              -1, 0, TEST_USTARFS_ENABLED);
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1203), VFSClose(nestedDirFd) == 0,
                              0, 0, TEST_USTARFS_ENABLED);
      }

      deepDirFd = VFSOpen(USTAR_TEST_MOUNT "/folder1/anotherfolder",
                          O_RDONLY, 0);
      TEST_POINT_ASSERT_INT(TEST_USTARFS(1204), deepDirFd >= 0, 0, deepDirFd,
                            TEST_USTARFS_ENABLED);
      if (deepDirFd >= 0)
      {
        deepEntryCount = 0;
        retVal = VFSReaddir(deepDirFd, &entry);
        while (retVal == 1)
        {
          TEST_POINT_ASSERT_INT(TEST_USTARFS(1205 + deepEntryCount),
                                entry.pName[0] != 0, 1,
                                entry.pName[0] != 0, TEST_USTARFS_ENABLED);
          ++deepEntryCount;
          retVal = VFSReaddir(deepDirFd, &entry);
        }
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1300), retVal == -1, -1, retVal,
                              TEST_USTARFS_ENABLED);
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1301), deepEntryCount == 2, 2,
                              deepEntryCount, TEST_USTARFS_ENABLED);
        TEST_POINT_ASSERT_INT(TEST_USTARFS(1302), VFSClose(deepDirFd) == 0,
                              0, 0, TEST_USTARFS_ENABLED);
      }
    }
    retVal = VFSUnmount(USTAR_TEST_MOUNT);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(1303), retVal == NO_ERROR, NO_ERROR,
                          retVal, TEST_USTARFS_ENABLED);
    retVal = VFSOpen(USTAR_TEST_MOUNT, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_USTARFS(1304), retVal == -1, -1, retVal,
                          TEST_USTARFS_ENABLED);
  }
  TEST_FRAMEWORK_END();
}

#endif