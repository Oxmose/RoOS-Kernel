/*******************************************************************************
 * @file CtrlBlock.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2023
 *
 * @version 3.0
 *
 * @brief Kernel control block structures definitions.
 *
 * @details Kernel control block structures definitions. The files contains all
 *  the data relative to the objects management in the system (thread structure,
 * thread state, etc.).
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_CTRL_BLOCK_H_
#define __CORE_CTRL_BLOCK_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <Critical.h>
#include <KernelQueue.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Maximal thread's name length. */
#define THREAD_NAME_MAX_LENGTH 32
/** @brief Maximal number of signals a thread can support */
#define THREAD_MAX_SIGNALS 32
/** @brief Maximal process' name length. */
#define PROCESS_NAME_MAX_LENGTH 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* Forward declarations */
typedef struct S_KernelProcess S_KernelProcess;
typedef struct S_KernelThread S_KernelThread;

/** @brief Thread's scheduling state. */
typedef enum
{
  /** @brief Thread's scheduling state: running. */
  THREAD_STATE_RUNNING,
  /** @brief Thread's scheduling state: running to be elected. */
  THREAD_STATE_READY,
  /** @brief Thread's scheduling state: sleeping. */
  THREAD_STATE_SLEEPING,
  /** @brief Thread's scheduling state: zombie. */
  THREAD_STATE_ZOMBIE,
  /** @brief Thread's scheduling state: joining. */
  THREAD_STATE_JOINING,
  /** @brief Thread's scheduling state: waiting. */
  THREAD_STATE_WAITING,
} E_ThreadState;

/**
 * @brief Define the thread's types in the kernel.
 */
typedef enum
{
  /** @brief Kernel thread type, create by and for the kernel. */
  THREAD_TYPE_KERNEL,
  /** @brief User thread type, created by the kernel for the user. */
  THREAD_TYPE_USER
} E_ThreadType;

/**
 * @brief Defines a thread error information table.
 */
typedef struct
{
  /** @brief Error information defined by segfault: address */
  uintptr_t segfaultAddr;
  /** @brief Error information defined by exceptions: type of exception */
  uint32_t exceptionId;
  /** @brief Error information defined error: address of the error */
  uintptr_t instAddr;
  /** @brief Stores the virtual CPU at the moment of the error */
  void* pExecVCpu;
} S_ErrorTable;

/**
 * @brief This is the representation of a process for the kernel.
 */
typedef struct S_KernelProcess
{
  /**************************************
   * Process properties
   *************************************/
  /** @brief Process' identifier. */
  int32_t pid;

  /** @brief Process' name. */
  char pName[PROCESS_NAME_MAX_LENGTH];

  /**************************************
   * Scheduler management
   *************************************/
  /** @brief Process' parent process */
  S_KernelProcess* pParent;

  /** @brief List of children processes */
  S_KernelQueue* pChildren;

  /** @brief Process' main thread */
  S_KernelThread* pMainThread;

  /** @brief Table of the thread list. */
  S_KernelQueue* pThreads;

  /**************************************
   * Resources management
   *************************************/
  /** @brief The process' structure lock */
  S_KernelSpinlock lock;

    /** @brief Stores the memory management data for the process */
  void* pMemoryData;

} S_Process;

/** @brief This is the representation of the thread for the kernel. */
typedef struct S_KernelThread
{
  /**
   * @brief Thread's virtual CPU context, must be at the begining of the
   * structure for easy interface with assembly.
   */
  void* pVCpu;

  /**************************************
   * Thread properties
   *************************************/
  /** @brief Thread's identifier. */
  int32_t tid;

  /** @brief Thread's name. */
  char pName[THREAD_NAME_MAX_LENGTH];

  /** @brief Thread's type. */
  E_ThreadType type;

  /**************************************
   * System interface
   *************************************/
  /** @brief Thread's start arguments. */
  void* pArgs;

  /** @brief Thread's entry point. */
  void* pEntryPoint;

  /** @brief Thread's routine. */
  void* (*pRoutine)(void*);

  /** @brief Thread's return value. */
  void* returnValue;

  /**************************************
   * Stacks
   *************************************/
  /** @brief Thread's stack. */
  uintptr_t stackEnd;

  /** @brief Thread's stack size. */
  size_t stackSize;

  /** @brief Thread's interrupt stack. */
  uintptr_t kernelStackEnd;

  /** @brief Thread's interrupt stack size. */
  size_t kernelStackSize;

  /**************************************
   * Time management
   *************************************/
  /** @brief Wake up time limit for the sleeping thread. */
  uint64_t wakeupTime;

  /** @brief Thread's start time. */
  uint64_t startTime;

  /** @brief Thread's end time. */
  uint64_t endTime;

  /** @brief Thread's execution time. */
  uint64_t executionTime;

  /**************************************
   * Scheduler management
   *************************************/
  /** @brief Thread's current priority. */
  uint32_t priority;

  /** @brief Thread's current state. */
  E_ThreadState currentState;

  /** @brief Thread's previous state. */
  E_ThreadState previousState;

  /** @brief Associated queue node in the scheduler */
  S_KernelQueueNode* pThreadNode;

  /** @brief Thread's CPU affinity */
  S_CPUMask affinity;

  /** @brief Thread's currently mapped CPU */
  uint32_t mappedCPU;

  /** @brief Process */
  struct S_KernelProcess* pProcess;

  /** @brief Stores the thread that is currently joining this thread */
  struct S_KernelThread* pJoiningThread;

  /**************************************
   * Resources management
   *************************************/
  /** @brief The thread's structure lock */
  S_KernelSpinlock lock;
  /** @brief Flags that tells if the thread can safelly be scheduled */
  volatile uint32_t isScheduled;

  /** @brief Thread's error table */
  S_ErrorTable errorTable;
} S_KernelThread;

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

/* None */

#endif /* #ifndef __CORE_CTRL_BLOCK_H_ */

/************************************ EOF *************************************/