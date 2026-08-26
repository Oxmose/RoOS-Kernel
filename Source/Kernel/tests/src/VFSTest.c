

/*******************************************************************************
 * @file VFSTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework VFS testing.
 *
 * @details Testing framework VFS testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <VirtualFS.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelQueue.h>
#include <Vector.h>

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

typedef struct
{
  S_Vector* pFDTable;
  S_KernelSpinlock lock;
} S_FDTable;

typedef struct
{
  char* pFilePath;
  void* pFileHandle;
  S_FSDriver* pDriver;
  S_KernelSpinlock lock;
} S_FileDescriptorShared;

typedef struct
{
  uint32_t tableId;
  S_FileDescriptorShared* pShared;
  int openMode;
  int openFlags;
} S_FileDescriptor;

typedef ssize_t (*T_VFSCleanPath)(char* pCleanPath, const char* kpOriginalPath);
typedef S_VFSNode* (*T_VFSFindNode)(S_VFSNode*   pRoot,
                                const char*  kpPath,
                                const size_t kPathLength,
                                size_t*      pNextToken);
typedef void (*T_VFSAddDriverNode)(S_VFSNode*  pRoot,
                                   const char* kpPath,
                                   size_t      pathLen,
                                   void* pDriver);
typedef bool (*T_RemoveDriverNode)(S_VFSNode* pRoot);
typedef int32_t (*T_CreateFD)(S_FDTable*  pTable,
                               S_FSDriver* pDriver,
                               void*       pFileHandle,
                               const char* kpPath,
                               const int   kFlags,
                               const int   kMode);
typedef void (*T_DestroyFD)(S_FDTable* pTable, const int32_t kFD);

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
static char buffer[VFS_PATH_MAX_LENGTH];
static S_VFSNode sNodePool[50];
static uint32_t sTestValue = 0;
static S_FSDriver driver0;
static S_FSDriver driver1;
static S_FSDriver driver2;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void _TestNextToken(void);
static void _TestCreateFDTable(void);
static void _TestDestroyFDTable(void);
static void _TestVFSGeneric(void);
static void _TestVFSOpen(void);
static void _TestVFSClose(void);
static void _TestVFSRead(void);
static void _TestVFSWrite(void);
static void _TestVFSReadDir(void);
static void _TestVFSIOCTL(void);
static void _TestVFSMount(void);
static void _TestVFSUnmount(void);

static void* _DummyOpen(void*       pDriverData,
                        const char* kpPath,
                        int32_t     flags,
                        int32_t     mode);
static int32_t _DummyClose(void* pDriverData, void* pFileHandle);
static ssize_t _DummyRead(void*  pDriverData,
                             void*  pFileHandle,
                             void*  pBuffer,
                             size_t count);
static ssize_t _DummyWrite(void*       pDriverData,
                              void*       pFileHandle,
                              const void* kpBuffer,
                              size_t      count);
static int32_t _DummyReadDir(void*             pDriverData,
                                void*             pFileHandle,
                                S_DirectoryEntry* pDirEntry);
static ssize_t _DummyIOCTL(void*    pDriverData,
                              void*    pFileHandle,
                              uint32_t operation,
                              void*    pArgs);
static E_Return _DummyMount(const char* kpPath,
                     const char* kpDevPath,
                     void**      pDriverMountData);
static E_Return _DummyUnmount(void* pDriverMountData);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _TestNextToken(void)
{
  ssize_t nextToken;

  nextToken = VFSGetNextPathTokenPosition("////", 4);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(0),
                          nextToken == 1,
                          1,
                          nextToken,
                          TEST_VFS_ENABLED);
  nextToken = VFSGetNextPathTokenPosition("this is a test/sdf", 18);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(1),
                          nextToken == 15,
                          15,
                          nextToken,
                          TEST_VFS_ENABLED);
  nextToken = VFSGetNextPathTokenPosition("this is a test/", 15);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(2),
                          nextToken == -1,
                          -1,
                          nextToken,
                          TEST_VFS_ENABLED);
  nextToken = VFSGetNextPathTokenPosition("/", 1);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(3),
                          nextToken == -1,
                          -1,
                          nextToken,
                          TEST_VFS_ENABLED);
  nextToken = VFSGetNextPathTokenPosition("/sdf", 4);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(4),
                          nextToken == 1,
                          1,
                          nextToken,
                          TEST_VFS_ENABLED);
  nextToken = VFSGetNextPathTokenPosition("sdfsdffend", 10);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(5),
                          nextToken == -1,
                          -1,
                          nextToken,
                          TEST_VFS_ENABLED);
  nextToken = VFSGetNextPathTokenPosition("sdfsdffend/", 11);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_NEXT_TOKEN(6),
                          nextToken == -1,
                          -1,
                          nextToken,
                          TEST_VFS_ENABLED);
}

static void _TestCreateFDTable(void)
{
  S_KernelProcess process;
  S_FDTable* pTable;
  E_Return retCode;

  memset(&process, 0, sizeof(process));

  retCode = CreateProcessHeap(&process.pHeap);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_TABLE_CREATE(1),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  retCode = CreateProcessFDTable(&process);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_TABLE_CREATE(2),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  pTable = (S_FDTable*)process.pFileDescriptorTable;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_TABLE_CREATE(3),
                            pTable != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTable,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FD_TABLE_CREATE(4),
                           pTable->pFDTable->size == 32,
                           32,
                           pTable->pFDTable->size,
                           TEST_VFS_ENABLED);

  DestroyProcessFDTable(&process);
}

static void _TestDestroyFDTable(void)
{
  S_KernelProcess process;
  E_Return retCode;

  memset(&process, 0, sizeof(process));

  retCode = CreateProcessHeap(&process.pHeap);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_TABLE_DESTROY(0),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  retCode = CreateProcessFDTable(&process);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_TABLE_DESTROY(1),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  DestroyProcessFDTable(&process);

  /* Just see if we are back without crashing */
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_TABLE_DESTROY(2),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
}

static void _TestVFSGeneric(void)
{
  S_FSDriver* pDriver;
  int32_t fd;
  int32_t retVal;
  S_DirectoryEntry dirEntry;
  char buffer[16];

  pDriver = RegisterVFSDriver("/test0/sub",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_GENERIC(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test0", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_GENERIC(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);

  retVal = VFSRead(fd, buffer, sizeof(buffer));
  TEST_POINT_ASSERT_DWORD(TEST_VFS_GENERIC(2),
                          retVal == -1,
                          -1,
                          retVal,
                          TEST_VFS_ENABLED);
  retVal = VFSWrite(fd, "x", 1);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_GENERIC(3),
                          retVal == -1,
                          -1,
                          retVal,
                          TEST_VFS_ENABLED);

  memset(&dirEntry, 0, sizeof(dirEntry));
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_VFS_GENERIC(4),
                        retVal == 0,
                        0,
                        retVal,
                        TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_VFS_GENERIC(5),
                        strcmp(dirEntry.pName, "sub") == 0,
                        0,
                        strcmp(dirEntry.pName, "sub"),
                        TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_VFS_GENERIC(6),
                        dirEntry.filenameLength == 3,
                        3,
                        dirEntry.filenameLength,
                        TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_VFS_GENERIC(7),
                        dirEntry.type == VFS_FILE_TYPE_DIR,
                        VFS_FILE_TYPE_DIR,
                        dirEntry.type,
                        TEST_VFS_ENABLED);
  retVal = VFSIOCTL(fd, 0x1234, NULL);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_GENERIC(8),
                          retVal == -1,
                          -1,
                          retVal,
                          TEST_VFS_ENABLED);

  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_VFS_GENERIC(9),
                        retVal == 0,
                        0,
                        retVal,
                        TEST_VFS_ENABLED);
}

