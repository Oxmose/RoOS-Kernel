/*******************************************************************************
 * @file ELFManagerTest.c
 *
 * @brief Kernel ELF manager integration tests.
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <Scheduler.h>
#include <VirtualFS.h>
#include <ELFManager.h>
#include <KernelError.h>

/* Header file */
#include <TestFramework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
#define ELF_TEST_DEVICE "/dev/storage/ramdisk0"
#define ELF_TEST_MOUNT  "/initrd"

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void ELFManagerTest(void)
{
  E_Return         error;
  uintptr_t        entryPoint;
  S_KernelProcess* pProcess;

  entryPoint = (uintptr_t)0xDEADC0DE;
  error = ELFManagerLoadElf(NULL, &entryPoint, NULL);
  TEST_POINT_ASSERT_RCODE(TEST_ELFMANAGER_NULL_PATH_ID,
                          error == ERR_INVALID_VALUE,
                          ERR_INVALID_VALUE,
                          error,
                          TEST_ELFMANAGER_ENABLED);

  error = ELFManagerLoadElf(ELF_TEST_MOUNT "/missing",
                            &entryPoint,
                            NULL);
  TEST_POINT_ASSERT_RCODE(TEST_ELFMANAGER_MISSING_ID,
                          error == ERR_INVALID_VALUE,
                          ERR_INVALID_VALUE,
                          error,
                          TEST_ELFMANAGER_ENABLED);

  error = VFSMount(ELF_TEST_MOUNT, ELF_TEST_DEVICE, "ustar");
  TEST_POINT_ASSERT_RCODE(TEST_ELFMANAGER_MOUNT_ID,
                          error == NO_ERROR,
                          NO_ERROR,
                          error,
                          TEST_ELFMANAGER_ENABLED);
  if (error == NO_ERROR)
  {
    entryPoint = (uintptr_t)0xDEADC0DE;
    error = ELFManagerLoadElf(ELF_TEST_MOUNT "/fil1.test",
                              &entryPoint,
                              NULL);
    TEST_POINT_ASSERT_RCODE(TEST_ELFMANAGER_INVALID_ID,
                            error == ERR_INVALID_VALUE,
                            ERR_INVALID_VALUE,
                            error,
                            TEST_ELFMANAGER_ENABLED);

    pProcess = NULL;
    error = CreateInitProcess(&pProcess);
    TEST_POINT_ASSERT_RCODE(TEST_ELFMANAGER_PROCESS_ID,
                            error == NO_ERROR,
                            NO_ERROR,
                            error,
                            TEST_ELFMANAGER_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_ELFMANAGER_PROCESS_ID + 1,
                              error != NO_ERROR || pProcess != NULL,
                              0xDEADC0DE,
                              (uintptr_t)pProcess,
                              TEST_ELFMANAGER_ENABLED);
    if (error == NO_ERROR && pProcess != NULL)
    {
      entryPoint = (uintptr_t)NULL;
      error = ELFManagerLoadElf(ELF_TEST_MOUNT "/init",
                                &entryPoint,
                                pProcess);
      TEST_POINT_ASSERT_RCODE(TEST_ELFMANAGER_LOAD_ID,
                              error == NO_ERROR,
                              NO_ERROR,
                              error,
                              TEST_ELFMANAGER_ENABLED);
      TEST_POINT_ASSERT_POINTER(TEST_ELFMANAGER_ENTRY_ID,
                                error != NO_ERROR || entryPoint != 0,
                                0xDEADC0DE,
                                entryPoint,
                                TEST_ELFMANAGER_ENABLED);
    }
  }

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/