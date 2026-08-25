/*******************************************************************************
 * @file CoreProcessFS.c
 *
 * @see CoreProcessFS.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/08/2026
 *
 * @version 1.0
 *
 * @brief Kernel's processes PROCFS entries manager.
 *
 * @details Kernel's processes PROCFS entries manager. This module manages the
 * PROCFS entries for the kernel's processes. It allows to read the information
 * from the kernel's processes and write it to the buffer given as parameter.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <Panic.h>
#include <stdlib.h>
#include <ProcFS.h>
#include <Memory.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <Scheduler.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <CoreProcessFS.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "COREPROCESSFS"

/** @brief Length of the buffer used to store process status information */
#define PROCESS_STATUS_LENGTH 1024

/** @brief Maximum length of a PROCFS entry name */
#define PROC_FS_ENTRY_NAME_MAX_LENGTH 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Structure representing a process file handle */
typedef struct
{
  /** @brief The process for which the entry is opened */
  S_KernelProcess* pProcess;
  /** @brief The offset of the cursor in the file */
  size_t offset;
} S_ProcessFileHandle;

/** @brief Structure representing a thread file handle */
typedef struct
{
  /** @brief The thread for which the entry is opened */
  S_KernelThread* pThread;
  /** @brief The offset of the cursor in the file */
  size_t offset;
} S_ThreadFileHandle;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the kernel queues to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the kernel queues to ensure correctness of
 * execution. Due to the critical nature of the kernel queues, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define COREPROCESSFS_ASSERT(COND, MSG, ERROR) {         \
  if ((COND) == false)                                   \
  {                                                      \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);        \
  }                                                      \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Opens the ProcFS process entry.
 *
 * @details Opens the ProcFS process entry. This function will open the
 * ProcFS process entry and return a pointer to the file handle. The file
 * handle will be used to read the information from the kernel.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] kpPath The path to the ProcFS entry.
 * @param[in] flags The flags for opening the file.
 * @param[in] mode The mode for opening the file.
 *
 * @return The pointer to the file handle or -1 in case of error.
 */
static void* _ProcFSGenericOpen(void*       pExtraData,
                                const char* kpPath,
                                int32_t     flags,
                                int32_t     mode);

/**
 * @brief Closes the ProcFS process entry.
 *
 * @details Closes the ProcFS process entry. This function will close the
 * ProcFS process entry and free the resources used by the entry.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSGenericClose(void* pExtraData, void* pFileHandle);

/**
 * @brief Read the ProcFS process status entry.
 *
 * @details Read the ProcFS process status entry. This function will read the
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSStatusRead(void*  pExtraData,
                                 void*  pFileHandle,
                                 void*  pBuffer,
                                 size_t count);

/**
 * @brief Read the ProcFS scheduling statistics entry.
 *
 * @details Read the ProcFS scheduling statistics entry. This function will read
 * the information from the kernel and write it to the buffer given as
 * parameter. The function will return the number of bytes read or an error code.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSSchedStatsRead(void*  pExtraData,
                                     void*  pFileHandle,
                                     void*  pBuffer,
                                     size_t count);

/**
 * @brief Read the ProcFS memory map entry.
 *
 * @details Read the ProcFS memory map entry. This function will read the
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSMemoryMapRead(void*  pExtraData,
                                    void*  pFileHandle,
                                    void*  pBuffer,
                                    size_t count);

/**
 * @brief Read the ProcFS file descriptor entry.
 *
 * @details Read the ProcFS file descriptor entry. This function will read the
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSFileDescriptorsRead(void*  pExtraData,
                                          void*  pFileHandle,
                                          void*  pBuffer,
                                          size_t count);

/**
 * @brief Opens the ProcFS thread entry.
 *
 * @details Opens the ProcFS thread entry. This function will open the
 * ProcFS thread entry and return a pointer to the file handle. The file
 * handle will be used to read the information from the kernel.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] kpPath The path to the ProcFS entry.
 * @param[in] flags The flags for opening the file.
 * @param[in] mode The mode for opening the file.
 *
 * @return The pointer to the file handle or -1 in case of error.
 */
