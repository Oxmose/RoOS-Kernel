/*******************************************************************************
 * @file Signals.c
 *
 * @see Signals.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <errno.h>
#include <stdint.h>
#include <Memory.h>
#include <Critical.h>
#include <CtrlBlock.h>
#include <Scheduler.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <Signals.h>

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
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void SignalInitinitalize(S_KernelThread* pThread,
                         S_KernelThread* pCurrentThread)
{
  if (pCurrentThread != NULL)
  {
    /* Inherit the signal mask from the current thread */
    pThread->blockedSignals = pCurrentThread->blockedSignals;
  }
  else
  {
    /* Reset the signal mask */
    pThread->blockedSignals = 0;
  }

  /* Clear the signal queue and mask */
  pThread->pendingSignals = 0;

  /* Initialize the signal lock */
  SPINLOCK_INIT(pThread->signalLock);
}

E_Return RegisterSignalHandler(const THREAD_SIGNAL_E kSignal,
                               T_SignalHandler       handler,
                               S_KernelThread*       pThread)
{
  E_Return         error;
  S_KernelProcess* pProcess;

  if (pThread->type == THREAD_TYPE_USER)
  {
    if (kSignal >= 0 &&
        kSignal < THREAD_SIG_MAX_VALUE &&
        kSignal != THREAD_SIGKILL &&
        kSignal != THREAD_SIGSTOP)
    {
      pProcess = pThread->pProcess;
      SpinlockAcquire(&pProcess->signalLock);
      pProcess->signalHandlers[kSignal] = (void*)handler;
      SpinlockRelease(&pProcess->signalLock);
      error = NO_ERROR;
    }
    else
    {
      error = ERR_INVALID_PARAMETER;
    }
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }

  return error;
}

E_Return SetSignalMask(S_KernelThread*       pThread,
                       const THREAD_SIGNAL_E kSignal,
                       const bool            kBlock)
{
  E_Return error;
  uint32_t intState;

  if (pThread->type == THREAD_TYPE_USER)
  {
    if (kSignal >= 0 &&
        kSignal < THREAD_SIG_MAX_VALUE &&
        kSignal != THREAD_SIGKILL &&
        kSignal != THREAD_SIGSTOP)
    {
      KERNEL_ENTER_CRITICAL_LOCAL(intState);
      SpinlockAcquireISR(&pThread->signalLock);
      if (kBlock == true)
      {
        pThread->blockedSignals |= (1 << kSignal);
      }
      else
      {
        pThread->blockedSignals &= ~(1 << kSignal);
      }
      SpinlockReleaseISR(&pThread->signalLock);
      KERNEL_EXIT_CRITICAL_LOCAL(intState);

      error = NO_ERROR;
    }
    else
    {
      error = ERR_INVALID_PARAMETER;
    }
  }
  else
  {
    error = ERR_UNAUTHORIZED_ACTION;
  }

  return error;
}

E_Return SignalThread(S_KernelThread* pThread, const THREAD_SIGNAL_E kSignal)
{
  E_Return error;
  uint32_t intState;

  if (kSignal >= 0 && kSignal < THREAD_SIG_MAX_VALUE)
  {
    if (pThread->type == THREAD_TYPE_USER)
    {
      KERNEL_ENTER_CRITICAL_LOCAL(intState);
      SpinlockAcquireISR(&pThread->signalLock);
      pThread->pendingSignals |= (1 << kSignal);
      if ((pThread->blockedSignals & (1 << kSignal)) == 0 ||
          kSignal == THREAD_SIGKILL ||
          kSignal == THREAD_SIGSTOP ||
          kSignal == THREAD_SIGCONT)
      {
        /* The signal is not blocked, set the thread to ready */
        SetThreadSignaled(pThread);
      }
      SpinlockReleaseISR(&pThread->signalLock);
      KERNEL_EXIT_CRITICAL_LOCAL(intState);
      error = NO_ERROR;
    }
    else
    {
      error = ERR_UNAUTHORIZED_ACTION;
    }
  }
  else
  {
    error = ERR_INVALID_PARAMETER;
  }

  return error;
}

