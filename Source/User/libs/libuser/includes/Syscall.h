/*******************************************************************************
 * @file Syscall.h
 *
 * @see UserSycall.s
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief User kernel library.
 *
 * @details User kernel library. This library provides non standard link
 * between the user and the kernel space.
 *
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_SYSCALL_H_
#define __LIB_SYSCALL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
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
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Performs a system call.
 *
 * @details Performs a system call. The underlying CPU system call facility will
 * be called to perform the required operation and issue the system call.
 * The parameters for input and output are provided by the pParams parameter.
 *
 * @param[in] kSyscallId The system call identifier to use.
 * @param[in, out] pParam0 The first parameter to pass to the system call.
 * @param[in, out] pParam1 The second parameter to pass to the system call.
 * @param[in, out] pParam2 The third parameter to pass to the system call.
 * @param[in, out] pParam3 The fourth parameter to pass to the system call.
 * @param[in, out] pParam4 The fifth parameter to pass to the system call.
 *
 * @return The result of the system call.
 */
int Syscall(const unsigned long long kSyscallId,
            void*                    pParam0,
            void*                    pParam1,
            void*                    pParam2,
            void*                    pParam3,
            void*                    pParam4);

#endif /* #ifndef __LIB_SYSCALL_H_ */

/************************************ EOF *************************************/