static void* _ProcFSThreadGenericOpen(void*       pExtraData,
                                     const char* kpPath,
                                     int32_t     flags,
                                     int32_t     mode);

/**
 * @brief Closes the ProcFS thread entry.
 *
 * @details Closes the ProcFS thread entry. This function will close the
 * ProcFS thread entry and free the resources used by the entry.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 *
 * @return The success state or the error code.
 */
static int32_t _ProcFSThreadGenericClose(void* pExtraData, void* pFileHandle);

/**
 * @brief Read the ProcFS thread status entry.
 *
 * @details Read the ProcFS thread status entry. This function will read the
 * information from the kernel and write it to the buffer given as parameter.
 * The function will return the number of bytes read or an error code.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSThreadStatusRead(void*  pExtraData,
                                       void*  pFileHandle,
                                       void*  pBuffer,
                                       size_t count);

/**
 * @brief Read the ProcFS thread scheduling statistics entry.
 *
 * @details Read the ProcFS thread scheduling statistics entry. This function
 * will read the information from the kernel and write it to the buffer given as
 * parameter. The function will return the number of bytes read or an error code.
 *
 * @param[in] pExtraData The driver data pointer.
 * @param[in] pFileHandle The file handle pointer.
 * @param[out] pBuffer The buffer to write the information to.
 * @param[in] count The size of the buffer given as parameter.
 *
 * @return The number of bytes read or an error code.
 */
static ssize_t _ProcFSThreadSchedStatsRead(void*  pExtraData,
                                           void*  pFileHandle,
                                           void*  pBuffer,
                                           size_t count);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PROCFS status operations */
