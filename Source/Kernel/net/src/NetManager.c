/*******************************************************************************
 * @file NetManager.c
 *
 * @see NetManager.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/10/2026
 *
 * @version 1.0
 *
 * @brief Network manager.
 *
 * @details Network manager. The functions of this module allows
 * to abtract the use of the network devices and their drivers. The network
 * manager provides a simple interface to send and receive data through the
 * network devices.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <IOCTL.h>
#include <Panic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <Memory.h>
#include <FastQueue.h>
#include <VirtualFS.h>
#include <Scheduler.h>
#include <KernelError.h>
#include <TimerManager.h>
#include <KernelSemaphore.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <NetManager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "NET MGR"

/** @brief Prefix for network interface paths. */
#define NET_MGR_IFACE_PATH_PREFIX "/dev/net/"
/** @brief Maximum length of a network interface path. */
#define NET_MGR_IFACE_PATH_MAX_LENGTH 32

/** @brief Priority for the kernel worker threads. */
#define KNETD_THREAD_PRIORITY 1
/** @brief Stack size for the kernel worker threads. */
#define KNETD_THREAD_STACK_SIZE 4096
/** @brief Size of the task queue. */
#define NET_MGR_TASK_QUEUE_SIZE 255

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Structure representing a kernel network worker task. */
typedef struct
{
  /** @brief Pointer to the task function. */
  T_NetManagerTask pTask;
  /** @brief Pointer to the argument for the task function. */
  void* pArg;
  /** @brief Pointer to the completion semaphore for the task. */
  S_KernelSemaphore* pCompletionSemaphore;
} S_KernelNetworkWorkerTask;

/** @brief Structure representing a kernel network worker periodic task. */
typedef struct
{
  /** @brief Handle for the timer associated with the periodic task. */
  T_TimerHandle timerHandle;
  /** @brief Pointer to the task function. */
  T_TimerPeriodicTask pTask;
  /** @brief Pointer to the argument for the task function. */
  void* pArg;
} S_KernelNetworkWorkerPeriodicTask;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the network manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the network manager to ensure correctness of
 * execution. Due to the critical nature of the network manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define NETMGR_ASSERT(COND, MSG, ERROR) {                \
  if ((COND) == false)                                   \
  {                                                      \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);        \
  }                                                      \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Get the next power of two for a given value.
 *
 * @param[in] kValue The value for which to get the next power of two.
 *
 * @return The next power of two.
 */
static size_t _GetNextPowerOfTwo(const size_t kValue);

/**
 * @brief Network VFS open hook.
 *
 * @details Network VFS open hook. This function returns a handle to control
 * the VFS network nodes.
 *
 * @param[in, out] pNetDevice The Network VFS driver.
 * @param[in] kpPath The path in the VFS mount point table.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _VFSOpen(void*       pNetDevice,
                      const char* kpPath,
                      int         flags,
                      int         mode);

/**
 * @brief Network VFS close hook.
 *
 * @details Network VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pNetDevice The Network VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _VFSClose(void* pNetDevice, void* pHandle);

/**
 * @brief Network VFS write hook.
 *
 * @details Network VFS write hook.
 *
 * @param[in, out] pNetDevice The Network VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _VFSWrite(void*       pNetDevice,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count);

/**
 * @brief Network VFS write hook.
 *
 * @details Network VFS write hook.
 *
 * @param[in, out] pNetDevice The Network VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _VFSRead(void*  pNetDevice,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count);

/**
 * @brief Network VFS IOCTL hook.
 *
 * @details Network VFS IOCTL hook. This function performs the IOCTL for the
 * Network driver.
 *
 * @param[in, out] pNetDevice The Network VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _VFSIOCTL(void*    pNetDevice,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs);

/**
 * @brief Kernel net worker thread routine.
 *
 * @details Kernel net worker thread routine. This function is executed in a
 * separate thread and handles the execution of tasks submitted to the kernel
 * worker.
 *
 * @param[in] pArgs Pointer to the arguments passed to the kernel worker thread,
 * usually NULL.
 *
 * @return This function does not return. It runs indefinitely until the kernel
 * worker thread is terminated.
 */
static void* _KernelNetThreadRoutine(void* pArgs);

