/*******************************************************************************
 * @file fcntl.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief File control functions for roOs.
 *
 * @details File control functions for roOs. This port is not inteded to be
 * complete and provides API for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_FCNTL_H_
#define __LIB_FCNTL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <sys/types.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Open for reading only. */
#define O_RDONLY 0x4
/** @brief Open for writing only. */
#define O_WRONLY 0x2
/** @brief Open for reading and writing. */
#define O_RDWR (O_RDONLY | O_WRONLY)

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
 * @brief Opens a file descriptor.
 *
 * @details Opens a file descriptor for the file specified by pathname. The
 * flags argument specifies how the file should be opened, and the mode argument
 * specifies the permissions to use in case a new file is created.
 *
 * @param[in] pathname The path to the file to open.
 * @param[in] flags The flags that specify how the file should be opened.
 * @param[in] mode The permissions to use in case a new file is created.
 *
 * @return Returns the file descriptor on success, or -1 on error.
 */
int open(const char *pathname, int flags, mode_t mode);

#endif /* #ifndef __LIB_FCNTL_H_ */

/************************************ EOF *************************************/