static S_ProcFSFileOperations sProcFSStatusOps =
{
  .pOpen    = _ProcFSGenericOpen,
  .pClose   = _ProcFSGenericClose,
  .pRead    = _ProcFSStatusRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief PROCFS scheduler statistics operations */
static S_ProcFSFileOperations sProcFSSchedStatsOps =
{
  .pOpen    = _ProcFSGenericOpen,
  .pClose   = _ProcFSGenericClose,
  .pRead    = _ProcFSSchedStatsRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief PROCFS memory map operations */
static S_ProcFSFileOperations sProcFSMemoryMapOps =
{
  .pOpen    = _ProcFSGenericOpen,
  .pClose   = _ProcFSGenericClose,
  .pRead    = _ProcFSMemoryMapRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief PROCFS file descriptors operations */
static S_ProcFSFileOperations sProcFSFileDescriptorsOps =
{
  .pOpen    = _ProcFSGenericOpen,
  .pClose   = _ProcFSGenericClose,
  .pRead    = _ProcFSFileDescriptorsRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief PROCFS thread status operations */
static S_ProcFSFileOperations sProcFSThreadStatusOps =
{
  .pOpen    = _ProcFSThreadGenericOpen,
  .pClose   = _ProcFSThreadGenericClose,
  .pRead    = _ProcFSThreadStatusRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/** @brief PROCFS thread scheduler statistics operations */
static S_ProcFSFileOperations sProcFSThreadSchedStatsOps =
{
  .pOpen    = _ProcFSThreadGenericOpen,
  .pClose   = _ProcFSThreadGenericClose,
  .pRead    = _ProcFSThreadSchedStatsRead,
  .pWrite   = NULL,
  .pReadDir = NULL,
  .pIOCTL   = NULL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* _ProcFSGenericOpen(void*       pExtraData,
                                const char* kpPath,
                                int32_t     flags,
                                int32_t     mode)
{
  S_ProcessFileHandle* pHandle;

  (void)mode;
  if(flags == O_RDONLY && *kpPath == 0)
  {
    pHandle = KMallocUser(sizeof(S_ProcessFileHandle), ALIGN_ADDRESS, NULL);
    if (pHandle != NULL)
    {
      pHandle->pProcess = (S_KernelProcess*)pExtraData;
      pHandle->offset   = 0;
    }
    else
    {
      pHandle = (void*)-1;
    }
  }
  else
  {
    pHandle = (void*)-1;
  }

  return pHandle;
}

static int32_t _ProcFSGenericClose(void* pExtraData, void* pFileHandle)
{
  int32_t retCode;

  (void)pExtraData;

  if(pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    KFreeUser(pFileHandle, NULL);
    retCode = 0;
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSStatusRead(void*  pExtraData,
                                 void*  pFileHandle,
                                 void*  pBuffer,
                                 size_t count)

{
  ssize_t              retCode;
  char*                pBufferChar;
  S_KernelProcess*     pProcess;
  S_KernelThread*      pThread;
  uint32_t             kThreadCount;
  uint32_t             uThreadCount;
  size_t               allocatedMemory;
  S_KernelQueueNode*   pNode;
  S_ProcessFileHandle* pProcessFileHandle;
  E_ThreadState        threadState;

  if (pFileHandle != NULL &&pFileHandle != (void*)-1)
  {
    pProcessFileHandle = (S_ProcessFileHandle*)pFileHandle;
    pBufferChar = KMallocUser(PROCESS_STATUS_LENGTH, ALIGN_ADDRESS, NULL);
    if (pBufferChar != NULL)
    {
      pProcess = (S_KernelProcess*)pExtraData;

      KERNEL_LOCK(pProcess->lock);
      allocatedMemory = MemoryGetProcessAllocatedMemory(pProcess);
      uThreadCount = 0;
      kThreadCount = 0;

      pNode = pProcess->pThreads->pHead;
      threadState = THREAD_STATE_READY;
      while (pNode != NULL)
      {
        pThread = (S_KernelThread*)pNode->pData;
        if (pThread->type == THREAD_TYPE_KERNEL)
        {
          kThreadCount++;
        }
        else
        {
          uThreadCount++;
        }
        if (pThread->currentState == THREAD_STATE_RUNNING)
        {
          threadState = THREAD_STATE_RUNNING;
        }
        else if (pThread->currentState == THREAD_STATE_SLEEPING)
        {
          if (threadState != THREAD_STATE_RUNNING)
          {
            threadState = THREAD_STATE_SLEEPING;
          }
        }
        else if (pThread->currentState == THREAD_STATE_JOINING)
        {
          if (threadState != THREAD_STATE_RUNNING &&
              threadState != THREAD_STATE_SLEEPING)
          {
            threadState = THREAD_STATE_JOINING;
          }
          threadState = THREAD_STATE_JOINING;
        }
        else if (pThread->currentState == THREAD_STATE_ZOMBIE)
        {
          if (threadState != THREAD_STATE_RUNNING &&
              threadState != THREAD_STATE_SLEEPING &&
              threadState != THREAD_STATE_JOINING)
          {
            threadState = THREAD_STATE_ZOMBIE;
          }
        }
        else if (pThread->currentState == THREAD_STATE_WAITING)
        {
          if (threadState != THREAD_STATE_RUNNING &&
              threadState != THREAD_STATE_SLEEPING &&
              threadState != THREAD_STATE_JOINING &&
              threadState != THREAD_STATE_ZOMBIE)
          {
            threadState = THREAD_STATE_WAITING;
          }
        }

        pNode = pNode->pNext;
      }
      retCode = snprintf(pBufferChar,
                         PROCESS_STATUS_LENGTH,
                         "Name: %s\n"
                         "PID: %d\n"
                         "Parent PID: %d\n"
                         "State: %d (%s)\n"
                         "Main Thread TID: %d\n"
                         "KThreads: %d\n"
                         "UThreads: %d\n"
                         "Memory Allocated: %d bytes\n",
                         pProcess->pName,
                         pProcess->pid,
                         pProcess->pParent != NULL ?
                                              pProcess->pParent->pid : -1,
                         threadState,
                         SchedulerGetThreadStateString(threadState),
                         pProcess->pMainThread->tid,
                         kThreadCount,
                         uThreadCount,
                         allocatedMemory);
      if (retCode > 0 && retCode < PROCESS_STATUS_LENGTH)
      {
        if (pProcessFileHandle->offset < (size_t)retCode)
        {
          retCode = retCode - pProcessFileHandle->offset;
          if (retCode > (ssize_t)count)
          {
            retCode = count;
          }
          memcpy(pBuffer,
                 pBufferChar + pProcessFileHandle->offset,
                 retCode);
          pProcessFileHandle->offset += retCode;
        }
        else
        {
          retCode = 0;
        }
      }
      else
      {
        retCode = -1;
      }
      KERNEL_UNLOCK(pProcess->lock);

      KFreeUser(pBufferChar, NULL);
    }
    else
    {
      retCode = -1;
    }
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSSchedStatsRead(void*  pExtraData,
                                     void*  pFileHandle,
                                     void*  pBuffer,
                                     size_t count)
{
  ssize_t              retCode;
  char*                pBufferChar;
  S_KernelProcess*     pProcess;
  S_KernelThread*      pThread;
  uint32_t             kThreadCount;
  uint32_t             uThreadCount;
  size_t               allocatedMemory;
  S_KernelQueueNode*   pNode;
  S_ProcessFileHandle* pProcessFileHandle;
  E_ThreadState        threadState;
  uint32_t             priotity;
  S_CPUMask            cpuMask;
  uint64_t             userTime;
  uint64_t             kernelTime;
  S_CPUMaskString      cpuMaskString;

  if (pFileHandle != NULL &&pFileHandle != (void*)-1)
  {
    pProcessFileHandle = (S_ProcessFileHandle*)pFileHandle;
    pBufferChar = KMallocUser(PROCESS_STATUS_LENGTH, ALIGN_ADDRESS, NULL);
    if (pBufferChar != NULL)
    {
      pProcess = (S_KernelProcess*)pExtraData;

      KERNEL_LOCK(pProcess->lock);
      allocatedMemory = MemoryGetProcessAllocatedMemory(pProcess);
      uThreadCount    = 0;
      kThreadCount    = 0;
      userTime        = 0;
      kernelTime      = 0;
      priotity        = KERNEL_LOWEST_PRIORITY;

      pNode = pProcess->pThreads->pHead;
      threadState = THREAD_STATE_READY;
      CPU_MASK_RESET(cpuMask);
      while (pNode != NULL)
      {
        pThread = (S_KernelThread*)pNode->pData;
        if (pThread->type == THREAD_TYPE_KERNEL)
        {
          kThreadCount++;
        }
        else
        {
          uThreadCount++;
        }
        if (pThread->currentState == THREAD_STATE_RUNNING)
        {
          threadState = THREAD_STATE_RUNNING;
        }
        else if (pThread->currentState == THREAD_STATE_SLEEPING)
        {
          if (threadState != THREAD_STATE_RUNNING)
          {
            threadState = THREAD_STATE_SLEEPING;
          }
        }
        else if (pThread->currentState == THREAD_STATE_JOINING)
        {
          if (threadState != THREAD_STATE_RUNNING &&
              threadState != THREAD_STATE_SLEEPING)
          {
            threadState = THREAD_STATE_JOINING;
          }
          threadState = THREAD_STATE_JOINING;
        }
        else if (pThread->currentState == THREAD_STATE_ZOMBIE)
        {
          if (threadState != THREAD_STATE_RUNNING &&
              threadState != THREAD_STATE_SLEEPING &&
              threadState != THREAD_STATE_JOINING)
          {
            threadState = THREAD_STATE_ZOMBIE;
          }
        }
        else if (pThread->currentState == THREAD_STATE_WAITING)
        {
          if (threadState != THREAD_STATE_RUNNING &&
              threadState != THREAD_STATE_SLEEPING &&
              threadState != THREAD_STATE_JOINING &&
              threadState != THREAD_STATE_ZOMBIE)
          {
            threadState = THREAD_STATE_WAITING;
          }
        }

        if (pThread->priority < priotity)
        {
          priotity = pThread->priority;
        }

        CPU_MASK_OR(cpuMask, pThread->affinity);
        userTime += pThread->execTimeUser;
        kernelTime += pThread->execTimeKernel;
        pNode = pNode->pNext;
      }
      CPUGetMaskString(&cpuMask, cpuMaskString);
      retCode = snprintf(pBufferChar,
                         PROCESS_STATUS_LENGTH,
                         "%d %s %d %d %llu %llu %d %llu"
                         " %llu %llu %s %llu\n",
                         pProcess->pid,
                         pProcess->pName,
                         threadState,
                         pProcess->pParent != NULL ?
                                              pProcess->pParent->pid : -1,
                         userTime,
                         kernelTime,
                         priotity,
                         kThreadCount + uThreadCount,
                         pProcess->pMainThread->startTime,
                         allocatedMemory,
                         cpuMaskString,
                         (uintptr_t)pProcess->pMainThread->returnValue);
      if (retCode > 0 && retCode < PROCESS_STATUS_LENGTH)
      {
        if (pProcessFileHandle->offset < (size_t)retCode)
        {
          retCode = retCode - pProcessFileHandle->offset;
          if (retCode > (ssize_t)count)
          {
            retCode = count;
          }
          memcpy(pBuffer,
                 pBufferChar + pProcessFileHandle->offset,
                 retCode);
          pProcessFileHandle->offset += retCode;
        }
        else
        {
          retCode = 0;
        }
      }
      else
      {
        retCode = -1;
      }
      KERNEL_UNLOCK(pProcess->lock);
      KFreeUser(pBufferChar, pProcess->pHeap);
    }
    else
    {
      retCode = -1;
    }
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSMemoryMapRead(void*  pExtraData,
                                    void*  pFileHandle,
                                    void*  pBuffer,
                                    size_t count)
{
  /* TODO: Implement read functionality */
  (void)pExtraData;
  (void)pFileHandle;
  (void)pBuffer;
  (void)count;

  return 0;
}

static ssize_t _ProcFSFileDescriptorsRead(void*  pExtraData,
                                          void*  pFileHandle,
                                          void*  pBuffer,
                                          size_t count)
{
  /* TODO: Implement read functionality */
  (void)pExtraData;
  (void)pFileHandle;
  (void)pBuffer;
  (void)count;

  return 0;
}

static void* _ProcFSThreadGenericOpen(void*       pExtraData,
                                      const char* kpPath,
                                      int32_t     flags,
                                      int32_t     mode)
{
  S_ThreadFileHandle* pHandle;

  (void)mode;
  if(flags == O_RDONLY && *kpPath == 0)
  {
    pHandle = KMallocUser(sizeof(S_ThreadFileHandle), ALIGN_ADDRESS, NULL);
    if (pHandle != NULL)
    {
      pHandle->pThread = (S_KernelThread*)pExtraData;
      pHandle->offset  = 0;
    }
    else
    {
      pHandle = (void*)-1;
    }
  }
  else
  {
    pHandle = (void*)-1;
  }

  return pHandle;
}

static int32_t _ProcFSThreadGenericClose(void* pExtraData, void* pFileHandle)
{
  int32_t retCode;

  (void)pExtraData;

  if(pFileHandle != (void*)-1 && pFileHandle != NULL)
  {
    KFreeUser(pFileHandle, NULL);
    retCode = 0;
  }
  else
  {
    retCode = -1;
  }

  return retCode;
}

static ssize_t _ProcFSThreadStatusRead(void*  pExtraData,
                                       void*  pFileHandle,
                                       void*  pBuffer,
                                       size_t count)
{
  /* TODO: Implement read functionality */
  (void)pExtraData;
  (void)pFileHandle;
  (void)pBuffer;
  (void)count;

  return 0;
}

static ssize_t _ProcFSThreadSchedStatsRead(void*  pExtraData,
                                           void*  pFileHandle,
                                           void*  pBuffer,
                                           size_t count)
{
  /* TODO: Implement read functionality */
  (void)pExtraData;
  (void)pFileHandle;
  (void)pBuffer;
  (void)count;

  return 0;
}

E_Return CoreProcessFSCreateEntry(S_KernelProcess* pProcess)
{
  E_Return error;
  E_Return retCode;
  int      size;
  char     procFsEntryName[PROC_FS_ENTRY_NAME_MAX_LENGTH];

  /* Init the ProcFS lock */
  KERNEL_SPINLOCK_INIT(pProcess->procFSLock);

  /* Create the ProcFS entry for the process */
  size = snprintf(procFsEntryName,
                  PROC_FS_ENTRY_NAME_MAX_LENGTH,
                  "%d",
                  pProcess->pid);
  if (size > 0 && size < PROC_FS_ENTRY_NAME_MAX_LENGTH)
  {
    error = ProcFSCreateDir(procFsEntryName, NULL, (S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
    if (error == NO_ERROR)
    {
      /* Add the thread directory */
      error = ProcFSCreateDir("threads",
                              procFsEntryName,
                              (S_ProcFSDirEntry **)&pProcess->pProcFSThreadsEntry);

      if (error != NO_ERROR)
      {
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS entry.",
                             retCode);
      }
    }

    if (error == NO_ERROR)
    {
      /* Add the status entry */
      error = ProcFSCreateEntry("status",
                                0,
                                pProcess->pProcFSEntry,
                                &sProcFSStatusOps,
                                pProcess,
                                (S_ProcFSDirEntry **)&pProcess->pProcFSStatusEntry);
      if (error != NO_ERROR)
      {
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSThreadsEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS threads entry.",
                             retCode);
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS entry.",
                             retCode);
      }
    }

    if (error == NO_ERROR)
    {
      /* Add the scheduler statistics entry */
      error = ProcFSCreateEntry("schedstats",
                                0,
                                pProcess->pProcFSEntry,
                                &sProcFSSchedStatsOps,
                                pProcess,
                                (S_ProcFSDirEntry **)&pProcess->pProcFSSchedulerStatsEntry);
      if (error != NO_ERROR)
      {
        retCode = ProcFSRemoveEntry("status", pProcess->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS status entry.",
                             retCode);
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSThreadsEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS threads entry.",
                             retCode);
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS entry.",
                             retCode);
      }

      if (error == NO_ERROR)
      {
        /* Add the memory map entries */
        error = ProcFSCreateEntry("memmap",
                                  0,
                                  pProcess->pProcFSEntry,
                                  &sProcFSMemoryMapOps,
                                  pProcess,
                                  (S_ProcFSDirEntry **)&pProcess->pProcFSMemoryMapEntry);
        if (error != NO_ERROR)
        {
          retCode = ProcFSRemoveEntry("schedstats", pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS scheduler stats entry.",
                              retCode);
          retCode = ProcFSRemoveEntry("status", pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS status entry.",
                              retCode);
          retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSThreadsEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS threads entry.",
                              retCode);
          retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS entry.",
                              retCode);
        }
      }

      if (error == NO_ERROR)
      {
        /* Add the file descriptor entries */
        error = ProcFSCreateEntry("fd",
                                  0,
                                  pProcess->pProcFSEntry,
                                  &sProcFSFileDescriptorsOps,
                                  pProcess,
                                  (S_ProcFSDirEntry **)&pProcess->pProcFSFileDescriptorsEntry);
        if (error != NO_ERROR)
        {
          retCode = ProcFSRemoveEntry("memmap", pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS memory map entry.",
                              retCode);
          retCode = ProcFSRemoveEntry("schedstats", pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS scheduler stats entry.",
                              retCode);
          retCode = ProcFSRemoveEntry("status", pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS status entry.",
                              retCode);
          retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSThreadsEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS threads entry.",
                              retCode);
          retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
          COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                              "Failed to remove ProcFS entry.",
                              retCode);
        }
      }
    }
  }
  else
  {
    error = ERR_EXCEEDED_LIMIT;
  }

  return error;
}

void CoreProcessFSDeleteEntry(S_KernelProcess* pProcess)
{
  E_Return retCode;

  /* Delete the thread directory */
  retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSThreadsEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS threads entry.",
                       retCode);


  /* Delete the status entry */
  retCode = ProcFSRemoveEntry("status", pProcess->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS status entry.",
                       retCode);

  /* Delete the scheduler statistics entry */
  retCode = ProcFSRemoveEntry("schedstats", pProcess->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS scheduler stats entry.",
                       retCode);

  /* Delete the memory map entries */
  retCode = ProcFSRemoveEntry("memmap", pProcess->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS memory map entry.",
                       retCode);
  /* Delete the file descriptor entries */
  retCode = ProcFSRemoveEntry("fd", pProcess->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS file descriptors entry.",
                       retCode);

  /* Delete the ProcFS entry for the process */
  retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pProcess->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS entry.",
                       retCode);
}

E_Return CoreProcessFSCreateThreadEntry(S_KernelThread* pThread)
{
  E_Return error;
  E_Return retCode;
  int      size;
  char     procFsEntryName[PROC_FS_ENTRY_NAME_MAX_LENGTH];
  char     parentName[PROC_FS_ENTRY_NAME_MAX_LENGTH + 10];

  /* Create the ProcFS entry for the thread */
  size = snprintf(procFsEntryName,
                  PROC_FS_ENTRY_NAME_MAX_LENGTH,
                  "%d",
                  pThread->tid);
  if (size > 0 && size < PROC_FS_ENTRY_NAME_MAX_LENGTH)
  {
    snprintf(parentName,
             PROC_FS_ENTRY_NAME_MAX_LENGTH + 10,
             "%d/threads",
             pThread->pProcess->pid);

    KERNEL_LOCK(pThread->pProcess->procFSLock);

    error = ProcFSCreateDir(procFsEntryName,
                            parentName,
                            (S_ProcFSDirEntry **)&pThread->pProcFSEntry);

    if (error == NO_ERROR)
    {
      /* Add the status entry */
      error = ProcFSCreateEntry("status",
                                0,
                                pThread->pProcFSEntry,
                                &sProcFSThreadStatusOps,
                                pThread,
                                (S_ProcFSDirEntry **)&pThread->pProcFSStatusEntry);
      if (error != NO_ERROR)
      {
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pThread->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS entry.",
                             retCode);
      }
    }

    if (error == NO_ERROR)
    {
      /* Add the scheduler statistics entry */
      error = ProcFSCreateEntry("schedstats",
                                0,
                                pThread->pProcFSEntry,
                                &sProcFSThreadSchedStatsOps,
                                pThread,
                                (S_ProcFSDirEntry **)&pThread->pProcFSSchedulerStatsEntry);
      if (error != NO_ERROR)
      {
        retCode = ProcFSRemoveEntry("status", pThread->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS status entry.",
                             retCode);
        retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pThread->pProcFSEntry);
        COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                             "Failed to remove ProcFS entry.",
                             retCode);
      }
    }

    KERNEL_UNLOCK(pThread->pProcess->procFSLock);
  }
  else
  {
    error = ERR_EXCEEDED_LIMIT;
  }

  return error;
}

void CoreProcessFSDeleteThreadEntry(S_KernelThread* pThread)
{
  E_Return retCode;

  /* Delete the status entry */
  retCode = ProcFSRemoveEntry("status", pThread->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS status entry.",
                       retCode);

  /* Delete the scheduler statistics entry */
  retCode = ProcFSRemoveEntry("schedstats", pThread->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS scheduler stats entry.",
                       retCode);

  KERNEL_LOCK(pThread->pProcess->procFSLock);

  /* Delete the ProcFS entry for the thread */
  retCode = ProcFSRemoveDir((S_ProcFSDirEntry **)&pThread->pProcFSEntry);
  COREPROCESSFS_ASSERT(retCode == NO_ERROR,
                       "Failed to remove ProcFS entry.",
                       retCode);

  KERNEL_UNLOCK(pThread->pProcess->procFSLock);
}

/************************************ EOF *************************************/