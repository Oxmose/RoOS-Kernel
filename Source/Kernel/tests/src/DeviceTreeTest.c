
/*******************************************************************************
 * @file DeviceTreeTest.c
 *
 * @see TestFramework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework device tree testing.
 *
 * @details Testing framework device tree testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <string.h>
#include <stdbool.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <KernelOutput.h>

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
void _WalkNodes(const S_FDTNode* pkNode, const uint8_t kLevel);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
void _WalkNodes(const S_FDTNode* pkNode, const uint8_t kLevel)
{
  uint8_t i;
  const S_FDTProperty* pProp;

  if(pkNode == NULL)
  {
    return;
  }

  for(i = 0; i < kLevel; ++i)
  {
    KPrintf("  ");
  }
  KPrintf("-> %s\n", pkNode->pName);
  pProp = FDTGetFirstProp(pkNode);
  while(pProp != NULL)
  {
    for(i = 0; i < kLevel; ++i)
    {
      KPrintf("  ");
    }

    KPrintf("   | %s\n", pProp->pName);
    pProp = FDTGetNextProp(pProp);
  }
  _WalkNodes(FDTGetChild(pkNode), kLevel + 1);
  _WalkNodes(FDTGetNextNode(pkNode), kLevel);
}

void DeviceTreeTest(void)
{
  const S_FDTNode*       pkNode;
  const S_FDTProperty*   pkProp;
  const void*            pProp;
  size_t                 propLen;
  const S_FDTMemoryNode* pMemNode;

  /* TEST CORRECT PARSING */
  pkNode = FDTGetRoot();
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_PARSE,
                            pkNode != NULL,
                            0xDEADC0DE,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);

  /* TEST FOR WALKING */
  _WalkNodes(pkNode, 0);

  KPrintf("------------ END OF FDT ------------\n");

  /* TEST TO GET ROOT COMPATIBLE */
  pkNode = FDTGetRoot();
  pProp = FDTGetProp(pkNode, "compatible", &propLen);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETPROP0,
                          propLen - 1 == (size_t)strlen("roOs,roOs-fdt-v1"),
                          (size_t)strlen("roOs,roOs-fdt-v1"),
                          propLen - 1,
                          TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_BYTE(TEST_DEVTREE_GETPROP1,
                          strcmp("roOs,roOs-fdt-v1", pProp) == 0,
                          0,
                          strcmp("roOs,roOs-fdt-v1", pProp) == 0,
                          TEST_DEVTREE_ENABLED);


  /* TEST FIRST PROP */
  pkProp = FDTGetFirstProp(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETFIRSTPROP0,
                            pkProp != NULL,
                            0xDEADC0DE,
                            (uintptr_t)pkProp,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETFIRSTPROP1,
                          strcmp(pkProp->pName, "compatible") == 0,
                          0,
                          strcmp(pkProp->pName, "compatible"),
                          TEST_DEVTREE_ENABLED);

  /* TEST NEXT PROP */
  pkProp = FDTGetNextProp(pkProp);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTPROP0,
                            pkProp != NULL,
                            0xDEADC0DE,
                            (uintptr_t)pkProp,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTPROP1,
                          strcmp(pkProp->pName, "#address-cells") == 0,
                          0,
                          strcmp(pkProp->pName, "#address-cells"),
                          TEST_DEVTREE_ENABLED);

  pkProp = FDTGetNextProp(pkProp);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTPROP2,
                            pkProp != NULL,
                            0xDEADC0DE,
                            (uintptr_t)pkProp,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTPROP3,
                          strcmp(pkProp->pName, "#size-cells") == 0,
                          0,
                          strcmp(pkProp->pName, "#size-cells"),
                          TEST_DEVTREE_ENABLED);

  pkProp = FDTGetNextProp(pkProp);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTPROP4,
                            pkProp == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pkProp,
                            TEST_DEVTREE_ENABLED);

  /* TEST FIRST CHILD */
  pkNode = FDTGetChild(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETCHILD0,
                            pkNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETCHILD1,
                          strcmp(pkNode->pName, "cpus") == 0,
                          0,
                          strcmp(pkNode->pName, "cpus"),
                          TEST_DEVTREE_ENABLED);

  pkNode = FDTGetChild(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETCHILD2,
                            pkNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETCHILD3,
                          strcmp(pkNode->pName, "cpu@0") == 0,
                          0,
                          strcmp(pkNode->pName, "cpu@0"),
                          TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETCHILD4,
                            FDTGetChild(pkNode) == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)FDTGetChild(pkNode),
                            TEST_DEVTREE_ENABLED);

  /* TEST NEXT NODE */
  pkNode = FDTGetNextNode(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE0,
                            pkNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTNODE1,
                          strcmp(pkNode->pName, "cpu@1") == 0,
                          0,
                          strcmp(pkNode->pName, "cpu@1"),
                          TEST_DEVTREE_ENABLED);
  pkNode = FDTGetNextNode(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE2,
                            pkNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTNODE3,
                          strcmp(pkNode->pName, "cpu@2") == 0,
                          0,
                          strcmp(pkNode->pName, "cpu@2"),
                          TEST_DEVTREE_ENABLED);
  pkNode = FDTGetNextNode(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE4,
                            pkNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTNODE5,
                          strcmp(pkNode->pName, "cpu@3") == 0,
                          0,
                          strcmp(pkNode->pName, "cpu@3"),
                          TEST_DEVTREE_ENABLED);
  pkNode = FDTGetNextNode(pkNode);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE6,
                            pkNode == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pkNode,
                            TEST_DEVTREE_ENABLED);

  pkNode = FDTGetNodeByHandle(1);

  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETHANDLE2,
                          strcmp(pkNode->pName, "acpi@E0000") == 0,
                          0,
                          strcmp(pkNode->pName, "acpi@E0000"),
                          TEST_DEVTREE_ENABLED);

  pMemNode = FDTGetMemory();

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETMEMORY0,
                          pMemNode != NULL,
                          (uintptr_t)0xDEADC0DE,
                          (uintptr_t)NULL,
                          TEST_DEVTREE_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETMEMORY1,
                          pMemNode->pNextNode == NULL,
                          (uintptr_t)NULL,
                          (uintptr_t)pMemNode->pNextNode,
                          TEST_DEVTREE_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETMEMORY2,
                          pMemNode->baseAddress == 0x0,
                          (uintptr_t)0x0,
                          (uintptr_t)pMemNode->baseAddress,
                          TEST_DEVTREE_ENABLED);
