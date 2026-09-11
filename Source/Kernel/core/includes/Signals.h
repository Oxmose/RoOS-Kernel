/*******************************************************************************
 * @file Signals.h
 *
 * @see Signals.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/09/2026
 *
 * @version 2.0
 *
 * @brief Kernel thread signaling manager.
 *
 * @details Kernel thread signaling manager. Signal are used to communicate
 * between threads. A signal is handled the next time the thread is scheduled.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_SIGNALS_H_
#define __CORE_SIGNALS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <CtrlBlock.h>
#include <KernelError.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Signal handler type. */
typedef void (*T_SignalHandler)(int32_t, void*);

/** @brief Defines the thread signals */
typedef enum
{
  /** @brief Signal: hangup. */
  THREAD_SIGHUP = 1,
  /** @brief Signal: interrupt. */
  THREAD_SIGINT = 2,
  /** @brief Signal: quit. */
  THREAD_SIGQUIT = 3,
  /** @brief Signal: illegal instruction. */
  THREAD_SIGILL = 4,
  /** @brief Signal: trace trap. */
  THREAD_SIGTRAP = 5,
  /** @brief Signal: abort. */
  THREAD_SIGABRT = 6,
  /** @brief Signal: bus error. */
  THREAD_SIGBUS = 7,
  /** @brief Signal: floating point exception. */
  THREAD_SIGFPE = 8,
  /** @brief Signal: kill. */
  THREAD_SIGKILL = 9,
  /** @brief Signal: user defined signal 1. */
  THREAD_SIGUSR1 = 10,
  /** @brief Signal: segmentation fault. */
  THREAD_SIGSEGV = 11,
  /** @brief Signal: user defined signal 2. */
  THREAD_SIGUSR2 = 12,
  /** @brief Signal: broken pipe. */
  THREAD_SIGPIPE = 13,
  /** @brief Signal: alarm clock. */
  THREAD_SIGALRM = 14,
  /** @brief Signal: termination. */
  THREAD_SIGTERM = 15,
  /** @brief Signal: stack fault. */
  THREAD_SIGSTKFLT = 16,
  /** @brief Signal: child process terminated. */
  THREAD_SIGCHLD = 17,
  /** @brief Signal: continue. */
  THREAD_SIGCONT = 18,
  /** @brief Signal: stop. */
  THREAD_SIGSTOP = 19,
  /** @brief Signal: terminal stop. */
  THREAD_SIGTSTP = 20,
  /** @brief Signal: terminal input. */
  THREAD_SIGTTIN = 21,
  /** @brief Signal: terminal output. */
  THREAD_SIGTTOU = 22,
  /** @brief Signal: urgent data available. */
  THREAD_SIGURG = 23,
  /** @brief Signal: cpu time limit exceeded. */
  THREAD_SIGXCPU = 24,
  /** @brief Signal: file size limit exceeded. */
  THREAD_SIGXFSZ = 25,
  /** @brief Signal: virtual timer expired. */
  THREAD_SIGVTALRM = 26,
  /** @brief Signal: profiling timer expired. */
  THREAD_SIGPROF = 27,
  /** @brief Signal: window size change. */
  THREAD_SIGWINCH = 28,
  /** @brief Signal: IO now available. */
  THREAD_SIGIO = 29,
  /** @brief Signal: bad system call. */
  THREAD_SIGPWR = 30,
  /** @brief Signal: bad system call. */
  THREAD_SIGSYS = 31,
  /** @brief Maximal signal value. */
  THREAD_SIG_MAX_VALUE
} THREAD_SIGNAL_E;

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
 * @brief Initializes the signals for a thread.
 *
 * @details Initializes the signals for a thread. The signal table is setup by
 * default handlers and the signal mask reset.
 *
 * @param[in, out] pThread The thread to initialize.
 * @param[in] pCurrentThread The current thread.
 */
void SignalInitinitalize(S_KernelThread* pThread,
                         S_KernelThread* pCurrentThread);

/**
 * @brief Registers a new signal handler for the current thread.
 *
 * @details Registers a new signal handler for the current thread. The handler
 * is added to the thread signal handers table and is effective immediately.
 *
 * @param[in] kSignal The signal to register the handler for.
 * @param[in] handler The handler to register.
 * @param[in, out] pThread The thread to register the handler for.
 *
 * @return The function returns the error or success status.
 */
