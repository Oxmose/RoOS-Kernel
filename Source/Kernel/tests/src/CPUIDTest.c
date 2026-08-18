/*******************************************************************************
 * @file CPUIDTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 05/08/2026
 *
 * @version 1.0
 *
 * @brief Testing framework CPUID testing.
 *
 * @details Testing framework CPUID testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPUID.h>
#include <string.h>
#include <stdbool.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <TestFramework.h>

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
void CPUIDTest(void)
{
  S_CPUInformation cpuInfo;
  char flagsBuffer[256];
  size_t flagsLen;
  bool hasVendor;
  bool hasName;
  bool hasFamily;
  bool hasFlags;

  memset(&cpuInfo, 0, sizeof(cpuInfo));
  memset(flagsBuffer, 0, sizeof(flagsBuffer));

  CPUIDAnalyzeCPU(&cpuInfo);

  hasVendor = (cpuInfo.pVendor[0] != '\0') && (strlen(cpuInfo.pVendor) > 0);
  hasName = (cpuInfo.pName[0] != '\0') && (strlen(cpuInfo.pName) > 0);
  hasFamily = (cpuInfo.family > 0);
  flagsLen = CPUIDGetFlagsString(flagsBuffer,
                                 sizeof(flagsBuffer),
                                 &cpuInfo.flags);
  hasFlags = (flagsLen > 0) &&
             ((strstr(flagsBuffer, "fpu") != NULL) ||
              (strstr(flagsBuffer, "sse") != NULL) ||
              (strstr(flagsBuffer, "sse2") != NULL));

  TEST_POINT_ASSERT_UINT(TEST_CPUID_VENDOR_ID,
                         hasVendor,
                         1,
                         hasVendor ? 1 : 0,
                         TEST_CPUID_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_CPUID_NAME_ID,
                         hasName,
                         1,
                         hasName ? 1 : 0,
                         TEST_CPUID_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_CPUID_FAMILY_ID,
                         hasFamily,
                         1,
                         hasFamily ? 1 : 0,
                         TEST_CPUID_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_CPUID_LEVEL_ID,
                         cpuInfo.cpuIdLevel > 0,
                         1,
                         cpuInfo.cpuIdLevel > 0 ? 1 : 0,
                         TEST_CPUID_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_CPUID_FLAGS_ID,
                         hasFlags,
                         1,
                         hasFlags ? 1 : 0,
                         TEST_CPUID_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/