static void _TestVFSOpen(void)
{
  S_FSDriver* pDriver;
  int32_t fd;

  pDriver = RegisterVFSDriver("/test1",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_OPEN(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test1/file", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_OPEN(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_OPEN(2),
                           sTestValue == 1,
                           1,
                           sTestValue,
                           TEST_VFS_ENABLED);

  fd = VFSOpen("/unknown", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_OPEN(3),
                        fd == -1,
                        -1,
                        fd,
                        TEST_VFS_ENABLED);
}

static void _TestVFSClose(void)
{
  S_FSDriver* pDriver;
  int32_t fd;
  int32_t retVal;

  pDriver = RegisterVFSDriver("/test2",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_CLOSE(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test2/file", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_CLOSE(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);
  sTestValue = 0;
  retVal = VFSClose(fd);
  TEST_POINT_ASSERT_INT(TEST_VFS_CLOSE(2),
                        retVal == 42,
                        42,
                        retVal,
                        TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_CLOSE(3),
                           sTestValue == 2,
                           2,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static void _TestVFSRead(void)
{
  S_FSDriver* pDriver;
  int32_t fd;
  ssize_t retVal;
  char buffer[16];

  pDriver = RegisterVFSDriver("/test3",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_READ(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test3/file", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_READ(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);
  sTestValue = 0;
  retVal = VFSRead(fd, buffer, sizeof(buffer));
  TEST_POINT_ASSERT_DWORD(TEST_VFS_READ(2),
                          retVal == -42,
                          -42,
                          retVal,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_READ(3),
                           sTestValue == 3,
                           3,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static void _TestVFSWrite(void)
{
  S_FSDriver* pDriver;
  int32_t fd;
  ssize_t retVal;
  char buffer[16];

  pDriver = RegisterVFSDriver("/test4",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_WRITE(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test4/file", O_RDWR, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_WRITE(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);
  sTestValue = 0;
  retVal = VFSWrite(fd, buffer, sizeof(buffer));
  TEST_POINT_ASSERT_DWORD(TEST_VFS_WRITE(2),
                          retVal == -42,
                          -42,
                          retVal,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_WRITE(3),
                           sTestValue == 4,
                           4,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static void _TestVFSReadDir(void)
{
  S_FSDriver* pDriver;
  int32_t fd;
  int32_t retVal;
  S_DirectoryEntry dirEntry;

  pDriver = RegisterVFSDriver("/test5",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_READDIR(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test5/file", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_READDIR(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);
  sTestValue = 0;
  retVal = VFSReaddir(fd, &dirEntry);
  TEST_POINT_ASSERT_INT(TEST_VFS_READDIR(2),
                        retVal == 678,
                        678,
                        retVal,
                        TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_READDIR(3),
                           sTestValue == 5,
                           5,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static void _TestVFSIOCTL(void)
{
  S_FSDriver* pDriver;
  int32_t fd;
  ssize_t retVal;

  pDriver = RegisterVFSDriver("/test6",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_IOCTL(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  fd = VFSOpen("/test6/file", O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_VFS_IOCTL(1),
                        fd >= 0,
                        0,
                        fd,
                        TEST_VFS_ENABLED);
  sTestValue = 0;
  retVal = VFSIOCTL(fd, 0x1234, NULL);
  TEST_POINT_ASSERT_DWORD(TEST_VFS_IOCTL(2),
                          retVal == 1010,
                          1010,
                          retVal,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_IOCTL(3),
                           sTestValue == 6,
                           6,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static void _TestVFSMount(void)
{
  E_Return retCode;

  memset(&driver0, 0, sizeof(driver0));
  driver0.pName = "test-fs";
  driver0.pOpen = _DummyOpen;
  driver0.pClose = _DummyClose;
  driver0.pRead = _DummyRead;
  driver0.pWrite = _DummyWrite;
  driver0.pReadDir = _DummyReadDir;
  driver0.pIOCTL = _DummyIOCTL;
  driver0.pMount = _DummyMount;
  driver0.pUnmount = _DummyUnmount;
  driver0.pDriverData = (void*)0xC0DEAABB;

  sTestValue = 0;
  retCode = VFSMount("/test-mount", "/dev/test", "test-fs");
  TEST_POINT_ASSERT_RCODE(TEST_VFS_MOUNT(0),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_MOUNT(1),
                           sTestValue == 7,
                           7,
                           sTestValue,
                           TEST_VFS_ENABLED);

  retCode = VFSUnmount("/test-mount");
  TEST_POINT_ASSERT_RCODE(TEST_VFS_MOUNT(2),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_MOUNT(3),
                           sTestValue == 8,
                           8,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static void _TestVFSUnmount(void)
{
  S_FSDriver* pDriver;
  E_Return retCode;

  pDriver = RegisterVFSDriver("/test7",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_UNMOUNT(0),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);
  pDriver->pUnmount = _DummyUnmount;
  pDriver->pMount = _DummyMount;
  sTestValue = 0;
  retCode = VFSUnmount("/test7");
  TEST_POINT_ASSERT_RCODE(TEST_VFS_UNMOUNT(1),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_UNMOUNT(2),
                           sTestValue == 8,
                           8,
                           sTestValue,
                           TEST_VFS_ENABLED);
}

static uint32_t lastTestVal = 0;
static void* _DummyOpen(void*       pDriverData,
                        const char* kpPath,
                        int32_t     flags,
                        int32_t     mode)
{
  (void)kpPath;
  (void)flags;
  (void)mode;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(63 + lastTestVal++),
                            pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriverData,
                            TEST_VFS_ENABLED);
  sTestValue = 1;

  return (void*)0xAABBCCDDEE;
}
static int32_t _DummyClose(void* pDriverData, void* pFileHandle)
{
  (void)pFileHandle;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(63 + lastTestVal++),
                            pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriverData,
                            TEST_VFS_ENABLED);
  sTestValue = 2;

  return 42;
}
static ssize_t _DummyRead(void*  pDriverData,
                             void*  pFileHandle,
                             void*  pBuffer,
                             size_t count)
{
  (void)pFileHandle;
  (void)pBuffer;
  (void)count;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(63 + lastTestVal++),
                            pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriverData,
                            TEST_VFS_ENABLED);
  sTestValue = 3;

  return -42;
}
static ssize_t _DummyWrite(void*       pDriverData,
                              void*       pFileHandle,
                              const void* kpBuffer,
                              size_t      count)
{
  (void)pFileHandle;
  (void)kpBuffer;
  (void)count;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(63 + lastTestVal++),
                            pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriverData,
                            TEST_VFS_ENABLED);
  sTestValue = 4;

  return -42;
}
static int32_t _DummyReadDir(void*             pDriverData,
                                void*             pFileHandle,
                                S_DirectoryEntry* pDirEntry)
{
  (void)pFileHandle;
  (void)pDirEntry;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(63 + lastTestVal++),
                            pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriverData,
                            TEST_VFS_ENABLED);
  sTestValue = 5;

  return 678;
}

static ssize_t _DummyIOCTL(void*    pDriverData,
                              void*    pFileHandle,
                              uint32_t operation,
                              void*    pArgs)
{
  (void)pFileHandle;
  (void)operation;
  (void)pArgs;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(63 + lastTestVal++),
                            pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriverData,
                            TEST_VFS_ENABLED);
  sTestValue = 6;

  return 1010;
}

static E_Return _DummyMount(const char* kpPath,
                     const char* kpDevPath,
                     void**      pDriverMountData)
{
  (void)kpPath;
  (void)kpDevPath;
  (void)pDriverMountData;
  sTestValue = 7;
  return NO_ERROR;
}

static E_Return _DummyUnmount(void* pDriverMountData)
{
  (void)pDriverMountData;
  sTestValue = 8;
  return NO_ERROR;
}

void VFSCleanPathTest(void* pArgs)
{
  ssize_t        returnVal;
  T_VFSCleanPath cleanPath;
  int32_t        cmpRet;

  cleanPath = (T_VFSCleanPath)pArgs;

  /* Check max path size */
  buffer[0] = 0;
  returnVal = cleanPath(buffer,
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters"
                        "/this_is_a_test/for/the/longest/path/that_willgrow_fur"
                        "ther/than/the_maximal/allowed_number/of_characters");

  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(0),
                          returnVal == -1,
                          -1,
                          returnVal,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(1),
                          buffer[0] == 0,
                          0,
                          buffer[0],
                          TEST_VFS_ENABLED);

  /* Check max file name size */
  buffer[0] = 0;
  returnVal = cleanPath(buffer,
                        "this_is_a_test_for_the_longest_name_that_willgrow_furt"
                        "her_than_the_maximal_allowed_number_of_characters_this"
                        "_is_a_test_for_the_longest_name_that_willgrow_further_"
                        "than_the_maximal_allowed_number_of_characters_this_is_"
                        "a_test_for_the_longest_name_that_willgro");

  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(2),
                          returnVal == 257,
                          257,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer,
                  "/this_is_a_test_for_the_longest_name_that_willgrow_furt"
                  "her_than_the_maximal_allowed_number_of_characters_this"
                  "_is_a_test_for_the_longest_name_that_willgrow_further_"
                  "than_the_maximal_allowed_number_of_characters_this_is_"
                  "a_test_for_the_longest_name_that_willgro");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(3),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
  buffer[0] = 0;
  returnVal = cleanPath(buffer,
                        "this_is_a_test_for_the_longest_name_that_willgrow_furt"
                        "her_than_the_maximal_allowed_number_of_characters_this"
                        "_is_a_test_for_the_longest_name_that_willgrow_further_"
                        "than_the_maximal_allowed_number_of_characters_this_is_"
                        "a_test_for_the_longest_name_that_willgrow");

  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(4),
                          returnVal == -1,
                          -1,
                          returnVal,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(5),
                          buffer[0] == 0,
                          0,
                          buffer[0],
                          TEST_VFS_ENABLED);

  /* Check adding first delimiter */
  buffer[0] = 0;
  returnVal = cleanPath(buffer, "notstartdelimiter");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(6),
                          returnVal == 18,
                          18,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/notstartdelimiter");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(7),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  /* Check handling the . character */
  buffer[0] = 0;
  returnVal = cleanPath(buffer, "/test/./sdf/./././sdf.sdf./.sdf/sdf.sfd/./");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(8),
                          returnVal == 31,
                          31,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/test/sdf/sdf.sdf./.sdf/sdf.sfd");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(9),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  buffer[0] = 0;
  returnVal = cleanPath(buffer, "///test//./one/../two/");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(40),
                          returnVal == 9,
                          9,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/test/two");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(41),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  returnVal = cleanPath(buffer, "/.");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(10),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(11),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  returnVal = cleanPath(buffer, "/./");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(12),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(13),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
  returnVal = cleanPath(buffer, "/./././.");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(14),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(15),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
  returnVal = cleanPath(buffer, ".");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(16),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(17),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  /* Check handling the .. character */
  buffer[0] = 0;
  returnVal = cleanPath(buffer, "/test/again/./sdf/./../../sdf..sdf../..sdf/sdf..sfd/..");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(18),
                          returnVal == 22,
                          22,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/test/sdf..sdf../..sdf");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(19),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  returnVal = cleanPath(buffer, "/../");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(20),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(21),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  returnVal = cleanPath(buffer, "/..");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(22),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(23),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
  returnVal = cleanPath(buffer, "/../../../");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(24),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(25),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  /* Check handling the ... character */
  buffer[0] = 0;
  returnVal = cleanPath(buffer, "/test/./sdf/..././fdf/../sdf...sdf.../...sdf/sdf...sfd/..../sdf../...");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(26),
                          returnVal == 58,
                          58,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/test/sdf/.../sdf...sdf.../...sdf/sdf...sfd/..../sdf../...");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(27),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  returnVal = cleanPath(buffer, "/...");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(28),
                          returnVal == 4,
                          4,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/...");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(29),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  returnVal = cleanPath(buffer, "/.../..../");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(30),
                          returnVal == 9,
                          9,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/.../....");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(31),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
  returnVal = cleanPath(buffer, "/.../...../....../.......");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(32),
                          returnVal == 25,
                          25,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/.../...../....../.......");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(33),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
  returnVal = cleanPath(buffer, "....");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(34),
                          returnVal == 5,
                          5,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/....");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(35),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  /* Check traling delimiter */
  returnVal = cleanPath(buffer, "_is_a_test/");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(36),
                          returnVal == 11,
                          11,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/_is_a_test");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(37),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);

  /* Handling emtpty path */
  returnVal = cleanPath(buffer, "");
  TEST_POINT_ASSERT_DWORD(TEST_VFS_CLEAN_PATH(38),
                          returnVal == 1,
                          1,
                          returnVal,
                          TEST_VFS_ENABLED);
  cmpRet = strcmp(buffer, "/");
  TEST_POINT_ASSERT_UBYTE(TEST_VFS_CLEAN_PATH(39),
                          cmpRet == 0,
                          0,
                          cmpRet,
                          TEST_VFS_ENABLED);
}

void VFSFindNodeTest(void* pArgs)
{
  T_VFSFindNode pFindNode = (T_VFSFindNode)pArgs;
  S_VFSNode* pNode;
  size_t nextToken;

  (void)pArgs;

  /* Create the test graph */
  memcpy(sNodePool[0].pMountPoint, "", 1);
  sNodePool[0].mountPointLength = 0;
  sNodePool[0].pDriver = (void*)0x42424242;
  sNodePool[0].pFirstChild = &sNodePool[1];
  sNodePool[0].pNextSibling = NULL;
  sNodePool[0].pParent = NULL;

  memcpy(sNodePool[1].pMountPoint, "t1", 3);
  sNodePool[1].mountPointLength = 2;
  sNodePool[1].pDriver = NULL;
  sNodePool[1].pFirstChild = &sNodePool[4];
  sNodePool[1].pNextSibling = &sNodePool[2];
  sNodePool[1].pParent = &sNodePool[0];

  memcpy(sNodePool[2].pMountPoint, "t2", 3);
  sNodePool[2].mountPointLength = 2;
  sNodePool[2].pDriver = (void*)0xA0A0A0A0;
  sNodePool[2].pFirstChild = NULL;
  sNodePool[2].pNextSibling = &sNodePool[3];
  sNodePool[2].pParent = &sNodePool[0];

  memcpy(sNodePool[3].pMountPoint, "t3", 3);
  sNodePool[3].mountPointLength = 2;
  sNodePool[3].pDriver = (void*)0xA5A5A5A5;
  sNodePool[3].pFirstChild = &sNodePool[6];
  sNodePool[3].pNextSibling = NULL;
  sNodePool[3].pParent = &sNodePool[0];

  memcpy(sNodePool[4].pMountPoint, "t11", 4);
  sNodePool[4].mountPointLength = 3;
  sNodePool[4].pDriver = NULL;
  sNodePool[4].pFirstChild = &sNodePool[8];
  sNodePool[4].pNextSibling = &sNodePool[5];
  sNodePool[4].pParent = &sNodePool[1];

  memcpy(sNodePool[5].pMountPoint, "t12", 5);
  sNodePool[5].mountPointLength = 3;
  sNodePool[5].pDriver = (void*)0x05050505;
  sNodePool[5].pFirstChild = NULL;
  sNodePool[5].pNextSibling = NULL;
  sNodePool[5].pParent = &sNodePool[1];

  memcpy(sNodePool[6].pMountPoint, "t31", 5);
  sNodePool[6].mountPointLength = 3;
  sNodePool[6].pDriver = NULL;
  sNodePool[6].pFirstChild = &sNodePool[9];
  sNodePool[6].pNextSibling = &sNodePool[7];
  sNodePool[6].pParent = &sNodePool[3];

  memcpy(sNodePool[7].pMountPoint, "t32", 5);
  sNodePool[7].mountPointLength = 3;
  sNodePool[7].pDriver = NULL;
  sNodePool[7].pFirstChild = &sNodePool[10];
  sNodePool[7].pNextSibling = NULL;
  sNodePool[7].pParent = &sNodePool[3];

  memcpy(sNodePool[8].pMountPoint, "t111", 5);
  sNodePool[8].mountPointLength = 4;
  sNodePool[8].pDriver = NULL;
  sNodePool[8].pFirstChild = NULL;
  sNodePool[8].pNextSibling = NULL;
  sNodePool[8].pParent = &sNodePool[4];

  memcpy(sNodePool[9].pMountPoint, "t311", 5);
  sNodePool[9].mountPointLength = 4;
  sNodePool[9].pDriver = (void*)0xDEADBEEF;
  sNodePool[9].pFirstChild = NULL;
  sNodePool[9].pNextSibling = NULL;
  sNodePool[9].pParent = &sNodePool[6];

  memcpy(sNodePool[10].pMountPoint, "t321", 5);
  sNodePool[10].mountPointLength = 4;
  sNodePool[10].pDriver = (void*)0xDEADBEEF;
  sNodePool[10].pFirstChild = &sNodePool[13];
  sNodePool[10].pNextSibling = &sNodePool[11];
  sNodePool[10].pParent = &sNodePool[7];

  memcpy(sNodePool[11].pMountPoint, "t322", 5);
  sNodePool[11].mountPointLength = 4;
  sNodePool[11].pDriver = NULL;
  sNodePool[11].pFirstChild = NULL;
  sNodePool[11].pNextSibling = &sNodePool[12];
  sNodePool[11].pParent = &sNodePool[7];

  memcpy(sNodePool[12].pMountPoint, "t323", 5);
  sNodePool[12].mountPointLength = 4;
  sNodePool[12].pDriver = NULL;
  sNodePool[12].pFirstChild = NULL;
  sNodePool[12].pNextSibling = NULL;
  sNodePool[12].pParent = &sNodePool[7];

  memcpy(sNodePool[13].pMountPoint, "t3211", 6);
  sNodePool[13].mountPointLength = 5;
  sNodePool[13].pDriver = (void*)0xDEADC0DE;
  sNodePool[13].pFirstChild = NULL;
  sNodePool[13].pNextSibling = NULL;
  sNodePool[13].pParent = &sNodePool[10];

  /* Invalid path tests */
  pNode = pFindNode(sNodePool, "", 0, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(0),
                            pNode == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(1),
                           nextToken == 0,
                           0,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/sdf", 4, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(2),
                            pNode == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(3),
                           nextToken == 1,
                           1,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/sdf/sdfs", 9, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(4),
                            pNode == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(5),
                           nextToken == 1,
                           1,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "dfg", 3, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(6),
                            pNode == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(7),
                           nextToken == 0,
                           0,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "///", 3, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(8),
                            pNode == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(9),
                           nextToken == 1,
                           1,
                           nextToken,
                           TEST_VFS_ENABLED);

  /* Valid full path tests */
  pNode = pFindNode(sNodePool, "/", 1, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(10),
                            pNode == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(11),
                           nextToken == 1,
                           1,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t1", 3, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(12),
                            pNode == &sNodePool[1],
                            (uintptr_t)&sNodePool[1],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(13),
                           nextToken == 3,
                           3,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t2", 3, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(14),
                            pNode == &sNodePool[2],
                            (uintptr_t)&sNodePool[2],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(15),
                           nextToken == 3,
                           3,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3", 3, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(16),
                            pNode == &sNodePool[3],
                            (uintptr_t)&sNodePool[3],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(17),
                           nextToken == 3,
                           3,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t1/t11", 7, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(18),
                            pNode == &sNodePool[4],
                            (uintptr_t)&sNodePool[4],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(19),
                           nextToken == 7,
                           7,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t1/t12", 7, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(20),
                            pNode == &sNodePool[5],
                            (uintptr_t)&sNodePool[5],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(21),
                           nextToken == 7,
                           7,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t1/t11/t111", 12, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(22),
                            pNode == &sNodePool[8],
                            (uintptr_t)&sNodePool[8],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(23),
                           nextToken == 12,
                           12,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t31", 7, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(24),
                            pNode == &sNodePool[6],
                            (uintptr_t)&sNodePool[6],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(25),
                           nextToken == 7,
                           7,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t32", 7, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(26),
                            pNode == &sNodePool[7],
                            (uintptr_t)&sNodePool[7],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(27),
                           nextToken == 7,
                           7,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t31/t311", 12, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(28),
                            pNode == &sNodePool[9],
                            (uintptr_t)&sNodePool[9],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(29),
                           nextToken == 12,
                           12,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t32/t321", 12, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(30),
                            pNode == &sNodePool[10],
                            (uintptr_t)&sNodePool[10],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(31),
                           nextToken == 12,
                           12,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t32/t322", 12, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(32),
                            pNode == &sNodePool[11],
                            (uintptr_t)&sNodePool[11],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(33),
                           nextToken == 12,
                           12,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t32/t323", 12, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(34),
                            pNode == &sNodePool[12],
                            (uintptr_t)&sNodePool[12],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(35),
                           nextToken == 12,
                           12,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t32/t321/t3211", 18, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(36),
                            pNode == &sNodePool[13],
                            (uintptr_t)&sNodePool[13],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(37),
                           nextToken == 18,
                           18,
                           nextToken,
                           TEST_VFS_ENABLED);

  /* Valid partial path tests */
  pNode = pFindNode(sNodePool, "/t4", 3, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(38),
                            pNode == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(39),
                           nextToken == 1,
                           1,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t1/t11/t123", 12, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(40),
                            pNode == &sNodePool[4],
                            (uintptr_t)&sNodePool[4],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(41),
                           nextToken == 8,
                           8,
                           nextToken,
                           TEST_VFS_ENABLED);
  pNode = pFindNode(sNodePool, "/t3/t32/t321/t3211/sdf", 22, &nextToken);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FIND_NODE(42),
                            pNode == &sNodePool[13],
                            (uintptr_t)&sNodePool[13],
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FIND_NODE(43),
                           nextToken == 19,
                           19,
                           nextToken,
                           TEST_VFS_ENABLED);
}

void VFSAddNodeTest(void* pArgs)
{
  T_VFSAddDriverNode pAddDriverNode = (T_VFSAddDriverNode)pArgs;
  S_VFSNode* pNode;
  S_VFSNode* pSaveNode;
  int32_t compareValue;
  S_FSDriver driver;

  /* Create the test graph */
  memcpy(sNodePool[0].pMountPoint, "", 1);
  sNodePool[0].mountPointLength = 0;
  sNodePool[0].pDriver = (void*)&driver;
  sNodePool[0].pFirstChild = &sNodePool[1];
  sNodePool[0].pNextSibling = NULL;
  sNodePool[0].pParent = NULL;

  memcpy(sNodePool[1].pMountPoint, "t1", 3);
  sNodePool[1].mountPointLength = 2;
  sNodePool[1].pDriver = NULL;
  sNodePool[1].pFirstChild = &sNodePool[4];
  sNodePool[1].pNextSibling = &sNodePool[2];
  sNodePool[1].pParent = &sNodePool[0];

  memcpy(sNodePool[2].pMountPoint, "t2", 3);
  sNodePool[2].mountPointLength = 2;
  sNodePool[2].pDriver = (void*)&driver;
  sNodePool[2].pFirstChild = NULL;
  sNodePool[2].pNextSibling = &sNodePool[3];
  sNodePool[2].pParent = &sNodePool[0];

  memcpy(sNodePool[3].pMountPoint, "t3", 3);
  sNodePool[3].mountPointLength = 2;
  sNodePool[3].pDriver = (void*)&driver;
  sNodePool[3].pFirstChild = &sNodePool[6];
  sNodePool[3].pNextSibling = NULL;
  sNodePool[3].pParent = &sNodePool[0];

  memcpy(sNodePool[4].pMountPoint, "t11", 4);
  sNodePool[4].mountPointLength = 3;
  sNodePool[4].pDriver = NULL;
  sNodePool[4].pFirstChild = &sNodePool[8];
  sNodePool[4].pNextSibling = &sNodePool[5];
  sNodePool[4].pParent = &sNodePool[1];

  memcpy(sNodePool[5].pMountPoint, "t12", 5);
  sNodePool[5].mountPointLength = 3;
  sNodePool[5].pDriver = (void*)&driver;
  sNodePool[5].pFirstChild = NULL;
  sNodePool[5].pNextSibling = NULL;
  sNodePool[5].pParent = &sNodePool[1];

  memcpy(sNodePool[6].pMountPoint, "t31", 5);
  sNodePool[6].mountPointLength = 3;
  sNodePool[6].pDriver = NULL;
  sNodePool[6].pFirstChild = &sNodePool[9];
  sNodePool[6].pNextSibling = &sNodePool[7];
  sNodePool[6].pParent = &sNodePool[3];

  memcpy(sNodePool[7].pMountPoint, "t32", 5);
  sNodePool[7].mountPointLength = 3;
  sNodePool[7].pDriver = NULL;
  sNodePool[7].pFirstChild = &sNodePool[10];
  sNodePool[7].pNextSibling = NULL;
  sNodePool[7].pParent = &sNodePool[3];

  memcpy(sNodePool[8].pMountPoint, "t111", 5);
  sNodePool[8].mountPointLength = 4;
  sNodePool[8].pDriver = NULL;
  sNodePool[8].pFirstChild = NULL;
  sNodePool[8].pNextSibling = NULL;
  sNodePool[8].pParent = &sNodePool[4];

  memcpy(sNodePool[9].pMountPoint, "t311", 5);
  sNodePool[9].mountPointLength = 4;
  sNodePool[9].pDriver = (void*)&driver;
  sNodePool[9].pFirstChild = NULL;
  sNodePool[9].pNextSibling = NULL;
  sNodePool[9].pParent = &sNodePool[6];

  memcpy(sNodePool[10].pMountPoint, "t321", 5);
  sNodePool[10].mountPointLength = 4;
  sNodePool[10].pDriver = (void*)&driver;
  sNodePool[10].pFirstChild = &sNodePool[13];
  sNodePool[10].pNextSibling = &sNodePool[11];
  sNodePool[10].pParent = &sNodePool[7];

  memcpy(sNodePool[11].pMountPoint, "t322", 5);
  sNodePool[11].mountPointLength = 4;
  sNodePool[11].pDriver = NULL;
  sNodePool[11].pFirstChild = NULL;
  sNodePool[11].pNextSibling = &sNodePool[12];
  sNodePool[11].pParent = &sNodePool[7];

  memcpy(sNodePool[12].pMountPoint, "t323", 5);
  sNodePool[12].mountPointLength = 4;
  sNodePool[12].pDriver = NULL;
  sNodePool[12].pFirstChild = NULL;
  sNodePool[12].pNextSibling = NULL;
  sNodePool[12].pParent = &sNodePool[7];

  memcpy(sNodePool[13].pMountPoint, "t3211", 6);
  sNodePool[13].mountPointLength = 5;
  sNodePool[13].pDriver = (void*)0xDEADC0DE;
  sNodePool[13].pFirstChild = NULL;
  sNodePool[13].pNextSibling = NULL;
  sNodePool[13].pParent = &sNodePool[10];

  /* Add root node */
  pAddDriverNode(&sNodePool[0], "t4", 2, (void*)&driver);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(0),
                            sNodePool[3].pNextSibling != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)sNodePool[3].pNextSibling,
                            TEST_VFS_ENABLED);
  pNode = sNodePool[3].pNextSibling;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(1),
                            pNode->pDriver == (void*)&driver,
                            (uintptr_t)&driver,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(2),
                            pNode->pFirstChild == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(3),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(4),
                            pNode->pParent == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_ADD_NODE(5),
                           pNode->mountPointLength == 2,
                           2,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "t4");
  TEST_POINT_ASSERT_INT(TEST_VFS_ADD_NODE(6),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  pAddDriverNode(&sNodePool[0], "t0", 2, (void*)&driver);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(7),
                            sNodePool[0].pFirstChild != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)sNodePool[0].pFirstChild,
                            TEST_VFS_ENABLED);
  pNode = sNodePool[0].pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(8),
                            pNode->pDriver == (void*)&driver,
                            (uintptr_t)&driver,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(9),
                            pNode->pFirstChild == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(10),
                            pNode->pNextSibling == (void*)&sNodePool[1],
                            (uintptr_t)&sNodePool[1],
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(11),
                            pNode->pParent == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_ADD_NODE(12),
                           pNode->mountPointLength == 2,
                           2,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "t0");
  TEST_POINT_ASSERT_INT(TEST_VFS_ADD_NODE(13),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  /* Add intermediary node */
  pAddDriverNode(&sNodePool[0], "a/test/inter/end", 16, (void*)&driver);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(14),
                            sNodePool[0].pFirstChild != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)sNodePool[0].pFirstChild,
                            TEST_VFS_ENABLED);
  pSaveNode = pNode;
  pNode = sNodePool[0].pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(15),
                            pNode->pDriver == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(16),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(17),
                            pNode->pNextSibling == (void*)pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(18),
                            pNode->pParent == &sNodePool[0],
                            (uintptr_t)&sNodePool[0],
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_ADD_NODE(19),
                           pNode->mountPointLength == 1,
                           1,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "a");
  TEST_POINT_ASSERT_INT(TEST_VFS_ADD_NODE(20),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(21),
                            sNodePool[0].pFirstChild != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)sNodePool[0].pFirstChild,
                            TEST_VFS_ENABLED);
  pSaveNode = pNode;
  pNode = pNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(22),
                            pNode->pDriver == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(23),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(24),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(25),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_ADD_NODE(26),
                           pNode->mountPointLength == 4,
                           4,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "test");
  TEST_POINT_ASSERT_INT(TEST_VFS_ADD_NODE(27),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(28),
                            pNode->pFirstChild != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  pSaveNode = pNode;
  pNode = pNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(29),
                            pNode->pDriver == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(30),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(31),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(32),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_ADD_NODE(33),
                           pNode->mountPointLength == 5,
                           5,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "inter");
  TEST_POINT_ASSERT_INT(TEST_VFS_ADD_NODE(34),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  pSaveNode = pNode;
  pNode = pNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(35),
                            pNode->pDriver == (void*)&driver,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(36),
                            pNode->pFirstChild == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(37),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_ADD_NODE(38),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_ADD_NODE(39),
                           pNode->mountPointLength == 3,
                           3,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "end");
  TEST_POINT_ASSERT_INT(TEST_VFS_ADD_NODE(40),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);
}

void VFSRegDriverTest(void* pArgs)
{
  S_VFSNode*   pRoot = (S_VFSNode*)pArgs;
  S_FSDriver* pDriver;
  S_FSDriver* pNewDriver;
  S_VFSNode* pNode;
  S_VFSNode* pSaveNode;
  int32_t compareValue;

  /* Encure root is empty */
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(0),
                            pRoot != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pRoot,
                            TEST_VFS_ENABLED);

  /* Unclean path */
  pDriver = RegisterVFSDriver("///test//./one/../two/",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(1),
                            pDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pDriver,
                            TEST_VFS_ENABLED);

  pSaveNode = pRoot;
  pNode = pRoot;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(2),
                            pNode->pDriver == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(3),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(4),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(5),
                            pNode->pParent == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_REGISTER_DRIVER(6),
                           pNode->mountPointLength == 0,
                           0,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "");
  TEST_POINT_ASSERT_INT(TEST_VFS_REGISTER_DRIVER(7),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  pSaveNode = pNode;
  pNode = pSaveNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(8),
                            pNode->pDriver == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(9),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(10),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(11),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_REGISTER_DRIVER(12),
                           pNode->mountPointLength == 4,
                           4,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "test");
  TEST_POINT_ASSERT_INT(TEST_VFS_REGISTER_DRIVER(13),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  pSaveNode = pNode;
  pNode = pSaveNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(14),
                            pNode->pDriver == pDriver,
                            (uintptr_t)pDriver,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(15),
                            pDriver->pNode == pNode,
                            (uintptr_t)pNode,
                            (uintptr_t)pDriver->pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(16),
                            pNode->pFirstChild == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(17),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(18),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_REGISTER_DRIVER(19),
                           pNode->mountPointLength == 3,
                           3,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "two");
  TEST_POINT_ASSERT_INT(TEST_VFS_REGISTER_DRIVER(20),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(21),
                            pDriver->pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pDriver->pDriverData,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(22),
                            pDriver->pOpen == (void*)_DummyOpen,
                            (uintptr_t)_DummyOpen,
                            (uintptr_t)pDriver->pOpen,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(23),
                            pDriver->pClose == (void*)_DummyClose,
                            (uintptr_t)_DummyClose,
                            (uintptr_t)pDriver->pClose,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(24),
                            pDriver->pRead == (void*)_DummyRead,
                            (uintptr_t)_DummyRead,
                            (uintptr_t)pDriver->pRead,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(25),
                            pDriver->pWrite == (void*)_DummyWrite,
                            (uintptr_t)_DummyWrite,
                            (uintptr_t)pDriver->pWrite,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(26),
                            pDriver->pReadDir == (void*)_DummyReadDir,
                            (uintptr_t)_DummyReadDir,
                            (uintptr_t)pDriver->pReadDir,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(27),
                            pDriver->pIOCTL == (void*)_DummyIOCTL,
                            (uintptr_t)_DummyIOCTL,
                            (uintptr_t)pDriver->pIOCTL,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(28),
                            pDriver->pMount == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pDriver->pMount,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(29),
                            pDriver->pUnmount == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pDriver->pUnmount,
                            TEST_VFS_ENABLED);

  /* Valid path register non null over null driver node */
  pNewDriver = RegisterVFSDriver("/test",
                              (void*)0xC0DEAABB,
                              _DummyOpen,
                              _DummyClose,
                              _DummyRead,
                              _DummyWrite,
                              _DummyReadDir,
                              _DummyIOCTL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(30),
                            pNewDriver != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(31),
                            pNewDriver != pDriver,
                            (uintptr_t)pDriver,
                            (uintptr_t)pNewDriver,
                            TEST_VFS_ENABLED);

  pSaveNode = pRoot;
  pNode = pRoot;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(32),
                            pNode->pDriver == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(33),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(34),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(35),
                            pNode->pParent == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_REGISTER_DRIVER(36),
                           pNode->mountPointLength == 0,
                           0,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "");
  TEST_POINT_ASSERT_INT(TEST_VFS_REGISTER_DRIVER(37),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  pSaveNode = pNode;
  pNode = pSaveNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(38),
                            pNode->pDriver == pNewDriver,
                            (uintptr_t)pNewDriver,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(39),
                            pNewDriver->pNode == pNode,
                            (uintptr_t)pNode,
                            (uintptr_t)pNewDriver->pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(40),
                            pNode->pFirstChild != (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(41),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(42),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_REGISTER_DRIVER(43),
                           pNode->mountPointLength == 4,
                           4,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "test");
  TEST_POINT_ASSERT_INT(TEST_VFS_REGISTER_DRIVER(44),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  pSaveNode = pNode;
  pNode = pSaveNode->pFirstChild;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(45),
                            pNode->pDriver == pDriver,
                            (uintptr_t)pDriver,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(46),
                            pDriver->pNode == pNode,
                            (uintptr_t)pNode,
                            (uintptr_t)pDriver->pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(47),
                            pNode->pFirstChild == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(48),
                            pNode->pNextSibling == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pNextSibling,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(49),
                            pNode->pParent == pSaveNode,
                            (uintptr_t)pSaveNode,
                            (uintptr_t)pNode->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_REGISTER_DRIVER(50),
                           pNode->mountPointLength == 3,
                           3,
                           pNode->mountPointLength,
                           TEST_VFS_ENABLED);
  compareValue = strcmp(pNode->pMountPoint, "two");
  TEST_POINT_ASSERT_INT(TEST_VFS_REGISTER_DRIVER(51),
                        compareValue == 0,
                        0,
                        compareValue,
                        TEST_VFS_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(52),
                            pNewDriver->pDriverData == (void*)0xC0DEAABB,
                            (uintptr_t)0xC0DEAABB,
                            (uintptr_t)pNewDriver->pDriverData,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(53),
                            pNewDriver->pOpen == (void*)_DummyOpen,
                            (uintptr_t)_DummyOpen,
                            (uintptr_t)pNewDriver->pOpen,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(54),
                            pNewDriver->pClose == (void*)_DummyClose,
                            (uintptr_t)_DummyClose,
                            (uintptr_t)pNewDriver->pClose,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(55),
                            pNewDriver->pRead == (void*)_DummyRead,
                            (uintptr_t)_DummyRead,
                            (uintptr_t)pNewDriver->pRead,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(56),
                            pNewDriver->pWrite == (void*)_DummyWrite,
                            (uintptr_t)_DummyWrite,
                            (uintptr_t)pNewDriver->pWrite,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(57),
                            pNewDriver->pReadDir == (void*)_DummyReadDir,
                            (uintptr_t)_DummyReadDir,
                            (uintptr_t)pNewDriver->pReadDir,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(58),
                            pNewDriver->pIOCTL == (void*)_DummyIOCTL,
                            (uintptr_t)_DummyIOCTL,
                            (uintptr_t)pNewDriver->pIOCTL,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(59),
                            pNewDriver->pMount == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNewDriver->pMount,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(60),
                            pNewDriver->pUnmount == (void*)NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNewDriver->pUnmount,
                            TEST_VFS_ENABLED);

  /* Invalid path driver already registered */
  pNewDriver = RegisterVFSDriver("/test",
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(61),
                            pNewDriver == VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver,
                            TEST_VFS_ENABLED);
  pNewDriver = RegisterVFSDriver("/test/two",
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REGISTER_DRIVER(62),
                            pNewDriver == VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver,
                            TEST_VFS_ENABLED);
}

void VFSRemoveNodeTest(void* pArgs)
{
  T_RemoveDriverNode pRemoveNode;
  S_FSDriver driver;
  bool isCleaned;
  uint32_t i;

  S_VFSNode* pNodePool[16];

  for (i = 0; i < 16; ++i)
  {
    pNodePool[i] = KMalloc(sizeof(S_VFSNode), KMALLOC_FREE_POOL);
  }

  pRemoveNode = (T_RemoveDriverNode)pArgs;


  /* Create the test graph */
  memcpy(pNodePool[0]->pMountPoint, "", 1);
  pNodePool[0]->mountPointLength = 0;
  pNodePool[0]->pDriver = (void*)&driver;
  pNodePool[0]->pFirstChild = pNodePool[1];
  pNodePool[0]->pNextSibling = NULL;
  pNodePool[0]->pParent = NULL;

  memcpy(pNodePool[1]->pMountPoint, "t1", 3);
  pNodePool[1]->mountPointLength = 2;
  pNodePool[1]->pDriver = NULL;
  pNodePool[1]->pFirstChild = pNodePool[4];
  pNodePool[1]->pNextSibling = pNodePool[2];
  pNodePool[1]->pParent = pNodePool[0];

  memcpy(pNodePool[2]->pMountPoint, "t2", 3);
  pNodePool[2]->mountPointLength = 2;
  pNodePool[2]->pDriver = (void*)&driver;
  pNodePool[2]->pFirstChild = NULL;
  pNodePool[2]->pNextSibling = pNodePool[3];
  pNodePool[2]->pParent = pNodePool[0];

  memcpy(pNodePool[3]->pMountPoint, "t3", 3);
  pNodePool[3]->mountPointLength = 2;
  pNodePool[3]->pDriver = (void*)&driver;
  pNodePool[3]->pFirstChild = pNodePool[6];
  pNodePool[3]->pNextSibling = NULL;
  pNodePool[3]->pParent = pNodePool[0];

  memcpy(pNodePool[4]->pMountPoint, "t11", 4);
  pNodePool[4]->mountPointLength = 3;
  pNodePool[4]->pDriver = NULL;
  pNodePool[4]->pFirstChild = pNodePool[8];
  pNodePool[4]->pNextSibling = pNodePool[5];
  pNodePool[4]->pParent = pNodePool[1];

  memcpy(pNodePool[5]->pMountPoint, "t12", 5);
  pNodePool[5]->mountPointLength = 3;
  pNodePool[5]->pDriver = NULL;
  pNodePool[5]->pFirstChild = pNodePool[10];
  pNodePool[5]->pNextSibling = NULL;
  pNodePool[5]->pParent = pNodePool[1];

  memcpy(pNodePool[6]->pMountPoint, "t31", 5);
  pNodePool[6]->mountPointLength = 3;
  pNodePool[6]->pDriver = &driver;
  pNodePool[6]->pFirstChild = pNodePool[11];
  pNodePool[6]->pNextSibling = pNodePool[7];
  pNodePool[6]->pParent = pNodePool[3];

  memcpy(pNodePool[7]->pMountPoint, "t32", 5);
  pNodePool[7]->mountPointLength = 3;
  pNodePool[7]->pDriver = NULL;
  pNodePool[7]->pFirstChild = pNodePool[12];
  pNodePool[7]->pNextSibling = NULL;
  pNodePool[7]->pParent = pNodePool[3];

  memcpy(pNodePool[8]->pMountPoint, "t111", 5);
  pNodePool[8]->mountPointLength = 4;
  pNodePool[8]->pDriver = NULL;
  pNodePool[8]->pFirstChild = NULL;
  pNodePool[8]->pNextSibling = pNodePool[9];
  pNodePool[8]->pParent = pNodePool[4];

  memcpy(pNodePool[9]->pMountPoint, "t112", 5);
  pNodePool[9]->mountPointLength = 4;
  pNodePool[9]->pDriver = NULL;
  pNodePool[9]->pFirstChild = NULL;
  pNodePool[9]->pNextSibling = NULL;
  pNodePool[9]->pParent = pNodePool[4];

  memcpy(pNodePool[10]->pMountPoint, "t121", 5);
  pNodePool[10]->mountPointLength = 4;
  pNodePool[10]->pDriver = &driver;
  pNodePool[10]->pFirstChild = NULL;
  pNodePool[10]->pNextSibling = NULL;
  pNodePool[10]->pParent = pNodePool[5];

  memcpy(pNodePool[11]->pMountPoint, "t311", 5);
  pNodePool[11]->mountPointLength = 4;
  pNodePool[11]->pDriver = (void*)&driver;
  pNodePool[11]->pFirstChild = NULL;
  pNodePool[11]->pNextSibling = NULL;
  pNodePool[11]->pParent = pNodePool[6];

  memcpy(pNodePool[12]->pMountPoint, "t321", 5);
  pNodePool[12]->mountPointLength = 4;
  pNodePool[12]->pDriver = NULL;
  pNodePool[12]->pFirstChild = pNodePool[15];
  pNodePool[12]->pNextSibling = pNodePool[13];
  pNodePool[12]->pParent = pNodePool[7];

  memcpy(pNodePool[13]->pMountPoint, "t322", 5);
  pNodePool[13]->mountPointLength = 4;
  pNodePool[13]->pDriver = NULL;
  pNodePool[13]->pFirstChild = NULL;
  pNodePool[13]->pNextSibling = pNodePool[14];
  pNodePool[13]->pParent = pNodePool[7];

  memcpy(pNodePool[14]->pMountPoint, "t323", 5);
  pNodePool[14]->mountPointLength = 4;
  pNodePool[14]->pDriver = NULL;
  pNodePool[14]->pFirstChild = NULL;
  pNodePool[14]->pNextSibling = NULL;
  pNodePool[14]->pParent = pNodePool[7];

  memcpy(pNodePool[15]->pMountPoint, "t3211", 6);
  pNodePool[15]->mountPointLength = 5;
  pNodePool[15]->pDriver = (void*)0xDEADC0DE;
  pNodePool[15]->pFirstChild = NULL;
  pNodePool[15]->pNextSibling = NULL;
  pNodePool[15]->pParent = pNodePool[12];

  /* Remove leaf with driver T3211 */
  isCleaned = pRemoveNode(pNodePool[15]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(0),
                         isCleaned == false,
                         false,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(1),
                            pNodePool[12]->pFirstChild == pNodePool[15],
                            (uintptr_t)pNodePool[15],
                            (uintptr_t)pNodePool[12]->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(2),
                            pNodePool[12] == pNodePool[15]->pParent,
                            (uintptr_t)pNodePool[12],
                            (uintptr_t)pNodePool[15]->pParent,
                            TEST_VFS_ENABLED);

  /* Remove leaf without driver T111 */
  isCleaned = pRemoveNode(pNodePool[8]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(3),
                         isCleaned == true,
                         true,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(4),
                            pNodePool[4]->pFirstChild == pNodePool[9],
                            (uintptr_t)pNodePool[9],
                            (uintptr_t)pNodePool[4]->pFirstChild,
                            TEST_VFS_ENABLED);

  /* Remove intermediary with driver and no driver in the rest of the path T11 */
  isCleaned = pRemoveNode(pNodePool[4]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(5),
                         isCleaned == true,
                         true,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(6),
                            pNodePool[4]->pFirstChild == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNodePool[4]->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(7),
                            pNodePool[1]->pFirstChild == pNodePool[5],
                            (uintptr_t)pNodePool[5],
                            (uintptr_t)pNodePool[4]->pFirstChild,
                            TEST_VFS_ENABLED);

  /* Remove intermediary with driver and driver in the rest of the path T31 */
  isCleaned = pRemoveNode(pNodePool[6]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(8),
                         isCleaned == false,
                         false,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(9),
                            pNodePool[3]->pFirstChild == pNodePool[6],
                            (uintptr_t)pNodePool[6],
                            (uintptr_t)pNodePool[3]->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(10),
                            pNodePool[6]->pFirstChild == pNodePool[11],
                            (uintptr_t)pNodePool[11],
                            (uintptr_t)pNodePool[6]->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(11),
                            pNodePool[6]->pParent == pNodePool[3],
                            (uintptr_t)pNodePool[3],
                            (uintptr_t)pNodePool[6]->pParent,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(12),
                            pNodePool[11]->pParent == pNodePool[6],
                            (uintptr_t)pNodePool[6],
                            (uintptr_t)pNodePool[11]->pParent,
                            TEST_VFS_ENABLED);

  /* Remove intermediary with no driver and no driver in the rest of the path T32 */
  pNodePool[15]->pDriver = NULL;
  isCleaned = pRemoveNode(pNodePool[15]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(13),
                         isCleaned == true,
                         true,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(14),
                            pNodePool[12]->pFirstChild == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNodePool[12]->pFirstChild,
                            TEST_VFS_ENABLED);
  isCleaned = pRemoveNode(pNodePool[7]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(15),
                         isCleaned == true,
                         true,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(16),
                            pNodePool[3]->pFirstChild == pNodePool[6],
                            (uintptr_t)pNodePool[6],
                            (uintptr_t)pNodePool[3]->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(17),
                            pNodePool[6]->pNextSibling == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNodePool[6]->pNextSibling,
                            TEST_VFS_ENABLED);

  /* Remove intermediary with no driver and driver in the rest of the path T12 */
  isCleaned = pRemoveNode(pNodePool[5]);
  TEST_POINT_ASSERT_BYTE(TEST_VFS_REMOVE_NODE(18),
                         isCleaned == false,
                         false,
                         isCleaned,
                         TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(19),
                            pNodePool[5]->pFirstChild == pNodePool[10],
                            (uintptr_t)pNodePool[10],
                            (uintptr_t)pNodePool[5]->pFirstChild,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_NODE(20),
                            pNodePool[10]->pParent == pNodePool[5],
                            (uintptr_t)pNodePool[5],
                            (uintptr_t)pNodePool[10]->pParent,
                            TEST_VFS_ENABLED);

}

void VFSRemoveDriverTest(void* pArgs)
{
  S_FSDriver* pNewDriver[5];
  E_Return    retCode;
  S_VFSNode*  pRoot = (S_VFSNode*)pArgs;
  S_VFSNode*  pNode;

  pNewDriver[0] = RegisterVFSDriver("/testremove",
                                    (void*)0xCAFEBABE,
                                    NULL,
                                    NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(0),
                            pNewDriver[0] != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver[0],
                            TEST_VFS_ENABLED);
  pNewDriver[1] = RegisterVFSDriver("/testremove/new",
                                     (void*)0xDEADBEEF,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(1),
                            pNewDriver[1] != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver[1],
                            TEST_VFS_ENABLED);
  pNewDriver[2] = RegisterVFSDriver("/testremove/new/one",
                                 (void*)0x42424242,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(2),
                            pNewDriver[2] != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver[2],
                            TEST_VFS_ENABLED);
  pNewDriver[3] = RegisterVFSDriver("/testremove/two",
                                 (void*)0x05050505,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(3),
                            pNewDriver[3] != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver[3],
                            TEST_VFS_ENABLED);
  pNewDriver[4] = RegisterVFSDriver("/testremove2",
                                 (void*)0xA0A0A0A0,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(4),
                            pNewDriver[4] != VFS_DRIVER_INVALID,
                            (uintptr_t)VFS_DRIVER_INVALID,
                            (uintptr_t)pNewDriver[4],
                            TEST_VFS_ENABLED);

  pNode = pRoot->pFirstChild;
  while (pNode != NULL)
  {
    if (pNode->pDriver == pNewDriver[4])
    {
      break;
    }
    pNode = pNode->pNextSibling;
  }
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(5),
                            pNewDriver[4] == pNode->pDriver,
                            (uintptr_t)pNewDriver[4],
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  retCode = UnregisterDriver(pNewDriver[4]);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_REMOVE_DRIVER(6),
                            retCode == NO_ERROR,
                            (uintptr_t)NO_ERROR,
                            (uintptr_t)retCode,
                            TEST_VFS_ENABLED);
  pNode = pRoot->pFirstChild;
  while (pNode != NULL)
  {
    if (pNode->pDriver == pNewDriver[4])
    {
      break;
    }
    pNode = pNode->pNextSibling;
  }
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(7),
                            NULL == pNode,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);


  pNode = pRoot->pFirstChild;
  while (pNode != NULL)
  {
    if (strcmp(pNode->pMountPoint, "testremove") == 0)
    {
      break;
    }
    pNode = pNode->pNextSibling;
  }
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(8),
                            pNewDriver[0] == pNode->pDriver,
                            (uintptr_t)pNewDriver[0],
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(9),
                            pNewDriver[0]->pDriverData == (void*)0xCAFEBABE,
                            (uintptr_t)pNewDriver[0]->pDriverData,
                            (uintptr_t)0xCAFEBABE,
                            TEST_VFS_ENABLED);
  retCode = UnregisterDriver(pNewDriver[0]);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_REMOVE_DRIVER(10),
                            retCode == NO_ERROR,
                            (uintptr_t)NO_ERROR,
                            (uintptr_t)retCode,
                            TEST_VFS_ENABLED);
  pNode = pRoot->pFirstChild;
  while (pNode != NULL)
  {
    if (strcmp(pNode->pMountPoint, "testremove") == 0)
    {
      break;
    }
    pNode = pNode->pNextSibling;
  }
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(11),
                            NULL != pNode,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(12),
                            NULL == pNode->pDriver,
                            (uintptr_t)NULL,
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  pNode = pNode->pFirstChild;
  while (pNode != NULL)
  {
    if (pNode->pDriver == pNewDriver[1])
    {
      break;
    }
    pNode = pNode->pNextSibling;
  }
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(13),
                            pNewDriver[1] == pNode->pDriver,
                            (uintptr_t)pNewDriver[1],
                            (uintptr_t)pNode->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_REMOVE_DRIVER(14),
                            pNewDriver[1]->pDriverData == (void*)0xDEADBEEF,
                            (uintptr_t)pNewDriver[1]->pDriverData,
                            (uintptr_t)0xDEADBEEF,
                            TEST_VFS_ENABLED);
}

