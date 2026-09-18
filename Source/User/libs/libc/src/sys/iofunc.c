/*******************************************************************************
 * @file iofunc.c
 *
 * @see unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief IO function familly for the roOs kernel.
 *
 * @details IO functions familly for the roOs kernel. Those functions might
 * rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <Syscall.h>

/* Header file */
#include <fcntl.h>
#include <stdio.h>
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
/** @brief The thread-local stdout variable. */
static int sSTDout = -1;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
ssize_t write(int fd, const void *buf, size_t count)
{
  ssize_t retVal;

  if (fd >= 0)
  {
    retVal = (ssize_t)(uintptr_t)Syscall(SYSCALL_ID_WRITE,
                                         (void*)(uintptr_t)fd,
                                         (void*)buf,
                                         (void*)count,
                                         NULL,
                                         NULL);
    if (retVal < 0)
    {
      errno = -retVal;
      retVal = -1;
    }
  }
  else
  {
    errno  = EBADF;
    retVal = -1;
  }

  return retVal;
}

ssize_t read(int fd, void *buf, size_t count)
{
  ssize_t retVal;

  if (fd >= 0)
  {
    retVal = (ssize_t)(uintptr_t)Syscall(SYSCALL_ID_READ,
                                         (void*)(uintptr_t)fd,
                                         (void*)buf,
                                         (void*)count,
                                         NULL,
                                         NULL);
    if (retVal < 0)
    {
      errno = -retVal;
      retVal = -1;
    }
  }
  else
  {
    errno  = EBADF;
    retVal = -1;
  }

  return retVal;
}

int close(int fd)
{
  int retVal;

  if (fd >= 0)
  {
    retVal = (int)(uintptr_t)Syscall(SYSCALL_ID_CLOSE,
                                     (void*)(uintptr_t)fd,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
    if (retVal < 0)
    {
      errno = -retVal;
      retVal = -1;
    }
  }
  else
  {
    errno  = EBADF;
    retVal = -1;
  }

  return retVal;
}

int open(const char *pathname, int flags, mode_t mode)
{
  int retVal;

  if (pathname != NULL)
  {
    retVal = (int)(uintptr_t)Syscall(SYSCALL_ID_OPEN,
                                     (void*)pathname,
                                     (void*)(uintptr_t)flags,
                                     (void*)(uintptr_t)mode,
                                     NULL,
                                     NULL);
    if (retVal < 0)
    {
      errno = -retVal;
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

int printf(const char *format, ...)
{
  int               retVal;
  int               bufferSize;
  __builtin_va_list args;
  char*             spBuffer;

  __builtin_va_start(args, format);
  bufferSize = vsnprintf(NULL, 0, format, args);
  __builtin_va_end(args);
  if (bufferSize > 0)
  {
    spBuffer = malloc(bufferSize + 1);
    if (spBuffer == NULL)
    {
      errno  = ENOMEM;
      retVal = -1;
    }
    else
    {
      __builtin_va_start(args, format);
      retVal = vsnprintf(spBuffer, bufferSize + 1, format, args);
      __builtin_va_end(args);
    }
  }
  else
  {
    errno  = EINVAL;
    retVal = -1;
  }
  __builtin_va_end(args);

  if (retVal >= 0)
  {
    retVal = write(stdout, spBuffer, retVal);
    free(spBuffer);
  }

  return retVal;
}

int GetStdout(void)
{
  if (sSTDout == -1)
  {
    /* TODO: Update to use actual stdout */
    sSTDout = open("/dev/vga-text", O_RDWR, 0);
  }
  return sSTDout;
}

/************************************ EOF *************************************/