E_Return RegisterSignalHandler(const THREAD_SIGNAL_E kSignal,
                               T_SignalHandler       handler,
                               S_KernelThread*       pThread);

/**
 * @brief Sets the signal mask for the thread for a given signal.
 *
 * @details Sets the signal mask for the thread for a given signal. The signal
 * mask determines which signals are blocked for the thread.
 *
 * @param[in, out] pThread The thread to set the signal mask for.
 * @param[in] kSignal The signal to add to the mask.
 * @param[in] kBlock If true, the signal is blocked; if false, the signal is
 * unblocked.
 *
 * @return The function returns the error or success status.
 */
E_Return SetSignalMask(S_KernelThread*       pThread,
                       const THREAD_SIGNAL_E kSignal,
                       const bool            kBlock);

/**
 * @brief Signals a thread.
 *
 * @details Signals a thread. The next time the thread is scheduled, the
 * execution flow will be redirected to the signal handler.
 *
 * @param[in] pThread The thread to signal.
 * @param[in] kSignal The signal to send.
 *
 * @return The function returns the error or success status.
 */
E_Return SignalThread(S_KernelThread* pThread, const THREAD_SIGNAL_E kSignal);

/**
 * @brief Manages a thread signals.
 *
 * @details Manages a thread signals. Will get the last signal sent to the
 * thread and apply the according action.
 *
 * @param[in] pThread The thread to manage.
 * @param[in] kIsSyscall Tells if the signal is being managed from a syscall or
 * an interrupt.
 *
 * @warning This function shall only be called in a interrupt context as it
 * changes the virtual CPU state saved at interrupts boundaries.
 */
void SignalManage(S_KernelThread* pThread, const bool kIsSyscall);

/*******************************************************************************
 * SYSCALL HANDLERS
 ******************************************************************************/
/**
 * @brief Syscall handler for the signal syscall.
 *
 * @details Syscall handler for the signal syscall. This function is called when
 * a thread sends a signal to another thread. The function will signal the
 * target thread and return the status.
 *
 * @param[in] pParam0 The signal to send.
 * @param[in] pParam1 The thread identifier of the thread to signal.
 * @param[in] pParam2 Unused.
 * @param[in] pParam3 Unused.
 * @param[in] pParam4 Unused.
 *
 * @return The function returns the error or success status.
 */
void* SyscallSignal(void* pParam0,
                    void* pParam1,
                    void* pParam2,
                    void* pParam3,
                    void* pParam4);

/**
 * @brief Registers a signal handler for the current thread.
 *
 * @details Registers a signal handler for the current thread. The handler will
 * be called when the signal is received by the thread.
 *
 * @param[in] pParam0 The signal to register a handler for.
 * @param[in] pParam1 The signal handler function.
 * @param[in] pParam2 Unused.
 * @param[in] pParam3 Unused.
 * @param[in] pParam4 Unused.
 *
 * @return The function returns the error or success status.
 */
void* SyscallSignalRegister(void* pParam0,
                            void* pParam1,
                            void* pParam2,
                            void* pParam3,
                            void* pParam4);

/**
 * @brief Sets the signal mask for the current thread.
 *
 * @details Sets the signal mask for the current thread. The signal mask
 * determines which signals are blocked for the thread.
 *
 * @param[in] pParam0 The signal to add to the mask.
 * @param[in] pParam1 If true, the signal is blocked; if false, the signal is unblocked.
 * @param[in] pParam2 Unused.
 * @param[in] pParam3 Unused.
 * @param[in] pParam4 Unused.
 *
 * @return The function returns the error or success status.
 */
void* SyscallSignalMask(void* pParam0,
                        void* pParam1,
                        void* pParam2,
                        void* pParam3,
                        void* pParam4);

/**
 * @brief Returns from a signal handler.
 *
 * @details Returns from a signal handler. This function is called when the
 * signal handler is finished executing.
 *
 * @param[in] pParam0 The user context to return to.
 * @param[in] pParam1 Unused.
 * @param[in] pParam2 Unused.
 * @param[in] pParam3 Unused.
 * @param[in] pParam4 Unused.
 *
 * @return The function returns the error or success status.
 */
void* SyscallSignalReturn(void* pParam0,
                          void* pParam1,
                          void* pParam2,
                          void* pParam3,
                          void* pParam4);

#endif /* #ifndef __CORE_SIGNALS_H_ */

/************************************ EOF *************************************/
