/*******************************************************************************
 * @file unistd.h
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

#ifndef __LIB_UNISTD_H_
#define __LIB_UNISTD_H_

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
 * @brief Causes the calling thread to sleep.
 *
 * @details Causes the calling thread to sleep either until the
 * number of real-time seconds specified in seconds have elapsed or
 * until a signal arrives which is not ignored.
 *
 * @param[in] seconds The number of seconds to sleep.
 *
 * @return Zero if the requested time has elapsed, or the number of seconds
 *  left to sleep, if the call was interrupted by a signal handler.
 */
unsigned int sleep(unsigned int seconds);

/**
 * @brief Suspend execution for microsecond intervals.
 *
 * @details Suspend execution of the calling thread for (at least) usec
 * microseconds. The sleep may be lengthened slightly by any system activity or
 * by the time spent processing the call or by the granularity of system timers.
 *
 * @param[in] usec The number of microseconds to sleep.
 *
 * @return  The usleep() function returns 0 on success. On error, -1 is
 * returned, with errno set to indicate the error.
 */
int usleep(useconds_t usec);

/**
 * @brief Write to a file descriptor.
 *
 * @details Write up to count bytes from the buffer starting at buf to the file
 * referred to by the file descriptor fd.
 *
 * @param[in] fd The file descriptor to write to.
 * @param[in] buf The buffer to write from.
 * @param[in] count The number of bytes to write.
 *
 * @return ssize_t The number of bytes written, or -1 on error.
 */
ssize_t write(int fd, const void *buf, size_t count);

/**
 * @brief Read from a file descriptor.
 *
 * @details Read up to count bytes from the file referred to by the file
 * descriptor fd into the buffer starting at buf.
 *
 * @param[in] fd The file descriptor to read from.
 * @param[out] buf The buffer to read into.
 * @param[in] count The number of bytes to read.
 *
 * @return ssize_t The number of bytes read, or -1 on error.
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 * @brief Close a file descriptor.
 *
 * @details Closes the file descriptor fd, so that it no longer refers to any
 * file or other resource.
 *
 * @param[in] fd The file descriptor to close.
 *
 * @return int Returns 0 on success, or -1 on error.
 */
int close(int fd);

#endif /* #ifndef __LIB_UNISTD_H_ */

/************************************ EOF *************************************/