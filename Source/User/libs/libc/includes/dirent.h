/*******************************************************************************
 * @file dirent.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Directory entry functions for roOs.
 *
 * @details Directory entry functions for roOs. This port is not inteded to be
 * complete and provides API for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_DIRENT_H_
#define __LIB_DIRENT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <sys/types.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the directory entry structure */
struct dirent
{
  /** @brief File entry name length */
  unsigned short filenameLength;
  /** @brief Directory entry name  */
  char pName[NAME_MAX + 1];
  /** @brief Directory entry type */
  unsigned char type;
};

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
 * @brief Read a directory entry.
 *
 * @details Read a directory entry. This function will read the directory entry
 * from the directory file descriptor and fill the dirent structure with the
 * information. The function will return the number of bytes read or an error
 * code.
 *
 * @param[in] fd The directory file descriptor.
 * @param[out] pDirent The dirent structure to fill with the information.
 * @param[in] count  The argument count is ignored; at most one dirent
 * structure is read.
 *
 * @return On success, 1 is returned. On end of directory, 0 is returned.
 * On error, -1 is returned, and errno is set appropriately.
 */
int readdir(int fd,
            struct dirent *pDirent,
            unsigned int count);

#endif /* #ifndef __LIB_DIRENT_H_ */

/************************************ EOF *************************************/