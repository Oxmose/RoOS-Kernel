/*******************************************************************************
 * @file unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Unistd port for roOs.
 *
 * @details Unistd port for roOs. This port is not inteded to be conplete and
 * provides API for roOs.
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

#endif /* #ifndef __LIB_UNISTD_H_ */

/************************************ EOF *************************************/