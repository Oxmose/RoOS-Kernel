/*******************************************************************************
 * @file stdio.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Kernel's standard input/output lib functions.
 *
 * @details Standard input/output lib functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_STDIO_H_
#define __LIB_STDIO_H_

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
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/** @brief Returns the current value of the stdout variable. */
#define stdout (GetStdout())

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
 * @brief Returns the current value of the stdout variable.
 *
 * @return int The current value of the stdout variable.
 */
int GetStdout(void);

/**
 * @brief Prints a formatted string to the standard output.
 *
 * @details Prints a formatted string to the standard output. This function is
 * similar to the standard C library's printf function.
 *
 * @param[in] format The format string.
 * @param[in] ... The arguments to be formatted.
 *
 * @return int The number of characters printed, or a negative value if an error
 * occurred.
 */
int printf(const char *format, ...);

#endif /* #ifndef __LIB_STDIO_H_ */

/************************************ EOF *************************************/
