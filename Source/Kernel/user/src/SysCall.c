/*******************************************************************************
 * @file SysCall.c
 *
 * @see SysCall.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/08/2026
 *
 * @version 1.0
 *
 * @brief Kernel's user entry point.
 *
 * @details Kernel system call manager. Used to register and handle system call
 * entry and exti points.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <stdint.h>
#include <Scheduler.h>
#include <VirtualFS.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <SysCall.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the current module name */
#define MODULE_NAME "SYSCALL"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the system call handler type */
typedef int32_t (*T_SyscallHandler)(void* pParam0,
                                    void* pParam1,
                                    void* pParam2,
                                    void* pParam3,
                                    void* pParam4);

/** @brief System call IDs */
typedef enum
{
  /** @brief Sleep system call */
  SYSCALL_ID_SLEEP = 0,
#if 0
  /** @brief Yield system call */
  SYSCALL_ID_YIELD,
  /** @brief Get time system call */
  SYSCALL_ID_GETTIME,
  /** @brief Get ticks system call */
  SYSCALL_ID_GETTICKS,
  /** @brief Get CPU ID system call */
  SYSCALL_ID_GETCPUID,
  /** @brief Get PID system call */
  SYSCALL_ID_GETPID,
  /** @brief Get TID system call */
  SYSCALL_ID_GETTID,
  /** @brief Thread create system call */
  SYSCALL_ID_THREAD_CREATE,
  /** @brief Thread exit system call */
  SYSCALL_ID_THREAD_EXIT,
  /** @brief Thread join system call */
  SYSCALL_ID_THREAD_JOIN,
  /** @brief Thread detach system call */
  SYSCALL_ID_THREAD_DETACH,
  /** @brief Thread set priority system call */
  SYSCALL_ID_THREAD_SET_PRIORITY,
  /** @brief Thread get priority system call */
  SYSCALL_ID_THREAD_GET_PRIORITY,
  /** @brief Thread get name system call */
  SYSCALL_ID_THREAD_GET_NAME,
  /** @brief Thread set affinity system call */
  SYSCALL_ID_THREAD_SET_AFFINITY,
  /** @brief Thread get affinity system call */
  SYSCALL_ID_THREAD_GET_AFFINITY,
#endif
  /** @brief Open file system call */
  SYSCALL_ID_OPEN,
  /** @brief Close file system call */
  SYSCALL_ID_CLOSE,
  /** @brief Read file system call */
  SYSCALL_ID_READ,
  /** @brief Readdir file system call */
  SYSCALL_ID_READDIR,
  /** @brief Write file system call */
  SYSCALL_ID_WRITE,
  /** @brief IOCTL system call */
  SYSCALL_ID_IOCTL,
  /** @brief Maximal system call ID */
  SYSCALL_ID_MAX
} E_SyscallId;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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
/** @brief System call handler table */
T_SyscallHandler spSyscallHandlerTable[SYSCALL_ID_MAX] =
{
  [SYSCALL_ID_SLEEP] = SyscallSleepNs,
#if 0
  [SYSCALL_ID_YIELD] = SyscallYield,
  [SYSCALL_ID_GETTIME] = SyscallGetTime,
  [SYSCALL_ID_GETTICKS] = SyscallGetTicks,
  [SYSCALL_ID_GETCPUID] = SyscallGetCPUID,
  [SYSCALL_ID_GETPID] = SyscallGetPID,
  [SYSCALL_ID_GETTID] = SyscallGetTID,
  [SYSCALL_ID_THREAD_CREATE] = SyscallThreadCreate,
  [SYSCALL_ID_THREAD_EXIT] = SyscallThreadExit,
  [SYSCALL_ID_THREAD_JOIN] = SyscallThreadJoin,
  [SYSCALL_ID_THREAD_DETACH] = SyscallThreadDetach,
  [SYSCALL_ID_THREAD_SET_PRIORITY] = SyscallThreadSetPriority,
  [SYSCALL_ID_THREAD_GET_PRIORITY] = SyscallThreadGetPriority,
  [SYSCALL_ID_THREAD_GET_NAME] = SyscallThreadGetName,
  [SYSCALL_ID_THREAD_SET_AFFINITY] = SyscallThreadSetAffinity,
  [SYSCALL_ID_THREAD_GET_AFFINITY] = SyscallThreadGetAffinity,
#endif
  [SYSCALL_ID_OPEN] = SyscallVFSOpen,
  [SYSCALL_ID_CLOSE] = SyscallVFSClose,
  [SYSCALL_ID_READ] = SyscallVFSRead,
  [SYSCALL_ID_READDIR] = SyscallVFSReadDir,
  [SYSCALL_ID_WRITE] = SyscallVFSWrite,
  [SYSCALL_ID_IOCTL] = SyscallVFSIOCTL,
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
int32_t SystemCallDispatcher(const uint64_t kSyscallId,
                             void*          pParam0,
                             void*          pParam1,
                             void*          pParam2,
                             void*          pParam3,
                             void*          pParam4)
{
  int32_t returnVal;

  if (kSyscallId < SYSCALL_ID_MAX)
  {
    returnVal = spSyscallHandlerTable[kSyscallId](pParam0,
                                                  pParam1,
                                                  pParam2,
                                                  pParam3,
                                                  pParam4);
  }
  else
  {
    returnVal = -1;
    /* TODO: Set errno to ENOSYS */
  }

  return returnVal;
}

/************************************ EOF *************************************/