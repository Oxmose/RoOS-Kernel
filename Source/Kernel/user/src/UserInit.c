/*******************************************************************************
 * @file UserInit.c
 *
 * @see UserInit.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief Kernel's user entry point.
 *
 * @details Kernel's user entry point. This file gather the functions called
 * by the kernel just before starting the scheduler and executing the test.
 * Users can use this function to add relevant code to their applications'
 * initialization or for other purposes.
 *
 * @warning All interrupts are disabled when calling the user initialization
 * functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <VirtualFS.h>
#include <Scheduler.h>
#include <ELFManager.h>
#include <KernelError.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <UserInit.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the current module name */
#define MODULE_NAME "USERINIT"
/** @brief Defines the init ram disk device path */
#define INITRD_DEV_PATH "/dev/storage/ramdisk0"
/** @brief Defines the init ram disk mount point */
#define INITRD_MNT_PATH "/initrd"
/** @brief Defines the init process config file path */
#define INIT_CONFIG_PATH "/initrd/.roos_init"
/** @brief Defines the init ELF path configuration variable */
#define CONF_INIT_PATH_VAR_NAME "INIT="
/** @brief Defines the init ELF path configuration variable length */
#define CONF_INIT_PATH_VAR_NAME_LEN 5
/** @brief Setup the INIT process priority */
#define INIT_MAIN_THREAD_PRIO 20

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Creates the init process.
 *
 * @details Creates the init process. The init process ELF is loaded from the
 * ramdisk and started.
 *
 * @return The function returns the success or error status.
 */
static E_Return _CreateInit(void);

/**
 * @brief Reads a line from the file descriptor.
 *
 * @details Reads a line from the file descriptor. Puts the line in the buffer.
 * The function copies data until the line feed character is encountered in the
 * file of the buffer size is exceeded.
 *
 * @param[in] kFd The file descriptor to use.
 * @param[out] pBuffer The buffer to fill with the line.
 * @param[in] kSize The size of the buffer.
 *
 * @return The function returns the line size.
 */
static ssize_t _ReadLine(const int32_t kFd, char* pBuffer, const size_t kSize);

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
static ssize_t _ReadLine(const int32_t kFd, char* pBuffer, const size_t kSize)
{
  ssize_t read;
  ssize_t readBytes;

  read = 0;
  while((size_t)read < kSize - 1)
  {
    readBytes = VFSRead(kFd, pBuffer + read, 1);
    if(readBytes < 0)
    {
      readBytes = -1;
      break;
    }
    else if(readBytes == 0)
    {
      break;
    }
    else if(pBuffer[read] == '\n')
    {
      ++read;
      break;
    }

    ++read;
  }

  if (readBytes >= 0)
  {
    pBuffer[read] = 0;
  }
  else
  {
    read = -1;
  }

  return (ssize_t)read;
}

static E_Return _CreateInit(void)
{
  S_KernelThread*  pInitThread;
  S_KernelProcess* pInitProcess;
  int32_t          fileFd;
  char*            pInitPath;
  char             pBuffer[512];
  ssize_t          readBytes;
  uintptr_t        entryPoint;
  E_Return         error;
  S_CPUMask        cpuMask;
  const char       pName[32] = "init";
  uint32_t         i;

  /* Open the init config */
  fileFd = VFSOpen(INIT_CONFIG_PATH, O_RDONLY, 0);
  if(fileFd >= 0)
  {
    /* Read the configuration */
    pInitPath = NULL;
    while(true)
    {
      readBytes = _ReadLine(fileFd, pBuffer, 512);
      if(readBytes > 0 &&readBytes < 512)
      {
        /* Try to get the init configuration */
        if(strncmp(pBuffer,
                   CONF_INIT_PATH_VAR_NAME,
                   CONF_INIT_PATH_VAR_NAME_LEN) == 0)
        {
            pInitPath = pBuffer + CONF_INIT_PATH_VAR_NAME_LEN;
            break;
        }
      }
      else if(readBytes < 0)
      {
        KERNEL_ERROR("Failed to read init configuration\n");
        error = ERR_INVALID_VALUE;
        break;
      }
      else if(readBytes == 0)
      {
        break;
      }
      else
      {
        KERNEL_ERROR("Configuration line is greater than 511 character.\n");
        error = ERR_INVALID_VALUE;
        break;
      }

    }

    fileFd = VFSClose(fileFd);
    if(fileFd >=  0)
    {
      if (pInitPath != NULL)
      {
        KERNEL_INFO("Loading init from %s\n", pInitPath);

        /* Create the init process */
        error = CreateInitProcess(&pInitProcess);
        if (error == NO_ERROR)
        {
          error = ELFManagerLoadElf(pInitPath, &entryPoint, pInitProcess);
          if (error == NO_ERROR)
          {
            /* Create the init thread */
            CPU_MASK_RESET(cpuMask);
            for (i = 0; i < CPUGetCount(); ++i)
            {
              CPU_MASK_SET(cpuMask, i);
            }
            error = CreateThread(&pInitThread,
                                 false,
                                 INIT_MAIN_THREAD_PRIO,
                                 pName,
                                 KERNEL_STACK_SIZE,
                                 cpuMask,
                                 (void*)entryPoint,
                                 NULL,
                                 pInitProcess);
            if (error == NO_ERROR)
            {
              KERNEL_INFO("Init thread created\n");
            }
            else
            {
              KERNEL_ERROR("Failed to create init thread, error %d\n", error);
            }
          }
          else
          {
            KERNEL_ERROR("Failed to load init ELF, error %d\n", error);
          }
        }
        else
        {
          KERNEL_ERROR("Failed to create init process, error %d\n", error);
        }
      }
      else
      {
        KERNEL_ERROR("Failed to close init configuration, error %d\n", fileFd);
      }
    }
    else
    {
      KERNEL_ERROR("Failed to get init path from configuration\n");
      error = ERR_INVALID_VALUE;
    }
  }
  else
  {
    KERNEL_ERROR("Failed to open init configuration, error %d\n", fileFd);
    error = ERR_INVALID_VALUE;
  }

  return error;
}

void UserInit(void)
{
  E_Return error;

  /* Mount the init ram disk */
  error = VFSMount(INITRD_MNT_PATH, INITRD_DEV_PATH, "ustar");
  if(error == NO_ERROR)
  {
    /* Create the init process */
    error = _CreateInit();
    if(error != NO_ERROR)
    {
      KERNEL_ERROR("Failed to create init process, error %d\n", error);
    }
  }
  else
  {
    KERNEL_ERROR("Failed to mount init ramdisk, error %d\n", error);
  }
}

/************************************ EOF *************************************/