/*******************************************************************************
 * @file sleep.c
 *
 * @see unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Sleep functions familly for the roOs kernel.
 *
 * @details Sleep functions familly for the roOs kernel. Those functions might
 * rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <errno.h>
#include <Syscall.h>
#include <sys/types.h>

/* Header file */
#include <time.h>
#include <unistd.h>

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
unsigned int sleep(unsigned int seconds)
{
  unsigned long long int sleepTimeNs;
  unsigned long long int remainingTimeNs;


  sleepTimeNs = (unsigned long long int)seconds * 1000000000;

  Syscall(SYSCALL_ID_SLEEP,
          (void*)sleepTimeNs,
          &remainingTimeNs,
          NULL,
          NULL,
          NULL);

  return (unsigned int)remainingTimeNs / 1000000000;
}

int usleep(useconds_t usec)
{
  unsigned long long int sleepTimeNs;
  unsigned long long int remainingTimeNs;
  int                    retVal;

  sleepTimeNs = (unsigned long long int)usec * 1000;

  retVal = Syscall(SYSCALL_ID_SLEEP,
                   (void*)sleepTimeNs,
                   &remainingTimeNs,
                   NULL,
                   NULL,
                   NULL);

  /* Set errno on error */
  if (retVal != 0)
  {
    errno  = -retVal;
    retVal = -1;
  }

  return retVal;
}

int nanosleep(const struct timespec *duration, struct timespec *rem)
{
  long long int sleepTimeNs;
  long long int remainingTimeNs;
  int           retVal;

  if (duration != NULL )
  {
    sleepTimeNs = (long long int)duration->tv_sec * 1000000000 +
                  (long long int)duration->tv_nsec;
    if (sleepTimeNs >= 0)
    {
      retVal = Syscall(SYSCALL_ID_SLEEP,
                       (void*)sleepTimeNs,
                       &remainingTimeNs,
                       NULL,
                       NULL,
                       NULL);

      /* Set errno on error */
      if (retVal != 0)
      {
        /* Set remaining time */
        if (retVal == -EINTR && rem != NULL)
        {
          rem->tv_sec  = remainingTimeNs / 1000000000;
          rem->tv_nsec = remainingTimeNs % 1000000000;
        }
        errno  = -retVal;
        retVal = -1;
      }
    }
    else
    {
      errno  = EINVAL;
      retVal = -1;
    }
  }
  else
  {
    errno  = EINVAL;
    retVal = -1;
  }

  return retVal;
}

/************************************ EOF *************************************/