/**
 * @brief Kernel net worker periodic task handler.
 *
 * @details Kernel net worker periodic task handler. This function is executed
 * in a timer ISR and handles the execution of periodic tasks submitted
 * to the kernel worker by submitting them to the kernel worker task queue.
 *
 * @param[in] pArg Pointer to the arguments passed to the periodic task, usually
 * NULL.
 *
 * @return This function does not return. It runs indefinitely until the kernel
 * worker thread is terminated.
 */
static void _NetManagerPeriodicTaskHandler(void* pArg);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Pointer to the task queue for the kernel worker. */
static S_FastQueue* spTaskQueue;
/** @brief Task semaphore for the kernel worker. */
static S_KernelSemaphore sTaskSemaphore;
/** @brief Task queue lock */
static S_KernelSpinlock sTaskQueueLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static size_t _GetNextPowerOfTwo(const size_t kValue)
{
  size_t power = 1;

  while (power < kValue)
  {
    power <<= 1;
  }

  return power;
}

static void* _VFSOpen(void*       pNetDevice,
                      const char* kpPath,
                      int         flags,
                      int         mode)
{
  void* pDesc;

  (void)mode;

  if (pNetDevice != NULL &&
     ((*kpPath == VFS_PATH_DELIMITER && *(kpPath + 1) == 0) || *kpPath == 0) &&
      (flags & O_RDWR) == O_RDWR)
  {
    pDesc = pNetDevice;
  }
  else
  {
    pDesc = (void*)-1;
  }

  return pDesc;
}

