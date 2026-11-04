/*******************************************************************************
 * @file Console.c
 *
 * @see Console.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.0
 *
 * @brief Console drivers abtraction.
 *
 * @details Console driver abtraction layer. The functions of this module allows
 * to abtract the use of any supported console driver and the selection of the
 * desired driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <IOCTL.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <VirtualFS.h>
#include <DeviceTree.h>
#include <DebugOutput.h>
#include <KernelOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <Console.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the current module name */
#define MODULE_NAME "CONS"

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
/** @brief Stores the stdin file descriptor */
static int32_t sStdinFd = -1;
/** @brief Stores the stdout file descriptor */
static int32_t sStdoutFd = -1;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void ConsoleInit(void)
{
  sStdoutFd = VFSOpen(CONFIG_CONSOLE_OUTPUT_DEV, O_RDWR, 0);
  if (sStdoutFd < 0)
  {
    KERNEL_ERROR("Failed to open console output device.\n");
  }
  else
  {
    ConsoleClear();
    KERNEL_INFO("Console output device: %s\n", CONFIG_CONSOLE_OUTPUT_DEV);
  }

  sStdinFd = VFSOpen(CONFIG_CONSOLE_INPUT_DEV, O_RDONLY, 0);
  if (sStdinFd < 0)
  {
    KERNEL_ERROR("Failed to open console input device.\n");
  }
  else
  {
    KERNEL_INFO("Console input device: %s\n", CONFIG_CONSOLE_INPUT_DEV);
  }
}

void ConsoleClear(void)
{
  if (sStdoutFd >= 0)
  {
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_CLEAR, NULL);
  }
}

void ConsoleGetCursor(S_ConsoleCursor* pBuffer)
{
  if (sStdoutFd >= 0)
  {
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_SAVE_CURSOR, pBuffer);
  }
}

void ConsoleSetCursor(const S_ConsoleCursor* pkBuffer)
{
  if (sStdoutFd >= 0)
  {
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_RESTORE_CURSOR, (void*)pkBuffer);
  }
}

void ConsoleScroll(const E_ScrollDirection kDirection, const uint32_t kLines)
{
  S_IOCTLScrollArguments args;

  if (sStdoutFd >= 0)
  {
    args.direction = kDirection;
    args.lineCount = kLines;
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_SCROLL, &args);
  }
}

void ConsoleSetColorScheme(const S_ColorScheme* pkColorScheme)
{
  if (sStdoutFd >= 0)
  {
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_SET_COLORSCHEME, (void*)pkColorScheme);
  }
}

void ConsoleGetColorScheme(S_ColorScheme* pBuffer)
{
  if (sStdoutFd >= 0)
  {
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_SAVE_COLORSCHEME, pBuffer);
  }
}

void ConsolePutString(const char* pkString)
{
  if (sStdoutFd >= 0)
  {
    VFSWrite(sStdoutFd, pkString, strnlen(pkString, 0xFFFFFFFF));
  }
#if OUTPUT_DEBUG_ENABLE
  DebugOutputPutString(pkString);
#endif
}

void ConsolePutChar(const char kCharacter)
{
  if (sStdoutFd >= 0)
  {
    VFSWrite(sStdoutFd, &kCharacter, 1);
  }
#if OUTPUT_DEBUG_ENABLE
  DebugOutputPutChar(kCharacter);
#endif
}

ssize_t ConsoleRead(char* pBuffer, size_t kBufferSize)
{
  ssize_t retVal;

  if (sStdinFd >= 0)
  {
    retVal = VFSRead(sStdinFd, pBuffer, kBufferSize);
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

void ConsoleFlush(void)
{
  if (sStdoutFd >= 0)
  {
    VFSIOCTL(sStdoutFd, VFS_IOCTL_CONS_FLUSH, NULL);
  }
}

/************************************ EOF *************************************/