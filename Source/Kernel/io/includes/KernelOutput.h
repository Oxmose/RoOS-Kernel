/*******************************************************************************
 * @file KernelOutput.h
 *
 * @see KernelOutput.c
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
 * really basic output too allow early kernel boot output and debug. These
 * functions can be used in interrupts handlers since no lock is required to use
 * them. This also makes them non thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __IO_KERNEL_OUTPUT_H_
#define __IO_KERNEL_OUTPUT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <config.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Kernel log level: debug */
#define DEBUG_LOG_LEVEL 3
/** @brief Kernel log level: info */
#define INFO_LOG_LEVEL 2
/** @brief Kernel log level: error */
#define ERROR_LOG_LEVEL 1
/** @brief Kernel log level: none */
#define NONE_LOG_LEVEL 0

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* Defines the output macros that can be enabled or disabled at compile time */
#if KERNEL_LOG_LEVEL >= INFO_LOG_LEVEL
#define KERNEL_INFO(...)    KPrintfInfo(__VA_ARGS__)
#define KERNEL_SUCCESS(...) KPrintfSuccess(__VA_ARGS__)
#else
#define KERNEL_INFO(...)
#define KERNEL_SUCCESS(...)
#endif

#if KERNEL_LOG_LEVEL >= ERROR_LOG_LEVEL
#define KERNEL_ERROR(...) KPrintfError(__VA_ARGS__)
#else
#define KERNEL_ERROR(...)
#endif

#if KERNEL_LOG_LEVEL >= DEBUG_LOG_LEVEL
#define KERNEL_DEBUG(ENABLED, MODULE, STR, ...)                     \
do {                                                                \
  if (ENABLED)                                                      \
  {                                                                 \
    KPrintfDebug(" " MODULE " | " STR " | " __FILE__ ":%d - %s\n",  \
                  ##__VA_ARGS__, __LINE__, __FUNCTION__);           \
  }                                                                 \
} while (0);
#else
#define KERNEL_DEBUG(...)
#endif

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
 * @brief Prints a formated string to the screen.
 *
 * @details Prints the desired string to the screen. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] kpFmt The format string to output.
 * @param[in] ... format's parameters.
 */
void KPrintf(const char* kpFmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a red [ERROR] tag at
 * the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] kpFmt The format string to output.
 * @param[in] ... format's parameters.
 */
void KPrintfError(const char* kpFmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a green [OK] tag at
 * the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] kpFmt The format string to output.
 * @param[in] ... format's parameters.
 */
void KPrintfSuccess(const char* kpFmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a cyan [INFO] tag at
 * the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] kpFmt The format string to output.
 * @param[in] ... format's parameters.
 */
void KPrintfInfo(const char* kpFmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a yellow [DEBUG] tag
 * at the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] kpFmt The format string to output.
 * @param[in] ... format's parameters.
 */
void KPrintfDebug(const char* kpFmt, ...);

/**
 * @brief Flushes the output buffer.
 *
 * @details The output uses a buffer before sending the data to the output
 * device. This function force a push to the device.
 */
void KPrintfFlush(void);

#endif /* #ifndef __IO_KERNEL_OUTPUT_H_ */

/************************************ EOF *************************************/
