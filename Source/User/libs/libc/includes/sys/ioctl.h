/*******************************************************************************
 * @file ioctl.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/10/2024
 *
 * @version 1.0
 *
 * @brief Lib C IOCTL operations for roOs.
 *
 * @details Lib C IOCTL operations for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_SYS_IOCTL_H_
#define __LIB_SYS_IOCTL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>

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
 * @brief Performs an IOCTL operation on a file descriptor.
 *
 * @details Performs an IOCTL operation on a file descriptor. The function sends
 * the IOCTL to the underlying driver to be processed.
 *
 * @param[in] fd The file descriptor of the file to use.
 * @param[in] op The IOCTL operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns the value returned by the IOCTL operation.
 */
int ioctl(int fd, unsigned long op, void* pArgs);

#endif /* #ifndef __LIB_SYS_IOCTL_H_ */

/************************************ EOF *************************************/