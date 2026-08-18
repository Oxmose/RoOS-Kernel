/*******************************************************************************
 * @file DebugOutput.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/06/2026
 *
 * @version 2.0
 *
 * @brief Debug output functionalities.
 *
 * @details Debug output functionalities. This file declares the functions that
 * can be used for debug output. Those functions should be implemented by the
 * BSP.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __BSP_DEBUG_OUTPUT_H_
#define __BSP_DEBUG_OUTPUT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <config.h>

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
#if OUTPUT_DEBUG_ENABLE
#define DEBUG_OUTPUT(STR) DebugOutputPutString(STR)
#else
#define DEBUG_OUTPUT(STR)
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

#if OUTPUT_DEBUG_ENABLE
/**
 * @brief Initializes the UART debugging driver.
 *
 * @details Initializes the UART debugging driver. This UART is used to output
 * debugging printfs used by the kernel an drivers.
 */
void DebugOutputInit(void);

/**
 * @brief Write the string given as patameter on the debug port.
 *
 * @details The function will output the data given as parameter on the debug
 * port. This call is blocking until the data has been sent to the uart port
 * controler.
 *
 * @param[in, out] pDrvCtrl The UART driver controler to use.
 * @param[in] kpString The string to write to the uart port.
 *
 * @warning string must be NULL terminated.
 */
void DebugOutputPutString(const char* kpString);

/**
 * @brief Write the character given as patameter on the debug port.
 *
 * @details The function will output the character given as parameter on the
 * debug port. This call is blocking until the data has been sent to the uart
 * port controler.
 *
 * @param[in, out] pDrvCtrl The UART driver controler to use.
 * @param[in] kCharacter The character to write to the uart port.
 */
void DebugOutputPutChar(const char kCharacter);
#endif

#endif /* #ifndef __BSP_DEBUG_OUTPUT_H_ */

/************************************ EOF *************************************/