void SignalManage(S_KernelThread* pThread, const bool kIsSyscall)
{
  uint32_t         i;
  void*            handler;
  bool             isReturningToUser;
  S_KernelProcess* pProcess;
  uint64_t         pendingSignals;
  uint32_t         intState;

  isReturningToUser = kIsSyscall || CPUIsReturningToUser(pThread);

  if (isReturningToUser == true)
  {
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    SpinlockAcquireISR(&pThread->signalLock);
    pendingSignals = pThread->pendingSignals;
    SpinlockReleaseISR(&pThread->signalLock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);
    if (pendingSignals != 0)
    {
      /* Get the next signal */
      for (i = 0; i < THREAD_SIG_MAX_VALUE; ++i)
      {
        if ((pendingSignals & (1 << i)) != 0)
        {

          /* Manage specific cases */
          if (i == THREAD_SIGKILL)
          {
            /* Clear the pending signal */
            pendingSignals &= ~(1 << i);

            /* Kill the thread */
            KillCurrentThread();
          }
          else if (i == THREAD_SIGSTOP)
          {
            /* Clear the pending signal */
            pendingSignals &= ~(1 << i);

            /* Stop the thread */
            SetCurrentThreadToWaiting(NULL, true);
          }
          else if ((pThread->blockedSignals & (1 << i)) == 0)
          {
            /* Clear the pending signal */
            KERNEL_ENTER_CRITICAL_LOCAL(intState);

            SpinlockAcquireISR(&pThread->signalLock);
            pThread->pendingSignals &= ~(1 << i);
            SpinlockReleaseISR(&pThread->signalLock);

            pProcess = pThread->pProcess;

            SpinlockAcquireISR(&pProcess->signalLock);
            handler = pProcess->signalHandlers[i];
            SpinlockReleaseISR(&pProcess->signalLock);

            KERNEL_EXIT_CRITICAL_LOCAL(intState);

            if (handler != NULL)
            {
              if (kIsSyscall == true)
              {
                CPUThreadSignalFromSyscall(pThread, (uintptr_t)handler, i);
              }
              else
              {
                CPUThreadSignalFromInt(pThread, (uintptr_t)handler, i);
              }
            }
            else
            {
              /* No handler, kill the thread */
              KillCurrentThread();
            }

            break;
          }
        }
      }
    }
  }
}

/*******************************************************************************
 * SYSCALL HANDLERS
 ******************************************************************************/
void* SyscallSignal(void* pParam0,
                    void* pParam1,
                    void* pParam2,
                    void* pParam3,
                    void* pParam4)
{
  E_Return        retCode;
  void*           returnValue;
  THREAD_SIGNAL_E signal;
  S_KernelThread* pThread;

  (void)pParam2;
  (void)pParam3;
  (void)pParam4;

  signal = (THREAD_SIGNAL_E)(uintptr_t)pParam0;
  pThread = (S_KernelThread*)pParam1;

  if (IsThreadValid(pThread) == true)
  {
    retCode = SignalThread(pThread, signal);
    if (retCode == NO_ERROR)
    {
      returnValue = (void*)0;
    }
    else if (retCode == ERR_INVALID_PARAMETER)
    {
      returnValue = (void*)-EINVAL;
    }
    else
    {
      returnValue = (void*)-EPERM;
    }
  }
  else
  {
    returnValue = (void*)-ESRCH;
  }

  return returnValue;
}

void* SyscallSignalRegister(void* pParam0,
                            void* pParam1,
                            void* pParam2,
                            void* pParam3,
                            void* pParam4)
{
  E_Return        retCode;
  void*           returnValue;
  THREAD_SIGNAL_E signal;
  T_SignalHandler handler;
  S_KernelThread* pThread;

  (void)pParam2;
  (void)pParam3;
  (void)pParam4;

  signal  = (THREAD_SIGNAL_E)(uintptr_t)pParam0;
  handler = (T_SignalHandler)(uintptr_t)pParam1;

  if (MemoryIsMappedWithFlags(handler,
                              sizeof(uintptr_t),
                              MEMMGR_MAP_USER |
                              MEMMGR_MAP_RO |
                              MEMMGR_MAP_EXEC) == true)
  {
    pThread = GetCurrentThread();
    retCode = RegisterSignalHandler(signal, handler, pThread);
    if (retCode == NO_ERROR)
    {
      returnValue = (void*)0;
    }
    else if (retCode == ERR_INVALID_PARAMETER)
    {
      returnValue = (void*)-EINVAL;
    }
    else
    {
      returnValue = (void*)-EPERM;
    }
  }
  else
  {
    returnValue = (void*)-EINVAL;
  }

  return returnValue;
}

void* SyscallSignalMask(void* pParam0,
                        void* pParam1,
                        void* pParam2,
                        void* pParam3,
                        void* pParam4)
{
  E_Return        retCode;
  void*           returnValue;
  THREAD_SIGNAL_E signal;
  bool            block;
  S_KernelThread* pThread;

  (void)pParam2;
  (void)pParam3;
  (void)pParam4;

  signal = (THREAD_SIGNAL_E)(uintptr_t)pParam0;
  block  = (bool)(uintptr_t)pParam1;

  pThread = GetCurrentThread();
  retCode = SetSignalMask(pThread, signal, block);
  if (retCode == NO_ERROR)
  {
    returnValue = (void*)0;
  }
  else if (retCode == ERR_INVALID_PARAMETER)
  {
    returnValue = (void*)-EINVAL;
  }
  else
  {
    returnValue = (void*)-EPERM;
  }

  return returnValue;
}

void* SyscallSignalReturn(void* pParam0,
                          void* pParam1,
                          void* pParam2,
                          void* pParam3,
                          void* pParam4)
{
  void* pUserContext;

  (void)pParam1;
  (void)pParam2;
  (void)pParam3;
  (void)pParam4;

  pUserContext = (void*)(uintptr_t)pParam0;
  CPUThreadSignalReturn(pUserContext);

  return (void*)-EFAULT;
}

/************************************ EOF *************************************/