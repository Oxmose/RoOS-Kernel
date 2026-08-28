/*******************************************************************************
 * @file kernelshell.c
 *
 * @see kernelshell.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief Kernel's shell definition.
 *
 * @details Kernel's shell definition. This shell is the entry point of the
 * kernel for the user. It has kernel rights and can be extended by the user
 * for different purposes.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <Memory.h>
#include <Console.h>
#include <Critical.h>
#include <VirtualFS.h>
#include <Scheduler.h>
#include <KernelHeap.h>
#include <KernelError.h>
#include <KernelOutput.h>
#include <TimerManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <TestFramework.h>

/* Header file */
#include <KernelShell.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the kernel shell version */
#define SHELL_VERSION "0.1"

/** @brief Defines the size of the input buffer for the kernel shell. */
#define SHELL_INPUT_BUFFER_SIZE 128

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines a command structure for the kernel shell. */
typedef struct
{
  /** @brief Pointer to the command name */
  const char* pCommandName;
  /** @brief Pointer to the command description */
  const char* pDescription;
  /** @brief Pointer to the command function */
  void (*pFunc)(const char*);
} S_ShellCommand;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
static void _ShellTimeTest(const char* args);
static void _ShellDisplayThreads(const char* args);
static void _ShellHelp(const char* args);
static void _ShellList(const char* args);
static void _ShellCat(const char* args);
static void _ShellMount(const char* args);
static void _ShellTest(const char* args);
static void _ShellPanic(const char* args);
static void _ShellSleep(const char* args);
static void _ShellGetMapping(const char* args);
static void _ShellExecuteCommand(void);
static void _ShellGetCommand(void);

/**
 * @brief Entry point for the kernel shell thread.
 *
 * @details Entry point for the kernel shell thread. This function is executed
 * in a separate thread and handles the shell input and command execution.
 *
 * @param[in] args Pointer to the arguments passed to the shell thread, usually
 * NULL.
 *
 * @return This function does not return. It runs indefinitely until the shell
 * terminates the thread.
 */
