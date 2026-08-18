/*******************************************************************************
 * @file KernelOutput.c
 *
 * @see KernelOutput.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.1
 *
 * @brief Kernel's output methods.
 *
 * @details Simple output functions to print messages to screen. These are
 * really basic output too allow early kernel boot output and debug.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <Console.h>
#include <Critical.h>
#include <TimerManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <KernelOutput.h>

/* Unit test header */
/* None TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the maximal size of the buffer before sending for a print. */
#define KPRINTF_BUFFER_SIZE 256

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Output descriptor, used to define the handlers that manage outputs */
typedef struct
{
  /** @brief The handler used to print character. */
  void (*pPutc)(const char);
  /** @brief The handler used to print string. */
  void (*pPuts)(const char*);
  /** @brief The handler used to flush the output. */
  void (*pFlush)(void);
} S_KOutputDescriptor;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Prints the tag for kernel output functions.
 *
 * @details Prints the tag for kernel output functions.
 *
 * @param[in] kpStr The formated string to print.
 * @param[in] ... The associated arguments to the formated string.
 */
static void _TagPrintf(const char* kpStr, ...);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the current output type. */
static S_KOutputDescriptor sCurrentOutput =
{
  .pPutc  = ConsolePutChar,
  .pPuts  = ConsolePutString,
  .pFlush = ConsoleFlush
};

/** @brief Stores the current buffer size */
static size_t sBufferSize = 0;

/** @brief Stores the current buffer size */
static char spBuffer[KPRINTF_BUFFER_SIZE];

/** @brief Kernel output lock */
static S_KernelSpinlock sLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _TagPrintf(const char* kpFmt, ...)
{
  __builtin_va_list args;

  if (kpFmt != NULL)
  {
    /* Format the string */
    __builtin_va_start(args, kpFmt);
    sBufferSize = vsnprintf(spBuffer + sBufferSize,
              KPRINTF_BUFFER_SIZE - sBufferSize,
              kpFmt,
              args);
    __builtin_va_end(args);

    /* Flush */
    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;
  }
}

void KPrintf(const char* kpFmt, ...)
{
    __builtin_va_list args;

    if (kpFmt != NULL)
    {
        return;
    }

    KERNEL_LOCK(sLock);

    /* Format the string */
    __builtin_va_start(args, kpFmt);
    sBufferSize = vsnprintf(spBuffer + sBufferSize,
              KPRINTF_BUFFER_SIZE - sBufferSize,
              kpFmt,
              args);
    __builtin_va_end(args);

    /* Flush */
    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;

    KERNEL_UNLOCK(sLock);
}

void KPrintfError(const char* kpFmt, ...)
{
  __builtin_va_list args;
  S_ColorScheme     buffer;
  S_ColorScheme     newScheme;

  if (kpFmt != NULL)
  {
    KERNEL_LOCK(sLock);

    newScheme.foreground = FG_RED;
    newScheme.background = BG_BLACK;

    /* No need to test return value */
    ConsoleGetColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    ConsoleSetColorScheme(&newScheme);

    /* Print tag */
    _TagPrintf("[ERROR] ");

    /* Restore original screen color scheme */
    ConsoleSetColorScheme(&buffer);

    /* Format the string */
    __builtin_va_start(args, kpFmt);
    sBufferSize = vsnprintf(spBuffer + sBufferSize,
              KPRINTF_BUFFER_SIZE - sBufferSize,
              kpFmt,
              args);
    __builtin_va_end(args);

    /* Flush */
    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;

    KERNEL_UNLOCK(sLock);
  }
}

void KPrintfSuccess(const char* kpFmt, ...)
{
  __builtin_va_list args;
  S_ColorScheme     buffer;
  S_ColorScheme     newScheme;

  if (kpFmt != NULL)
  {
    KERNEL_LOCK(sLock);

    newScheme.foreground = FG_GREEN;
    newScheme.background = BG_BLACK;

    /* No need to test return value */
    ConsoleGetColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    ConsoleSetColorScheme(&newScheme);

    /* Print tag */
    _TagPrintf("[OK] ");

    /* Restore original screen color scheme */
    ConsoleSetColorScheme(&buffer);

    /* Format the string */
    __builtin_va_start(args, kpFmt);
    sBufferSize = vsnprintf(spBuffer + sBufferSize,
              KPRINTF_BUFFER_SIZE - sBufferSize,
              kpFmt,
              args);
    __builtin_va_end(args);

    /* Flush */
    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;

    KERNEL_UNLOCK(sLock);
  }
}

void KPrintfInfo(const char* kpFmt, ...)
{
  __builtin_va_list args;
  S_ColorScheme     buffer;
  S_ColorScheme     newScheme;

  if (kpFmt != NULL)
  {
    KERNEL_LOCK(sLock);

    newScheme.foreground = FG_CYAN;
    newScheme.background = BG_BLACK;

    /* No need to test return value */
    ConsoleGetColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    ConsoleSetColorScheme(&newScheme);

    /* Print tag */
    _TagPrintf("[INFO] ");

    /* Restore original screen color scheme */
    ConsoleSetColorScheme(&buffer);

    /* Format the string */
    __builtin_va_start(args, kpFmt);
    sBufferSize = vsnprintf(spBuffer + sBufferSize,
              KPRINTF_BUFFER_SIZE - sBufferSize,
              kpFmt,
              args);
    __builtin_va_end(args);

    /* Flush */
    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;

    KERNEL_UNLOCK(sLock);
  }
}

void KPrintfDebug(const char* kpFmt, ...)
{
  __builtin_va_list args;
  S_ColorScheme     buffer;
  S_ColorScheme     newScheme;
  uint64_t          uptime;

  if (kpFmt != NULL)
  {
    KERNEL_LOCK(sLock);

    newScheme.foreground = FG_YELLOW;
    newScheme.background = BG_BLACK;

    /* No need to test return value */
    ConsoleGetColorScheme(&buffer);

    /* Set REG on BLACK color scheme */
    ConsoleSetColorScheme(&newScheme);

    /* Print tag */
    uptime = TimeGetUptime();
    _TagPrintf("[DEBUG %d | %02llu.%03llu.%03llu.%03llu]",
                CPUGetId(),
                uptime / 1000000000,
                (uptime / 1000000) % 1000,
                (uptime / 1000) % 1000,
                uptime % 1000);

    /* Restore original screen color scheme */
    ConsoleSetColorScheme(&buffer);

    /* Format the string */
    __builtin_va_start(args, kpFmt);
    sBufferSize = vsnprintf(spBuffer + sBufferSize,
              KPRINTF_BUFFER_SIZE - sBufferSize,
              kpFmt,
              args);
    __builtin_va_end(args);

    /* Flush */
    spBuffer[sBufferSize] = 0;
    sCurrentOutput.pPuts(spBuffer);
    sBufferSize = 0;

    KERNEL_UNLOCK(sLock);
  }
}

void KPrintfFlush(void)
{
  KERNEL_LOCK(sLock);

  spBuffer[sBufferSize] = 0;
  sCurrentOutput.pPuts(spBuffer);
  sBufferSize = 0;

  KERNEL_UNLOCK(sLock);
}

/************************************ EOF *************************************/