T_CreateFD pCreateFD;
void VFSCreateFDTest(void* pArgs)
{
  S_KernelProcess process;
  S_FDTable* pTable;
  E_Return retCode;
  int32_t fdId;
  S_FileDescriptor* pInternalFD;

  pCreateFD = (T_CreateFD)pArgs;

  memset(&process, 0, sizeof(process));

  retCode = CreateProcessHeap(&process.pHeap);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_CREATE(0),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  retCode = CreateProcessFDTable(&process);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_CREATE(1),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  pTable = (S_FDTable*)process.pFileDescriptorTable;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_CREATE(2),
                            pTable != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTable,
                            TEST_VFS_ENABLED);

  fdId = pCreateFD(pTable,
                   &driver0,
                   (void*)0x12345678,
                   "/test/file.txt",
                   0x5A,
                   0x3C);
  TEST_POINT_ASSERT_INT(TEST_VFS_FD_CREATE(3),
                        fdId == 0,
                        0,
                        fdId,
                        TEST_VFS_ENABLED);

  retCode = VectorGet(pTable->pFDTable, (size_t)fdId, (void**)&pInternalFD);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_CREATE(4),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_CREATE(5),
                            pInternalFD != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pInternalFD,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FD_CREATE(6),
                           pInternalFD->tableId == (uint32_t)fdId,
                           fdId,
                           pInternalFD->tableId,
                           TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_CREATE(7),
                            pInternalFD->pShared != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pInternalFD->pShared,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FD_CREATE(8),
                           pInternalFD->openFlags == 0x5A,
                           0x5A,
                           pInternalFD->openFlags,
                           TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_UDWORD(TEST_VFS_FD_CREATE(9),
                           pInternalFD->openMode == 0x3C,
                           0x3C,
                           pInternalFD->openMode,
                           TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_CREATE(10),
                            pInternalFD->pShared->pDriver == &driver0,
                            (uintptr_t)&driver0,
                            (uintptr_t)pInternalFD->pShared->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_CREATE(11),
                            pInternalFD->pShared->pFileHandle ==
                            (void*)0x12345678,
                            (uintptr_t)0x12345678,
                            (uintptr_t)pInternalFD->pShared->pFileHandle,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_VFS_FD_CREATE(12),
                        strcmp(pInternalFD->pShared->pFilePath,
                               "/test/file.txt") == 0,
                        0,
                        strcmp(pInternalFD->pShared->pFilePath,
                               "/test/file.txt"),
                        TEST_VFS_ENABLED);

  DestroyProcessFDTable(&process);
}