static void* _ShellEntry(void* args);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Input buffer for the shell. */
static char sInputBuffer[SHELL_INPUT_BUFFER_SIZE + 1];
/** @brief Cursor position in the input buffer. */
static size_t sInputBufferCursor;
/** @brief Array of available shell commands. */
static const S_ShellCommand sCommands[] =
{
  {"tasks", "Display the current threads", _ShellDisplayThreads},
  {"timePrec", "Timer precision test", _ShellTimeTest},
  {"ls", "List files in a path", _ShellList},
  {"mount", "Mount a device", _ShellMount},
  {"cat", "Cat a file", _ShellCat},
  {"test", "Current dev test for testing purpose", _ShellTest},
  {"panic", "Generates a kernel panic", _ShellPanic},
  {"sleep", "Sleeps for ns time", _ShellSleep},
  {"map", "Get a thread memory mapping", _ShellGetMapping},
  {"help", "Display this help", _ShellHelp},
  {NULL, NULL, NULL}
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _ShellSleep(const char* args)
{
  uint64_t time;
  E_Return error;

  time = strtoul(args, NULL, 10);
  error = SleepNs(time);

  if (error != NO_ERROR)
  {
    KPrintf("Error: %d\n", error);
  }
}

static void _ShellPanic(const char* args)
{
  (void)args;
  PANIC(NO_ERROR, "KERNEL_Shell", "Kernel Shell Panic Generator", false, false);
}

static void _ShellTest(const char* args)
{
  (void)args;

  _ShellMount("/dev/storage/ramdisk0 /initrd ustar");
  _ShellList("/initrd");


  KPrintf("Kernel Shell Test Command Executed\n");
}

static void _ShellCat(const char* args)
{
  int32_t fd;
  int32_t ret;
  char    buffer[100];

  fd = VFSOpen(args, O_RDONLY, 0);
  if (fd >= 0)
  {
    while ((ret = VFSRead(fd, buffer, 99)) > 0)
    {
      buffer[ret] = 0;
      KPrintf("%s", buffer);
    }
    if (ret == -1)
    {
      KPrintf("\nError reading file: %s\n", args);
    }
    else
    {
      KPrintf("\n");
    }

    ret = VFSClose(fd);
    if (ret != 0)
    {
      KPrintf("Error closing file: %s\n", args);
    }
  }
  else
  {
    KPrintf("Failed to open %s\n", args);
  }
}

static void _ShellMount(const char* args)
{
  E_Return retVal;
  uint32_t i;
  uint32_t lastCpy;
  size_t   length;
  uint8_t  copyIndex;
  char     argsVal[3][128];

  lastCpy   = 0;
  copyIndex = 0;
  length    = strnlen(args, SHELL_INPUT_BUFFER_SIZE);
  for (i = 0; i < length; ++i)
  {
    if (args[i] == ' ')
    {
      memcpy(argsVal[copyIndex], args + lastCpy, i - lastCpy);
      argsVal[copyIndex][i - lastCpy] = 0;
      ++copyIndex;
      lastCpy = i + 1;
    }
    else if (i == length - 1)
    {
      memcpy(argsVal[copyIndex], args + lastCpy, i - lastCpy + 1);
      argsVal[copyIndex][i - lastCpy + 1] = 0;
      ++copyIndex;
      lastCpy = i + 1;
    }
  }

  if (copyIndex != 3)
  {
    KPrintf("Error: mount <dev_path> <dir_path> <fs_name>\n");
    return;
  }

  KPrintf("Mouting %s to %s (fs: %s)\n", argsVal[0], argsVal[1], argsVal[2]);

  retVal = VFSMount(argsVal[1], argsVal[0], argsVal[2]);
  if (retVal != NO_ERROR)
  {
    KPrintf("Failed to mount: %d\n", retVal);
  }
}

static void _ShellList(const char* args)
{
  int32_t          fd;
  S_DirectoryEntry dirEnt;
  int32_t          ret;

  fd = VFSOpen(args, O_RDONLY, 0);
  if (fd < 0)
  {
    KPrintf("Failed to open %s\n", args);
    return;
  }

  while (VFSReaddir(fd, &dirEnt) >= 0)
  {
    KPrintf("%s\n", dirEnt.pName);
  }

  ret = VFSClose(fd);
  if (ret != 0)
  {
    KPrintf("Error closing file: %s\n", args);
  }
}

static void _ShellHelp(const char* args)
{
  (void)args;
  size_t i;

  for (i = 0; sCommands[i].pCommandName != NULL; ++i)
  {
    KPrintf("%s - %s\n", sCommands[i].pCommandName, sCommands[i].pDescription);
  }
}

static void _ShellTimeTest(const char* args)
{
  (void)args;

  uint32_t i;
  uint64_t time;
  uint64_t s;
  uint64_t ms;
  uint64_t us;
  uint64_t ns;
  uint64_t oldTime = TimeGetUptime();
  uint64_t tmp;
  for (i = 0; i < 1000; ++i)
  {
    tmp = TimeGetUptime();
    time = tmp - oldTime;
    oldTime = tmp;
    s = time / 1000000000;
    ms = (time % 1000000000) / 1000000;
    us = (time % 1000000) / 1000;
    ns = (time % 1000);
    KPrintf("Time: %llu | %llu.%llu.%llu.%llu\n", time, s, ms, us, ns);
  }
}

#if 0
static void _ShellDisplayThreads(const char* args)
{
  (void)args;
  size_t       threadCount;
  size_t       i;
  uint32_t     j;
  size_t       prio;
  int32_t*     pThreadTable;
  S_ThreadInfo threadInfo;

  threadCount  = SchedulerGetThreadCount();
  pThreadTable = KMalloc(sizeof(int32_t) * threadCount,
                         KMALLOC_FREE_POOL,
                         ALIGN_4_BYTES);
  if (pThreadTable == NULL)
  {
    KPrintf("Unable to allocate thread table memory.\n");
    return;
  }

  threadCount = SchedulerGetThreadsIds(pThreadTable, threadCount);
  KPrintf("#---------------------------------------------------------------------------------------------------------#\n");
  KPrintf("|  PID  |  TID  | NAME                           | TYPE   | PRIO | STATE    | CPU | STACKS                |\n");
  KPrintf("#---------------------------------------------------------------------------------------------------------#\n");
  for (prio = KERNEL_HIGHEST_PRIORITY; prio <= KERNEL_LOWEST_PRIORITY; ++prio)
  {
    for (i = 0; i < threadCount; ++i)
    {
      if (SchedulerGetThreadInfo(&threadInfo, pThreadTable[i]) != NO_ERROR)
      {
        continue;
      }
      if (threadInfo.priority != prio)
      {
        continue;
      }
      KPrintf("| % 5d | % 5d | %s",
              threadInfo.pid,
              threadInfo.tid,
              threadInfo.pName);
      for (j = 0; j < THREAD_NAME_MAX_LENGTH - strnlen(threadInfo.pName, THREAD_NAME_MAX_LENGTH) - 1; ++j)
      {
        KPrintf(" ");
      }
      switch(threadInfo.type)
      {
        case THREAD_TYPE_KERNEL:
          KPrintf("| KERNEL |");
          break;
        case THREAD_TYPE_USER:
          KPrintf("| USER   |");
          break;
        default:
          KPrintf("| NONE   |");
          break;
      }
      KPrintf("  % 3d |", threadInfo.priority);
      switch(threadInfo.currentState)
      {
        case THREAD_STATE_RUNNING:
          KPrintf(" RUNNING  |");
          break;
        case THREAD_STATE_READY:
          KPrintf(" READY    |");
          break;
        case THREAD_STATE_SLEEPING:
          KPrintf(" SLEEPING |");
          break;
        case THREAD_STATE_ZOMBIE:
          KPrintf(" ZOMBIE   |");
          break;
        case THREAD_STATE_JOINING:
          KPrintf(" JOINING  |");
          break;
        case THREAD_STATE_WAITING:
          KPrintf(" WAITING  |");
          break;
        default:
          KPrintf(" UNKNOWN  |");
          break;
      }
      if (threadInfo.currentState == THREAD_STATE_RUNNING)
      {
        KPrintf(" % 3d |", threadInfo.schedCpu);
      }
      else
      {
        KPrintf("   * |", threadInfo.schedCpu);
      }
      KPrintf(" K: 0x%P |\n", threadInfo.kStack);
      if (threadInfo.uStack != (uintptr_t)NULL)
      {
        KPrintf("|       |       |                                |        |      |          |     | U: 0x%P |\n", threadInfo.uStack);
      }
      KPrintf("#---------------------------------------------------------------------------------------------------------#\n");
    }
  }

  KFree(pThreadTable, KMALLOC_FREE_POOL);
}
#else
static void _ShellDisplayThreads(const char* args)
{
  (void)args;

  KPrintf("Command 'tasks' is disabled in this build.\n");
}
#endif

static void _ShellGetMapping(const char* args)
{
#if 0
  int32_t          tid;
  size_t           infoSize;
  size_t           i;
  S_MemoryPageInfo infos[20];
  S_KernelThread*  pThread;

  infoSize = 20;

  tid = strtol(args, NULL, 10);
  pThread = SchedulerGetThread(tid);
  if (pThread == NULL)
  {
    KPrintf("Cannot find thread with ID %d.\n", tid);
    return;
  }

  KPrintf("Thread ID: %d | Name: %s\n", pThread->tid, pThread->pName);

  /* Get the thread mapping */
  MemoryGetPagesInfo(pThread, infos, &infoSize);
  KPrintf("#--------------------------------------------------------------#\n");
  KPrintf("|      Physical      |      Virtual       |       Flags        |\n");
  KPrintf("#--------------------------------------------------------------#\n");

  for (i = 0; i < infoSize; ++i)
  {
      KPrintf("| 0x%p | 0x%p | 0x%p |\n",
              infos[i].physAddress,
              infos[i].virtAddress,
              infos[i].flags);
  }
  KPrintf("#--------------------------------------------------------------#\n");
#else
  (void)args;
  KPrintf("Command 'map' is disabled in this build.\n");
#endif
}

static void _ShellExecuteCommand(void)
{
  size_t cursor;
  size_t i;
  char   command[SHELL_INPUT_BUFFER_SIZE + 1];
  char*  args;

  if (sInputBufferCursor == 0)
  {
    return;
  }

  for (cursor = 0; cursor < sInputBufferCursor; ++cursor)
  {
    if (sInputBuffer[cursor] == ' ')
    {
      break;
    }
    else
    {
      command[cursor] = sInputBuffer[cursor];
    }
  }

  command[cursor] = 0;
  if (cursor == sInputBufferCursor)
  {
    args = "\0";
  }
  else
  {
    args = &sInputBuffer[cursor + 1];
  }

  for (i = 0; sCommands[i].pCommandName != NULL; ++i)
  {
    if (strcmp(command, sCommands[i].pCommandName) == 0)
    {
      sCommands[i].pFunc(args);
      break;
    }
  }

  if (sCommands[i].pCommandName == NULL)
  {
    KPrintf("Unknown command: %s\n", command);
  }
}

static void _ShellGetCommand(void)
{
  ssize_t       readCount;
  char          readChar;
  S_ColorScheme saveScheme;
  S_ColorScheme promptScheme;

  sInputBufferCursor = 0;
  promptScheme.background = BG_BLACK;
  promptScheme.foreground = FG_CYAN;
  ConsoleGetColorScheme(&saveScheme);
  ConsoleSetColorScheme(&promptScheme);
  KPrintf(">");
  KPrintfFlush();
  ConsoleSetColorScheme(&saveScheme);
  KPrintf(" ");
  KPrintfFlush();
  while (true)
  {
    readCount = ConsoleRead(&readChar, 1);
    if (readCount > 0)
    {
      if (readChar == 0xD || readChar == 0xA)
      {
        KPrintf("\n");
        break;
      }
      else if (readChar == 0x7F || readChar == '\b')
      {
        if (sInputBufferCursor > 0)
        {
          --sInputBufferCursor;
          KPrintf("\b \b");
          KPrintfFlush();
        }
      }
      else if (sInputBufferCursor < SHELL_INPUT_BUFFER_SIZE)
      {
        sInputBuffer[sInputBufferCursor] = readChar;
        ++sInputBufferCursor;
        KPrintf("%c", readChar);
        KPrintfFlush();
      }
    }
  }
  sInputBuffer[sInputBufferCursor] = 0;
}

static void* _ShellEntry(void* args)
{
  (void)args;

  /* Wait for all the system to be up */
  SleepNs(100000000);

  KPrintf("\n==== ROOS Kernel Shell ==== Version %s\n", SHELL_VERSION);

  while (true)
  {
    _ShellGetCommand();
    _ShellExecuteCommand();
  }

  return (void*)0;
}

void KernelShellInit(void)
{
  E_Return        error;
  S_KernelThread* pShellThread;
  S_CPUMask       cpuMask;
  char            name[32] = "Kernel Shell\0";
  uint32_t        coreCount;

  CPU_MASK_RESET(cpuMask);
  for (coreCount = 0; coreCount < CPUGetCount(); ++coreCount)
  {
    CPU_MASK_SET(cpuMask, coreCount);
  }


  /* We don't keep the kernel shell thread handle, it the child of the main
   * kernel thread (IDLE) and will be fully destroyed on exit, without need
   * of join.
   */
  error = CreateThread(&pShellThread,
                       true,
                       10,
                       name,
                       0x1000,
                       cpuMask,
                       _ShellEntry,
                       NULL,
                       NULL);
  if (error != NO_ERROR)
  {
    KERNEL_ERROR("Failed to create kernel shell thread. Error: %d\n", error);
  }
}

/************************************ EOF *************************************/