/*******************************************************************************
 * @file mman.c
 *
 * @see mman.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/09/2026
 *
 * @version 1.0
 *
 * @brief Memory management functions for the roOs kernel.
 *
 * @details Memory management functions for the roOs kernel. Those functions
 * might rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <Syscall.h>
#include <sys/types.h>

/* Header file */
#include <sys/mman.h>

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
void *mmap(void*  addr,
           size_t length,
           int    prot,
           int    flags,
           int    fd,
           off_t  offset)
{
  void*    mapped;
  void*    addrRectified;
  uint64_t parameters;

  parameters = (uint64_t)prot | ((uint64_t)flags << 32);
  addrRectified = addr;
  mapped = Syscall(SYSCALL_ID_MMAP,
                   &addrRectified,
                   (void*)length,
                   (void*)parameters,
                   (void*)(uintptr_t)fd,
                   (void*)offset);
  if (mapped == MAP_FAILED)
  {
    errno = -((uintptr_t)addrRectified);
    mapped = MAP_FAILED;
  }

  return mapped;
}

int munmap(void* addr, size_t length)
{
  int retVal;

  retVal = (int)(uintptr_t)Syscall(SYSCALL_ID_MUNMAP,
                                   addr,
                                   (void*)length,
                                   NULL,
                                   NULL,
                                   NULL);
  if (retVal != 0)
  {
    errno  = -retVal;
    retVal = -1;
  }

  return retVal;
}

/************************************ EOF *************************************/