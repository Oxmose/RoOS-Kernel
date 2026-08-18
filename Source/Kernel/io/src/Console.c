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
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <DeviceTree.h>
#include <DebugOutput.h>
#include <DriverManager.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <Console.h>

/* Unit test header */
/* None TODO */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the current module name */
#define MODULE_NAME "CONS"

/** @brief FDT console node name */
#define FDT_CONSOLE_NODE_NAME "console"
/** @brief FDT property for the console input device property */
#define FDT_CONSOLE_INPUT_DEV_PROP "inputdev"
/** @brief FDT property for the console output device property */
#define FDT_CONSOLE_OUTPUT_DEV_PROP "outputdev"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Execute a function if it exists.
 *
 * @param[in] FUNC The function to execute.
 * @param[in, out] ... Function parameters.
 */
#define EXEC_IF_SET(DRIVER, FUNC, ...) { \
  if (DRIVER.FUNC != NULL)                \
  {                                      \
    DRIVER.FUNC(__VA_ARGS__);            \
  }                                      \
}

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
/** @brief Stores the console driver */
static S_ConsoleDriver sConsoleDriver;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void ConsoleInit(void)
{
  memset(&sConsoleDriver, 0, sizeof(S_ConsoleDriver));
}

void ConsoleSetDriver(const S_ConsoleDriver* kpDriver)
{
  sConsoleDriver = *kpDriver;
}

void ConsoleClear(void)
{
  EXEC_IF_SET(sConsoleDriver, pClear);
}

void ConsoleGetCursor(S_ConsoleCursor* pBuffer)
{
  EXEC_IF_SET(sConsoleDriver, pGetCursor, pBuffer);
}

void ConsoleSetCursor(const S_ConsoleCursor* pkBuffer)
{
  EXEC_IF_SET(sConsoleDriver, pSetCursor, pkBuffer);
}

void ConsoleScroll(const E_ScrollDirection kDirection, const uint32_t kLines)
{
  EXEC_IF_SET(sConsoleDriver, pScroll, kDirection, kLines);
}

void ConsoleSetColorScheme(const S_ColorScheme* pkColorScheme)
{
  EXEC_IF_SET(sConsoleDriver, pSetColorScheme, pkColorScheme);
}

void ConsoleGetColorScheme(S_ColorScheme* pBuffer)
{
  EXEC_IF_SET(sConsoleDriver, pGetColorScheme, pBuffer);
}

void ConsolePutString(const char* pkString)
{
  EXEC_IF_SET(sConsoleDriver, pPutString, pkString);
#if OUTPUT_DEBUG_ENABLE
  DebugOutputPutString(pkString);
#endif
}

void ConsolePutChar(const char kCharacter)
{
  EXEC_IF_SET(sConsoleDriver, pPutChar, kCharacter);
#if OUTPUT_DEBUG_ENABLE
  DebugOutputPutChar(kCharacter);
#endif
}

ssize_t ConsoleRead(char* pBuffer, size_t kBufferSize)
{
  if (sConsoleDriver.pRead != NULL)
  {
    return sConsoleDriver.pRead(pBuffer, kBufferSize);
  }
  else
  {
    return -1;
  }
}

void ConsoleFlush(void)
{
  EXEC_IF_SET(sConsoleDriver, pFlush);
}

/************************************ EOF *************************************/