static int32_t _VFSClose(void* pNetDevice, void* pHandle)
{
  int32_t retVal;

  (void)pNetDevice;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    retVal = 0;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static ssize_t _VFSWrite(void*       pNetDevice,
                         void*       pHandle,
                         const void* kpBuffer,
                         size_t      count)
{
  ssize_t retVal;

  S_NetworkDevice* pNetDeviceInstance;

  (void)pHandle;

  /* TODO: Updae that function*/

  pNetDeviceInstance = (S_NetworkDevice*)pNetDevice;
  retVal = pNetDeviceInstance->operations.pSend(pNetDeviceInstance,
                                                kpBuffer,
                                                count,
                                                true,
                                                true);

  return retVal;
}

static ssize_t _VFSRead(void*  pNetDevice,
                        void*  pHandle,
                        void*  pBuffer,
                        size_t count)
{
  ssize_t retVal;

  (void)pNetDevice;
  (void)pHandle;
  (void)pBuffer;
  (void)count;

  retVal = -1;

  return retVal;
}

static ssize_t _VFSIOCTL(void*    pNetDevice,
                         void*    pHandle,
                         uint32_t operation,
                         void*    pArgs)
{
  ssize_t          retVal;
  S_NetworkDevice* pNetDeviceInstance;

  (void)pArgs;

  if (pHandle != NULL && pHandle != (void*)-1)
  {
    pNetDeviceInstance = (S_NetworkDevice*)pNetDevice;
    switch (operation)
    {
      case VFS_IOCTL_NET_IFACE_UP:
        retVal = pNetDeviceInstance->operations.pUplink(pNetDevice);
        if (retVal != NO_ERROR)
        {
          retVal = -1;
        }
        else
        {
          retVal = 0;
        }
        break;
      case VFS_IOCTL_NET_IFACE_DOWN:
        retVal = pNetDeviceInstance->operations.pDownlink(pNetDevice);
        if (retVal != NO_ERROR)
        {
          retVal = -1;
        }
        else
        {
          retVal = 0;
        }
        break;
      default:
        retVal = pNetDeviceInstance->operations.pControl(pNetDevice,
                                                         operation,
                                                         pArgs);
        break;
    }
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

static void* _KernelNetThreadRoutine(void* pArgs)
{
  E_Return                  retCode;
  S_KernelNetworkWorkerTask task;
  bool                      taskAvailable;

  (void)pArgs;

  while (true)
  {
    /* Wait for a task to be submitted */
    retCode = KernelSemaphoreWait(&sTaskSemaphore);
    NETMGR_ASSERT(retCode == NO_ERROR,
                   "Failed to wait for task semaphore.",
                   retCode);

    /* Get the task */
    KERNEL_LOCK(sTaskQueueLock);
    taskAvailable = FQueuePop(spTaskQueue, &task);
    KERNEL_UNLOCK(sTaskQueueLock);
    NETMGR_ASSERT(taskAvailable == true,
                  "Failed to dequeue task from the task queue.",
                  ERR_INVALID_VALUE);

    /* Execute the task */
    task.pTask(task.pArg);

    /* Signal task completion */
    if (task.pCompletionSemaphore != NULL)
    {
      retCode = KernelSemaphorePost(task.pCompletionSemaphore);
      NETMGR_ASSERT(retCode == NO_ERROR,
                     "Failed to signal task completion semaphore.",
                     retCode);
    }
  }

  /* Kernel worker thread is not allowed to return */
  PANIC(ERR_UNAUTHORIZED_ACTION,
        MODULE_NAME,
        "Kernel net worker thread returned",
        false,
        false);

  return NULL;
}

static void _NetManagerPeriodicTaskHandler(void* pArg)
{
  E_Return                           retCode;
  S_KernelNetworkWorkerPeriodicTask* pPeriodicTask;
  S_KernelNetworkWorkerTask          newTask;

  pPeriodicTask = (S_KernelNetworkWorkerPeriodicTask*)pArg;

  /* Create the task structure */
  newTask.pTask = pPeriodicTask->pTask;
  newTask.pArg = pPeriodicTask->pArg;
  newTask.pCompletionSemaphore = NULL;

  /* Enqueue the task */
  KERNEL_LOCK(sTaskQueueLock);
  FQueuePush(spTaskQueue, &newTask);
  KERNEL_UNLOCK(sTaskQueueLock);

  /* Signal the task semaphore */
  retCode = KernelSemaphorePost(&sTaskSemaphore);
  NETMGR_ASSERT(retCode == NO_ERROR,
                "Failed to signal task semaphore.",
                retCode);
}

void NetManagerInit(void)
{
  E_Return        retCode;
  uint32_t        cpuCount;
  uint32_t        i;
  S_KernelThread* pThread;
  char            name[THREAD_NAME_MAX_LENGTH];
  S_CPUMask       cpuMask;

  /* Spawn kernel worker threads */
  cpuCount = CPUGetCount();
  for (i = 0; i < cpuCount; ++i)
  {
    snprintf(name, THREAD_NAME_MAX_LENGTH, "knetd-%u", i);
    CPU_MASK_RESET(cpuMask);
    CPU_MASK_SET(cpuMask, i);
    /* Create the kernel worker thread */
    retCode = CreateThread(&pThread,
                           true,
                           KNETD_THREAD_PRIORITY,
                           name,
                           KNETD_THREAD_STACK_SIZE,
                           cpuMask,
                           _KernelNetThreadRoutine,
                           NULL,
                           NULL);
    NETMGR_ASSERT(retCode == NO_ERROR,
                   "Failed to create kernel net worker thread.",
                   retCode);
  }

  /* Initialize the task queue */
  spTaskQueue = FQueueCreate(NET_MGR_TASK_QUEUE_SIZE,
                             sizeof(S_KernelNetworkWorkerTask));
  NETMGR_ASSERT(spTaskQueue != NULL,
                "Failed to create task queue.",
                ERR_NO_MEMORY);

  /* Initialize the task semaphore */
  retCode = KernelSemaphoreInit(&sTaskSemaphore,
                                KSEMAPHORE_FLAG_QUEUING_FIFO,
                                0);
  NETMGR_ASSERT(retCode == NO_ERROR,
                 "Failed to initialize task semaphore.",
                 retCode);
}

E_Return NetManagerCreateBufferPool(const size_t     kBufferSize,
                                    const size_t     kBufferCount,
                                    const uint32_t   kAlignment,
                                    S_NetworkDevice* pDevice)
{
  E_Return              retCode;
  size_t                totalMemorySize;
  size_t                bufferAlignedSize;
  size_t                buffersToAllocate;
  void*                 pBuffer;
  uint32_t              i;
  S_NetworkPacketBuffer packetBuffer;

  /* Make sure to have a power of 2 buffer count */
  buffersToAllocate = kBufferCount * 2 - 1;
  buffersToAllocate = _GetNextPowerOfTwo(buffersToAllocate);

  /* Calculate the total memory size needed for the buffer pool */
  bufferAlignedSize = (kBufferSize + kAlignment - 1) & ~(kAlignment - 1);
  totalMemorySize   = buffersToAllocate * bufferAlignedSize;
  totalMemorySize   = ALIGN_UP(totalMemorySize , KERNEL_PAGE_SIZE);

  /* Allocate the buffer memory */
  pBuffer = MemoryKernelAllocate(totalMemorySize,
                                  MEMMGR_MAP_KERNEL |
                                  MEMMGR_MAP_RW     |
                                  MEMMGR_MAP_CACHE_DISABLED,
                                  &retCode);
  if (retCode == NO_ERROR)
  {
    /* Setup the memory pool for the driver */
    pDevice->pMemoryPool    = pBuffer;
    pDevice->memoryPoolSize = totalMemorySize;
    pDevice->bufferSize     = bufferAlignedSize;

    /* Setup the buffers for the driver */
    pDevice->pBufferPool    = pBuffer;
    pDevice->bufferPoolSize = bufferAlignedSize * kBufferCount;
    pDevice->bufferCount    = kBufferCount;

    /* Setup the buffer for the manager */
    pBuffer = (void*)((uintptr_t)pBuffer + pDevice->bufferPoolSize);
    pDevice->pReservedBufferPool    = pBuffer;
    pDevice->reservedBufferPoolSize = bufferAlignedSize * (kBufferCount - 1);
    pDevice->reservedBufferCount    = kBufferCount - 1;

    /* Initialize the free buffer queue */
    pDevice->pFreeBufferQueue = FQueueCreate(pDevice->reservedBufferCount,
                                             sizeof(S_NetworkPacketBuffer));

    for (i = 0; i < pDevice->reservedBufferCount; ++i)
    {
      packetBuffer.virtAddr = (uintptr_t)pDevice->pReservedBufferPool +
                                         i * pDevice->bufferSize;
      packetBuffer.physAddr = MemoryMgrGetPhysAddr(packetBuffer.virtAddr,
                                                   NULL,
                                                   NULL);
      FQueuePush(pDevice->pFreeBufferQueue, &packetBuffer);
    }
  }

  return retCode;
}

E_Return NetManagerReleaseBufferPool(S_NetworkDevice* pDevice)
{
  E_Return retCode;
  /* TODO:
   * Reclaim all buffers (free and in-use) and free the memory allocated for the
   * buffer pool.
   */
  retCode = MemoryKernelFree(pDevice->pMemoryPool, pDevice->memoryPoolSize);
  NETMGR_ASSERT(retCode == NO_ERROR,
                "Failed to free buffer pool memory",
                retCode);

  FQueueDestroy(pDevice->pFreeBufferQueue);

  return retCode;
}

E_Return NetManagerRegisterDevice(T_NetUpLink      pUplink,
                                  T_NetDownLink    pDownlink,
                                  T_NetSend        pSend,
                                  T_NetControl     pCtrl,
                                  const uint8_t    kBus,
                                  const uint8_t    kDevice,
                                  const uint8_t    kFunction,
                                  void*            pDriverData,
                                  S_NetworkDevice* pDevice)
{
  E_Return retCode;
  char     name[NET_MGR_IFACE_NAME_MAX_LENGTH + NET_MGR_IFACE_PATH_MAX_LENGTH];

  if (pUplink != NULL &&
      pDownlink != NULL &&
      pSend != NULL &&
      pCtrl != NULL &&
      pDevice != NULL)
  {
    pDevice->pDriverData          = pDriverData;
    pDevice->operations.pUplink   = pUplink;
    pDevice->operations.pDownlink = pDownlink;
    pDevice->operations.pSend     = pSend;
    pDevice->operations.pControl  = pCtrl;
    pDevice->pFreeBufferQueue     = NULL;

    /* Setup the device name */
    if (kFunction == 0xFF)
    {
      snprintf(pDevice->pName,
               NET_MGR_IFACE_NAME_MAX_LENGTH,
               "enp%ds%d",
               kBus,
               kDevice);
    }
    else
    {
      snprintf(pDevice->pName,
               NET_MGR_IFACE_NAME_MAX_LENGTH,
               "enp%ds%df%d",
               kBus,
               kDevice,
               kFunction);
    }
    snprintf(name,
             NET_MGR_IFACE_NAME_MAX_LENGTH + NET_MGR_IFACE_PATH_MAX_LENGTH,
             NET_MGR_IFACE_PATH_PREFIX "%s",
             pDevice->pName);

    /* Create the device entry in the network manager */
    pDevice->fsDriver = RegisterVFSDriver(name,
                                          pDevice,
                                          _VFSOpen,
                                          _VFSClose,
                                          _VFSRead,
                                          _VFSWrite,
                                          NULL,
                                          _VFSIOCTL);

    if (pDevice->fsDriver != VFS_DRIVER_INVALID)
    {
      retCode = NO_ERROR;
    }
    else
    {
      FQueueDestroy(pDevice->pFreeBufferQueue);
      retCode = ERR_UNAUTHORIZED_ACTION;
    }
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

S_NetworkFragmentList* NetManagerAllocateFragmentList(void)
{
  S_NetworkFragmentList* pFragmentList;

  pFragmentList = KMalloc(sizeof(S_NetworkFragmentList), KMALLOC_FREE_POOL);

  return pFragmentList;
}

#include <KernelOutput.h>
void NetManagerProcessReceivedPacket(S_NetworkFragmentList* pFragmentList,
                                     S_NetworkDevice*       pDevice)
{
  /* Go through each fragment and process it */
  for (uint32_t i = 0; i < pFragmentList->fragmentCount; i++)
  {
    KERNEL_DEBUG(NET_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Received packet fragment %d/%d, size: %zu bytes",
                 i + 1,
                 pFragmentList->fragmentCount,
                 pFragmentList->pFragments[i].size);
  }

  NetManagerFreeFragmentList(pFragmentList, pDevice);
}

void NetManagerFreeFragmentList(S_NetworkFragmentList* pFragmentList,
                                S_NetworkDevice*       pDevice)
{
  uint32_t i;

  /* Add back the free buffers for the network device */
  for (i = 0; i < pFragmentList->fragmentCount; ++i)
  {
    /* Release the fragment */
    FQueuePush(pDevice->pFreeBufferQueue, &pFragmentList->pFragments[i].buffer);
  }

  /* Free the fragment list */
  KFree(pFragmentList);
}

void NetManagerSubmitTask(T_NetManagerTask   task,
                          void*              pArg,
                          S_KernelSemaphore* pCompletionSemaphore)
{
  S_KernelNetworkWorkerTask newTask;
  E_Return                  retCode;

  /* Create the task structure */
  newTask.pTask = task;
  newTask.pArg = pArg;
  newTask.pCompletionSemaphore = pCompletionSemaphore;

  /* Enqueue the task */
  KERNEL_LOCK(sTaskQueueLock);
  FQueuePush(spTaskQueue, &newTask);
  KERNEL_UNLOCK(sTaskQueueLock);

  /* Signal the task semaphore */
  retCode = KernelSemaphorePost(&sTaskSemaphore);
  NETMGR_ASSERT(retCode == NO_ERROR,
                "Failed to signal task semaphore.",
                retCode);
}

void NetManagerReleaseTransmitBuffer(void*        pBufferAddress,
                                     const size_t kBufferSize)
{
  /* TODO: Implement the release of the transmit buffer */
  (void)pBufferAddress;
  (void)kBufferSize;
}

T_NetManagerTaskHandle NetManagerAddPeriodicTask(T_NetManagerTask task,
                                                 void*            pArg,
                                                 const uint32_t   kPeriodNs)
{
  S_TimerPeriodicTask*               pTaskHandle;
  S_KernelNetworkWorkerPeriodicTask* pPeriodicTask;

  pPeriodicTask = KMalloc(sizeof(S_KernelNetworkWorkerPeriodicTask),
                           KMALLOC_FREE_POOL);
  pPeriodicTask->pTask = task;
  pPeriodicTask->pArg = pArg;

  /* Create the task */
  pTaskHandle = KMalloc(sizeof(S_TimerPeriodicTask), KMALLOC_FREE_POOL);
  pTaskHandle->pTask = _NetManagerPeriodicTaskHandler;
  pTaskHandle->pArg = pPeriodicTask;

  /* Add the task to the periodic task list */
  pPeriodicTask->timerHandle  = TimeManagerAddTask(pTaskHandle, kPeriodNs);

  return pTaskHandle;
}

void NetManagerRemovePeriodicTask(T_NetManagerTaskHandle taskHandle)
{
  S_TimerPeriodicTask*               pTaskHandle;
  S_KernelNetworkWorkerPeriodicTask* pPeriodicTask;

  pTaskHandle = (S_TimerPeriodicTask*)taskHandle;
  pPeriodicTask = (S_KernelNetworkWorkerPeriodicTask*)pTaskHandle->pArg;

  /* Remove the task from the periodic task list */
  TimeManagerRemoveTask(pPeriodicTask->timerHandle);

  /* Free the task handle */
  KFree(pPeriodicTask);
  KFree(pTaskHandle);
}

/************************************ EOF *************************************/