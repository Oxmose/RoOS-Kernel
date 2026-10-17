/*******************************************************************************
 * @file StorageTest.c
 *
 * @brief SATA storage integration tests.
 *
 * @details These tests exercise the DeviceManager, PCIe, AHCI and disk
 * manager through their public VFS interface.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

#include <IOCTL.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <Critical.h>
#include <Memory.h>
#include <KernelHeap.h>
#include <Scheduler.h>
#include <VirtualFS.h>

#include <TestFramework.h>

#define STORAGE_TEST_ROOT "/dev/storage"
#define STORAGE_TEST_DEVICE_COUNT 2
#define STORAGE_TEST_THREAD_COUNT 4
#define STORAGE_TEST_TRANSFER_SIZE (128 * 512)

static volatile uint32_t sStorageThreadsReady;
static volatile uint32_t sStorageThreadsTarget;
static uint8_t sStorageThreadBuffers[STORAGE_TEST_THREAD_COUNT]
                                    [STORAGE_TEST_TRANSFER_SIZE];

typedef struct
{
  uint32_t index;
  ssize_t readSize;
  int32_t openResult;
} S_StorageThreadContext;

static const char* skStorageDevices[STORAGE_TEST_DEVICE_COUNT] =
{
  "sda",
  "sdb"
};

static const char* skStoragePartitions[] =
{
  "sda1", "sda2", "sda4",
  "sdb1", "sdb2", "sdb4", "sdb5", "sdb6", "sdb7"
};

static const uint32_t skStoragePartitionSectorCounts[] =
{
  1024, 256, 2048,
  1024, 256, 2048, 2048, 256, 512
};

static void _TestStorageDevices(void)
{
  S_DirectoryEntry entry;
  bool foundSda;
  bool foundSdb;
  uint32_t i;
  int32_t fd;
  int32_t retVal;

  foundSda = false;
  foundSdb = false;
  fd = VFSOpen(STORAGE_TEST_ROOT, O_RDONLY, 0);
  TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(0), fd >= 0, 1, fd,
                        TEST_STORAGE_ENABLED);
  if (fd < 0)
  {
    return;
  }

  while ((retVal = VFSReaddir(fd, &entry)) == 1)
  {
    if (strcmp(entry.pName, "sda") == 0)
    {
      foundSda = true;
    }
    else if (strcmp(entry.pName, "sdb") == 0)
    {
      foundSdb = true;
    }
  }

  TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(1), retVal == 0, 0, retVal,
                        TEST_STORAGE_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(2), foundSda, 1, foundSda,
                        TEST_STORAGE_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(3), foundSdb, 1, foundSdb,
                        TEST_STORAGE_ENABLED);
  TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(4), VFSClose(fd) == 0, 1, 0,
                        TEST_STORAGE_ENABLED);

  for (i = 0; i < STORAGE_TEST_DEVICE_COUNT; ++i)
  {
    char path[32];

    snprintf(path, sizeof(path), "%s/%s", STORAGE_TEST_ROOT,
             skStorageDevices[i]);
    fd = VFSOpen(path, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(5 + i), fd >= 0, 1, fd,
                          TEST_STORAGE_ENABLED);
    if (fd >= 0)
    {
      TEST_POINT_ASSERT_INT(TEST_STORAGE_DEVICE(7 + i),
                            VFSClose(fd) == 0, 1, 0,
                            TEST_STORAGE_ENABLED);
    }
  }
}

static void _TestStoragePartitions(void)
{
  uint32_t i;
  int32_t fd;
  char path[32];

  for (i = 0; i < sizeof(skStoragePartitions) /
                  sizeof(skStoragePartitions[0]); ++i)
  {
    snprintf(path, sizeof(path), "%s/%s", STORAGE_TEST_ROOT,
             skStoragePartitions[i]);
    fd = VFSOpen(path, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_PARTITION(i), fd >= 0, 1, fd,
                          TEST_STORAGE_ENABLED);
    if (fd >= 0)
    {
      TEST_POINT_ASSERT_INT(TEST_STORAGE_PARTITION_CLOSE(i),
                            VFSClose(fd) == 0, 1, 0,
                            TEST_STORAGE_ENABLED);
    }
  }
}

static void _TestStoragePartitionContent(void)
{
  static const char* const prefixes[] =
  {
    "PARTITION 0 | MBR", "PARTITION 1 | MBR", "PARTITION 3 | MBR",
    "PARTITION 0 | GPT", "PARTITION 1 | GPT", "PARTITION 3 | GPT",
    "PARTITION 4 | GPT", "PARTITION 5 | GPT", "PARTITION 6 | GPT"
  };
  char path[32];
  char buffer[32];
  uint32_t i;
  size_t prefixLength;
  ssize_t readSize;
  int32_t fd;

  for (i = 0; i < sizeof(skStoragePartitions) /
                  sizeof(skStoragePartitions[0]); ++i)
  {
    prefixLength = strlen(prefixes[i]);
    snprintf(path, sizeof(path), "%s/%s", STORAGE_TEST_ROOT,
             skStoragePartitions[i]);
    fd = VFSOpen(path, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_CONTENT_OPEN(i), fd >= 0, 1, fd,
                          TEST_STORAGE_ENABLED);
    if (fd < 0)
    {
      continue;
    }

    memset(buffer, 0, sizeof(buffer));
    readSize = VFSRead(fd, buffer, prefixLength);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_CONTENT_READ(i),
                          readSize == (ssize_t)prefixLength,
                          prefixLength, readSize, TEST_STORAGE_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_CONTENT_MATCH(i),
                          readSize == (ssize_t)prefixLength &&
                          memcmp(buffer, prefixes[i], prefixLength) == 0,
                          0, readSize == (ssize_t)prefixLength ?
                          memcmp(buffer, prefixes[i], prefixLength) : -1,
                          TEST_STORAGE_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_CONTENT_CLOSE(i),
                          VFSClose(fd) == 0, 1, 0, TEST_STORAGE_ENABLED);
  }
}

static void _TestStoragePartitionBoundary(void)
{
  uint8_t* pBuffer;
  uint8_t byte;
  uint32_t i;
  size_t partitionSize;
  ssize_t readSize;
  int32_t fd;
  E_Return error;
  char path[32];

  pBuffer = MemoryKernelAllocate(2048 * 512, MEMMGR_MAP_KERNEL | MEMMGR_MAP_RW, &error);
  TEST_POINT_ASSERT_RCODE(TEST_STORAGE_BOUNDARY_ALLOC,
                            error == NO_ERROR, NO_ERROR, error,
                            TEST_STORAGE_ENABLED);


  for (i = 0; i < sizeof(skStoragePartitions) /
                  sizeof(skStoragePartitions[0]); ++i)
  {
    partitionSize = (size_t)skStoragePartitionSectorCounts[i] * 512;
    snprintf(path, sizeof(path), "%s/%s", STORAGE_TEST_ROOT,
             skStoragePartitions[i]);
    fd = VFSOpen(path, O_RDONLY, 0);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_BOUNDARY_OPEN(i),
                          fd >= 0, 1, fd, TEST_STORAGE_ENABLED);
    if (fd < 0)
    {
      continue;
    }

    readSize = VFSRead(fd, pBuffer, partitionSize);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_BOUNDARY_READ(i),
                          readSize == (ssize_t)partitionSize,
                          partitionSize, readSize, TEST_STORAGE_ENABLED);
    readSize = VFSRead(fd, &byte, sizeof(byte));
    TEST_POINT_ASSERT_INT(TEST_STORAGE_BOUNDARY_END(i),
                          readSize == -1, -1, readSize,
                          TEST_STORAGE_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_BOUNDARY_CLOSE(i),
                          VFSClose(fd) == 0, 1, 0, TEST_STORAGE_ENABLED);
  }
}

static void* _StorageConcurrentRead(void* pArgs)
{
  S_StorageThreadContext* pContext;
  int32_t fd;

  pContext = pArgs;
  fd = VFSOpen(STORAGE_TEST_ROOT "/sdb", O_RDONLY, 0);
  pContext->openResult = fd;
  if (fd < 0)
  {
    AtomicIncrement32(&sStorageThreadsReady);
    return NULL;
  }

  AtomicIncrement32(&sStorageThreadsReady);
  while (sStorageThreadsReady < sStorageThreadsTarget)
  {
    SleepNs(1000000, NULL);
  }

  pContext->readSize = VFSRead(fd,
                               sStorageThreadBuffers[pContext->index],
                               STORAGE_TEST_TRANSFER_SIZE);
  VFSClose(fd);
  return NULL;
}

static void _TestStorageConcurrency(void)
{
  S_KernelThread* threads[STORAGE_TEST_THREAD_COUNT];
  S_StorageThreadContext contexts[STORAGE_TEST_THREAD_COUNT];
  S_CPUMask cpuMask;
  void* returnValue;
  E_Return error;
  uint32_t i;
  uint32_t createdCount;
  char name[THREAD_NAME_MAX_LENGTH] = "SATA_READ";

  sStorageThreadsReady = 0;
  sStorageThreadsTarget = STORAGE_TEST_THREAD_COUNT;
  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  for (i = 0; i < STORAGE_TEST_THREAD_COUNT; ++i)
  {
    contexts[i].index = i;
    contexts[i].readSize = -1;
    contexts[i].openResult = -1;
    error = CreateThread(&threads[i], true, 1, name,
                         0x1000, cpuMask, _StorageConcurrentRead,
                         &contexts[i], NULL);
    TEST_POINT_ASSERT_RCODE(TEST_STORAGE_THREAD_CREATE(i),
                            error == NO_ERROR, NO_ERROR, error,
                            TEST_STORAGE_ENABLED);
    if (error != NO_ERROR)
    {
      sStorageThreadsTarget = sStorageThreadsReady;
      break;
    }
  }
  createdCount = i;

  for (uint32_t joinIndex = 0; joinIndex < createdCount; ++joinIndex)
  {
    error = JoinThread(threads[joinIndex], &returnValue);
    TEST_POINT_ASSERT_RCODE(TEST_STORAGE_THREAD_JOIN(joinIndex),
                            error == NO_ERROR, NO_ERROR, error,
                            TEST_STORAGE_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_THREAD_OPEN(joinIndex),
                          contexts[joinIndex].openResult >= 0, 1,
                          contexts[joinIndex].openResult,
                          TEST_STORAGE_ENABLED);
    TEST_POINT_ASSERT_INT(TEST_STORAGE_THREAD_READ(joinIndex),
                          contexts[joinIndex].readSize ==
                          STORAGE_TEST_TRANSFER_SIZE,
                          STORAGE_TEST_TRANSFER_SIZE,
                          contexts[joinIndex].readSize,
                          TEST_STORAGE_ENABLED);
  }

  for (i = 1; i < createdCount; ++i)
  {
    TEST_POINT_ASSERT_INT(TEST_STORAGE_THREAD_DATA(i),
                          memcmp(sStorageThreadBuffers[0],
                                 sStorageThreadBuffers[i],
                                 STORAGE_TEST_TRANSFER_SIZE) == 0,
                          0,
                          memcmp(sStorageThreadBuffers[0],
                                 sStorageThreadBuffers[i],
                                 STORAGE_TEST_TRANSFER_SIZE),
                          TEST_STORAGE_ENABLED);
  }
}

static void* StorageTestEntry(void* args)
{
  (void)args;

  /* Wait for the storage devices to be initialized */
  SleepNs(1000000000, NULL);


  _TestStorageDevices();
  _TestStoragePartitions();
  _TestStoragePartitionContent();
  _TestStorageConcurrency();
  (void)_TestStorageDevices;
  (void)_TestStoragePartitions;
  (void)_TestStoragePartitionContent;
  (void)_TestStorageConcurrency;
  _TestStoragePartitionBoundary();
  TEST_FRAMEWORK_END();

  return NULL;
}

void StorageTest(void)
{
  E_Return        error;
  S_CPUMask       cpuMask;
  char            name[32] = "TEST_THREAD_VALID\0";
  S_KernelThread* pTestThread;

  CPU_MASK_RESET(cpuMask);
  CPU_MASK_SET(cpuMask, 0);

  /* Create the test thread */
  pTestThread = NULL;
  error = CreateThread(&pTestThread,
                       true,
                       0,
                       name,
                       0x1000,
                       cpuMask,
                       StorageTestEntry,
                       (void*)0xDEADC0DEBEEF0000ULL,
                         NULL);
  TEST_POINT_ASSERT_RCODE(STORAGE_CREATE_TEST_THREAD_ID(0),
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_STORAGE_ENABLED);
  TEST_POINT_ASSERT_POINTER(STORAGE_CREATE_TEST_THREAD_ID(1),
                            pTestThread != NULL,
                            0xFFFFFFFFFFFFFFFFULL,
                            (uintptr_t)pTestThread,
                            TEST_STORAGE_ENABLED);
}

#endif