#ifdef ARCH_32_BITS
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETMEMORY3,
                          FDTTOCPU32(pMemNode->size) == 0x10000000,
                          (uintptr_t)0x10000000,
                          (uintptr_t)FDTTOCPU32(pMemNode->size),
                          TEST_DEVTREE_ENABLED);
#else
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETMEMORY3,
                          FDTTOCPU64(pMemNode->size) == 0x10000000,
                          (uintptr_t)0x10000000,
                          (uintptr_t)FDTTOCPU64(pMemNode->size),
                          TEST_DEVTREE_ENABLED);
#endif
  pMemNode = FDTGetReservedMemory();

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETRESMEMORY0,
                          pMemNode != NULL,
                          (uintptr_t)0xDEADC0DE,
                          (uintptr_t)NULL,
                          TEST_DEVTREE_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETRESMEMORY1,
                          pMemNode->pNextNode != NULL,
                          (uintptr_t)0xDEADC0DE,
                          (uintptr_t)pMemNode->pNextNode,
                          TEST_DEVTREE_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETRESMEMORY2,
                          pMemNode->baseAddress == 0x0,
                          (uintptr_t)0x0,
                          (uintptr_t)pMemNode->baseAddress,
                          TEST_DEVTREE_ENABLED);

#ifdef ARCH_32_BITS
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETRESMEMORY3,
                          FDTTOCPU32(pMemNode->size) == 0x100000,
                          (uintptr_t)0x100000,
                          (uintptr_t)FDTTOCPU32(pMemNode->size),
                          TEST_DEVTREE_ENABLED);
#else
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETRESMEMORY3,
                          FDTTOCPU64(pMemNode->size) == 0x100000,
                          (uintptr_t)0x100000,
                          (uintptr_t)FDTTOCPU64(pMemNode->size),
                          TEST_DEVTREE_ENABLED);
#endif

  pkNode = FDTGetNodeByName("timeconfig");

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNODEBYNAME,
                          pkNode != NULL,
                          (uintptr_t)0xDEADC0DE,
                          (uintptr_t)pkNode,
                          TEST_DEVTREE_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/