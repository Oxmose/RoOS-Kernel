/*******************************************************************************
 * @file ProcFSTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework ProcFS testing.
 *
 * @details Testing framework ProcFS testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <string.h>
#include <stdint.h>
#include <ProcFS.h>
#include <VirtualFS.h>
#include <TestFramework.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestFramework.h>


/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

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
static uint32_t sOpenCount;
static uint32_t sCloseCount;


/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void* _Open(void* pDriverData, const char* kpPath, int32_t flags,
                   int32_t mode)
{
  (void)kpPath;
  (void)flags;
  (void)mode;
  ++sOpenCount;
  return pDriverData;
}

static int32_t _Close(void* pDriverData, void* pFileHandle)
{
  (void)pDriverData;
  (void)pFileHandle;
  ++sCloseCount;
  return 0;
}

static ssize_t _Read(void* pDriverData, void* pFileHandle, void* pBuffer,
                     size_t count)
{
  (void)pDriverData;
  (void)pFileHandle;
  if (pBuffer != NULL && count >= 4)
  {
    memcpy(pBuffer, "read", 4);
    return 4;
  }
  return -1;
}

static ssize_t _Write(void* pDriverData, void* pFileHandle,
                      const void* kpBuffer, size_t count)
{
  (void)pDriverData;
  (void)pFileHandle;
  (void)kpBuffer;
  return (ssize_t)count;
}

static int32_t _ReadDir(void* pDriverData, void* pFileHandle,
                        S_DirectoryEntry* pDirEntry)
{
  (void)pDriverData;
  (void)pFileHandle;
  (void)pDirEntry;
  return 37;
}

static ssize_t _IOCTL(void* pDriverData, void* pFileHandle,
                      uint32_t operation, void* pArgs)
{
  (void)pDriverData;
  (void)pFileHandle;
  (void)operation;
  (void)pArgs;
  return 73;
}

static void* _FailOpen(void* pDriverData, const char* kpPath, int32_t flags,
                       int32_t mode)
{
  (void)pDriverData;
  (void)kpPath;
  (void)flags;
  (void)mode;
  return (void*)-1;
}

static int32_t _FailClose(void* pDriverData, void* pFileHandle)
{
  (void)pDriverData;
  (void)pFileHandle;
  return -1;
}

static S_ProcFSFileOperations sFops =
{
  .pOpen = _Open,
  .pClose = _Close,
  .pRead = _Read,
  .pWrite = _Write,
  .pReadDir = _ReadDir,
  .pIOCTL = _IOCTL
};

static S_ProcFSFileOperations sNoOpenFops =
{
  .pOpen = NULL,
  .pClose = _Close,
  .pRead = _Read,
  .pWrite = _Write,
  .pReadDir = _ReadDir,
  .pIOCTL = _IOCTL
};

static S_ProcFSFileOperations sFailOpenFops =
{
  .pOpen = _FailOpen,
  .pClose = _Close,
  .pRead = _Read,
  .pWrite = _Write,
  .pReadDir = _ReadDir,
  .pIOCTL = _IOCTL
};

static S_ProcFSFileOperations sNoCloseFops =
{
  .pOpen = _Open,
  .pClose = NULL,
  .pRead = _Read,
  .pWrite = _Write,
  .pReadDir = NULL,
  .pIOCTL = NULL
};

static S_ProcFSFileOperations sFailCloseFops =
{
  .pOpen = _Open,
  .pClose = _FailClose,
  .pRead = NULL,
  .pWrite = NULL,
  .pReadDir = NULL,
  .pIOCTL = NULL
};

static void _TestCreate(void)
{
  S_ProcFSDirEntry* pDirectory;
  S_ProcFSDirEntry* pNestedDirectory;
  S_ProcFSDirEntry* pEntry;
  E_Return retCode;

  pDirectory = NULL;
  retCode = ProcFSCreateDir("procfs_test", NULL, &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_DIR(0), retCode == NO_ERROR,
                          NO_ERROR, retCode, TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_PROCFS_CREATE_DIR(1), pDirectory != NULL,
                            0, (uintptr_t)pDirectory, TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_CREATE_DIR(2),
                        strcmp(pDirectory->name, "procfs_test") == 0, 0,
                        strcmp(pDirectory->name, "procfs_test"),
                        TEST_PROCFS_ENABLED);

  pNestedDirectory = NULL;
  retCode = ProcFSCreateDir("nested", "/procfs_test", &pNestedDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_DIR(3), retCode == NO_ERROR,
                          NO_ERROR, retCode, TEST_PROCFS_ENABLED);
  pEntry = NULL;
  retCode = ProcFSCreateEntry("sample", 6, pNestedDirectory, &sFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_ENTRY(0), retCode == NO_ERROR,
                          NO_ERROR, retCode, TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_PROCFS_CREATE_ENTRY(1), pEntry != NULL, 0,
                            (uintptr_t)pEntry, TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_CREATE_ENTRY(2), pEntry->mode == 6, 6,
                        pEntry->mode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateDir("procfs_test", NULL, &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_DIR(4),
                          retCode == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION, retCode,
                          TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateEntry("sample", 6, pNestedDirectory, &sFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_ENTRY(3),
                          retCode == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION, retCode,
                          TEST_PROCFS_ENABLED);

  pEntry = NULL;
  retCode = ProcFSCreateEntry("sampler", 6, pNestedDirectory, &sFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_ENTRY(4), retCode == NO_ERROR,
                          NO_ERROR, retCode, TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_CREATE_ENTRY(5),
                        pEntry != NULL && strcmp(pEntry->name, "sampler") == 0,
                        0, pEntry == NULL ? -1 : strcmp(pEntry->name, "sampler"),
                        TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateDir("invalid_child",
                            "/procfs_test/nested/sample",
                            &pNestedDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_DIR(5),
                          retCode == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION, retCode,
                          TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateEntry("invalid_child", 6, pEntry, &sFops, NULL,
                              &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_CREATE_ENTRY(6),
                          retCode == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION, retCode,
                          TEST_PROCFS_ENABLED);
}

static void _TestErrors(void)
{
  S_ProcFSDirEntry* pEntry;
  E_Return retCode;

  pEntry = (void*)0x1;
  retCode = ProcFSCreateDir("", NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(0),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_PROCFS_ERRORS(1), pEntry == (void*)0x1, 0x1,
                            (uintptr_t)pEntry, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateDir("bad/name", NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(2),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateDir("missing", "/does_not_exist", &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(3), retCode == ERR_NOT_FOUND,
                          ERR_NOT_FOUND, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateEntry(NULL, 6, NULL, NULL, NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(4),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateEntry("bad/name", 6, NULL, &sFops, NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(5),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateDir(NULL, NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(6),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateDir("invalid", NULL, NULL);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(7),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateEntry("invalid", 6, NULL, &sFops, NULL, NULL);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(8),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSCreateEntry("invalid", 6, NULL, NULL, NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(9),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveDir(NULL);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(10),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  pEntry = NULL;
  retCode = ProcFSRemoveDir(&pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(11),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveEntry(NULL, NULL);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(12),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveEntry("bad/name", NULL);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_ERRORS(13),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
}

static void _TestOpenBranches(void)
{
  S_ProcFSDirEntry* pDirectory;
  S_ProcFSDirEntry* pEntry;
  int32_t fd;
  int32_t retVal;
  E_Return retCode;

  pDirectory = NULL;
  retCode = ProcFSCreateDir("branches", NULL, &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(30), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);

  pEntry = NULL;
  retCode = ProcFSCreateEntry("no_open", 6, pDirectory, &sNoOpenFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(31), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/branches/no_open", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(32), fd == -1, -1, fd,
                        TEST_PROCFS_ENABLED);

  pEntry = NULL;
  retCode = ProcFSCreateEntry("fail_open", 6, pDirectory, &sFailOpenFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(33), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/branches/fail_open", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(34), fd == -1, -1, fd,
                        TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/branches/fail", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(35), fd == -1, -1, fd,
                        TEST_PROCFS_ENABLED);

  pEntry = NULL;
  retCode = ProcFSCreateEntry("no_close", 6, pDirectory, &sNoCloseFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(36), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/branches/no_close", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(37), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(38), retVal == 0, 0, retVal,
                        TEST_PROCFS_ENABLED);

  pEntry = NULL;
  retCode = ProcFSCreateEntry("fail_close", 6, pDirectory, &sFailCloseFops, NULL,
                              &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(39), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/branches/fail_close", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(40), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(41), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);

  fd = VFSOpen("/proc", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(42), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(43), retVal == 0, 0, retVal,
                        TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc", O_RDWR, 0);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(44), fd == -1, -1, fd,
                        TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/branches", O_RDWR, 0);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(45), fd == -1, -1, fd,
                        TEST_PROCFS_ENABLED);
}

static void _TestOperations(void)
{
  S_ProcFSDirEntry* pDirectory;
  S_ProcFSDirEntry* pEntry;
  S_DirectoryEntry dirEntry;
  char buffer[8];
  int32_t fd;
  int32_t retVal;
  E_Return retCode;

  pDirectory = NULL;
  retCode = ProcFSCreateDir("ops", NULL, &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(0), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  pEntry = NULL;
  retCode = ProcFSCreateEntry("file", 6, pDirectory, &sFops, NULL,&pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_OPEN(1), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);

  sOpenCount = sCloseCount = 0;
  fd = VFSOpen("/proc/ops/file", O_RDWR, 6);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(2), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_PROCFS_OPEN(3), sOpenCount == 1, 1,
                           sOpenCount, TEST_PROCFS_ENABLED);
  retVal = VFSRead(fd, buffer, sizeof(buffer));
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READ(0), retVal == 4, 4, retVal,
                        TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READ(1), memcmp(buffer, "read", 4) == 0,
                        0, memcmp(buffer, "read", 4), TEST_PROCFS_ENABLED);
  retVal = VFSRead(fd, buffer, 2);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READ(3), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSWrite(fd, "x", 1);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_WRITE(0), retVal == 1, 1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSIOCTL(fd, 1, NULL);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_IOCTL(0), retVal == 73, 73, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(0), retVal == 37, 37, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(4), retVal == 0, 0, retVal,
                        TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_PROCFS_OPEN(5), sCloseCount == 1, 1,
                           sCloseCount, TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/ops/file", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(10), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSWrite(fd, "x", 1);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_WRITE(1), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSIOCTL(fd, 1, NULL);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_IOCTL(1), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(11), retVal == 0, 0, retVal,
                        TEST_PROCFS_ENABLED);

  fd = VFSOpen("/proc/ops/file", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(12), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSRead(fd, NULL, 1);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READ(2), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(13), retVal == 0, 0, retVal,
                        TEST_PROCFS_ENABLED);

  fd = VFSOpen("/proc/ops", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(1), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(2), retVal == 1, 1, retVal,
                        TEST_PROCFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(3), strcmp(dirEntry.pName, ".") == 0,
                        0, strcmp(dirEntry.pName, "."), TEST_PROCFS_ENABLED);
  VFSClose(fd);

  fd = VFSOpen("/proc/ops/file", O_RDONLY, 1);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_OPEN(14), fd == -1, -1, fd,
                        TEST_PROCFS_ENABLED);

  pDirectory = NULL;
  retCode = ProcFSCreateDir("empty", NULL, &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_READDIR(6), retCode == NO_ERROR,
                          NO_ERROR, retCode, TEST_PROCFS_ENABLED);
  fd = VFSOpen("/proc/empty", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(7), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(8), retVal == 1, 1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(9), retVal == 0, 0, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(10), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  VFSClose(fd);

  fd = VFSOpen("/proc/branches/no_close", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(11), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READDIR(12), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  VFSClose(fd);

  fd = VFSOpen("/proc/branches/fail_close", O_RDONLY, 4);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READ(4), fd >= 0, 0, fd,
                        TEST_PROCFS_ENABLED);
  retVal = VFSRead(fd, buffer, sizeof(buffer));
  TEST_POINT_ASSERT_INT(TEST_PROCFS_READ(5), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSWrite(fd, "x", 1);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_WRITE(2), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  retVal = VFSIOCTL(fd, 1, NULL);
  TEST_POINT_ASSERT_INT(TEST_PROCFS_IOCTL(2), retVal == -1, -1, retVal,
                        TEST_PROCFS_ENABLED);
  VFSClose(fd);
}

static void _TestRemove(void)
{
  S_ProcFSDirEntry* pDirectory;
  S_ProcFSDirEntry* pEntry;
  E_Return retCode;

  pDirectory = NULL;
  retCode = ProcFSCreateDir("remove", NULL, &pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(0), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  pEntry = NULL;
  retCode = ProcFSCreateEntry("file", 6, pDirectory, &sFops, NULL, &pEntry);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(1), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveDir(&pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(2),
                          retCode == ERR_UNAUTHORIZED_ACTION,
                          ERR_UNAUTHORIZED_ACTION, retCode,
                          TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveEntry("file", pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(3), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveDir(&pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(4), retCode == NO_ERROR, NO_ERROR,
                          retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveEntry("unknown", NULL);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(5),
                          retCode == ERR_NOT_FOUND,
                          ERR_NOT_FOUND, retCode, TEST_PROCFS_ENABLED);
  retCode = ProcFSRemoveDir(&pDirectory);
  TEST_POINT_ASSERT_RCODE(TEST_PROCFS_REMOVE(6),
                          retCode == ERR_INVALID_PARAMETER,
                          ERR_INVALID_PARAMETER, retCode, TEST_PROCFS_ENABLED);
}

void ProcFSTest(void)
{
  _TestCreate();
  _TestErrors();
  _TestOpenBranches();
  _TestOperations();
  _TestRemove();
  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/