void VFSDestroyFDTest(void* pArgs)
{
  T_DestroyFD pDestroyFD = (T_DestroyFD)pArgs;
  S_KernelProcess process;
  S_FDTable* pTable;
  E_Return retCode;
  int32_t fdId;
  S_FileDescriptor* pInternalFD;

  memset(&process, 0, sizeof(process));


  retCode = CreateProcessHeap(&process.pHeap);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_DESTROY(0),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  retCode = CreateProcessFDTable(&process);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_DESTROY(1),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);

  pTable = (S_FDTable*)process.pFileDescriptorTable;
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_DESTROY(2),
                            pTable != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pTable,
                            TEST_VFS_ENABLED);

  fdId = (pCreateFD)(pTable,
                             &driver1,
                             (void*)0xAABBCCDD,
                             "/destroy/test",
                             0x11,
                             0x22);
  TEST_POINT_ASSERT_INT(TEST_VFS_FD_DESTROY(3),
                        fdId == 0,
                        0,
                        fdId,
                        TEST_VFS_ENABLED);

  pDestroyFD(pTable, fdId);

  retCode = VectorGet(pTable->pFDTable, (size_t)fdId, (void**)&pInternalFD);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_DESTROY(4),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_DESTROY(5),
                            pInternalFD == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pInternalFD,
                            TEST_VFS_ENABLED);

  fdId = (pCreateFD)(pTable,
                             &driver2,
                             (void*)0x11223344,
                             "/destroy/again",
                             0x33,
                             0x44);
  TEST_POINT_ASSERT_INT(TEST_VFS_FD_DESTROY(6),
                        fdId == 0,
                        0,
                        fdId,
                        TEST_VFS_ENABLED);

  retCode = VectorGet(pTable->pFDTable, (size_t)fdId, (void**)&pInternalFD);
  TEST_POINT_ASSERT_RCODE(TEST_VFS_FD_DESTROY(7),
                          retCode == NO_ERROR,
                          NO_ERROR,
                          retCode,
                          TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_DESTROY(8),
                            pInternalFD != NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pInternalFD,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_VFS_FD_DESTROY(9),
                            pInternalFD->pShared->pDriver == &driver2,
                            (uintptr_t)&driver2,
                            (uintptr_t)pInternalFD->pShared->pDriver,
                            TEST_VFS_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_VFS_FD_DESTROY(10),
                        strcmp(pInternalFD->pShared->pFilePath,
                               "/destroy/again") == 0,
                        0,
                        strcmp(pInternalFD->pShared->pFilePath,
                               "/destroy/again"),
                        TEST_VFS_ENABLED);

  DestroyProcessFDTable(&process);
}

void VirtualFSTest(void)
{
  _TestNextToken();
  _TestCreateFDTable();
  _TestDestroyFDTable();
  _TestVFSGeneric();
  _TestVFSOpen();
  _TestVFSClose();
  _TestVFSRead();
  _TestVFSWrite();
  _TestVFSReadDir();
  _TestVFSIOCTL();
  _TestVFSMount();
  _TestVFSUnmount();
  TEST_FRAMEWORK_END();
}

VFS_REG_FS(driver0);
VFS_REG_FS(driver1);
VFS_REG_FS(driver2);

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/