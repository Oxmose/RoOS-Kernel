
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
static uint32_t _CountNodes(const S_FDTNode* pkNode);
static uint32_t _CountProperties(const S_FDTNode* pkNode);
static bool _HasProperty(const S_FDTNode* pkNode, const char* kpName);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static uint32_t _CountNodes(const S_FDTNode* pkNode)
{
  if (pkNode == NULL)
  {
    return 0;
  }

  return 1 + _CountNodes(FDTGetChild(pkNode)) +
         _CountNodes(FDTGetNextNode(pkNode));
}

static uint32_t _CountProperties(const S_FDTNode* pkNode)
{
  const S_FDTProperty* pkProp;
  uint32_t count;

  count = 0;
  pkProp = FDTGetFirstProp(pkNode);
  while (pkProp != NULL)
  {
    ++count;
    pkProp = FDTGetNextProp(pkProp);
  }

  return count;
}

static bool _HasProperty(const S_FDTNode* pkNode, const char* kpName)
{
  const S_FDTProperty* pkProp;

  if (pkNode == NULL || kpName == NULL)
  {
    return false;
  }

  pkProp = FDTGetFirstProp(pkNode);
  while (pkProp != NULL)
  {
    if (strcmp(pkProp->pName, kpName) == 0)
    {
      return true;
    }
    pkProp = FDTGetNextProp(pkProp);
  }

  return false;
}

void _WalkNodes(const S_FDTNode* pkNode, const uint8_t kLevel)
{
  uint8_t i;
  const S_FDTProperty* pProp;

  if (pkNode == NULL)
  {
    return;
  }

  for (i = 0; i < kLevel; ++i)
  {
    KPrintf("  ");
  }
  KPrintf("-> %s\n", pkNode->pName);
  pProp = FDTGetFirstProp(pkNode);
  while (pProp != NULL)
  {
    for (i = 0; i < kLevel; ++i)
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
  const S_FDTNode*       pkRoot;
  const S_FDTProperty*   pkProp;
  const void*            pProp;
  const void*            pNullProp;
  const S_FDTProperty*   pkNullProp;
  const S_FDTNode*       pkCpus;
  const S_FDTNode*       pkMissingNode;
  const S_FDTMemoryNode* pMemNode;
  const S_FDTMemoryNode* pReservedMemNode;
  size_t                 propLen;
  uint32_t               nodeCount;
  uint32_t               propCount;
  size_t                 nullLen;

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

  /* EXTENDED NULL AND STRUCTURAL TESTS */
  nullLen = 0xDEADBEEF;
  pNullProp = FDTGetProp(NULL, "compatible", &nullLen);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_NULL_INPUT0,
                            pNullProp == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pNullProp,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_NULL_INPUT1,
                         nullLen == 0,
                         0,
                         nullLen,
                         TEST_DEVTREE_ENABLED);

  pkNullProp = FDTGetFirstProp(NULL);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_NULL_INPUT2,
                            pkNullProp == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pkNullProp,
                            TEST_DEVTREE_ENABLED);

  pkNullProp = FDTGetNextProp(NULL);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_NULL_INPUT3,
                            pkNullProp == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pkNullProp,
                            TEST_DEVTREE_ENABLED);

  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_NULL_INPUT4,
                            FDTGetChild(NULL) == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)FDTGetChild(NULL),
                            TEST_DEVTREE_ENABLED);

  pkRoot = FDTGetRoot();
  nodeCount = _CountNodes(pkRoot);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_COUNT0,
                         nodeCount >= 12,
                         12,
                         nodeCount,
                         TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_COUNT1,
                         nodeCount <= 128,
                         128,
                         nodeCount,
                         TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_COUNT2,
                         nodeCount > 0,
                         1,
                         nodeCount,
                         TEST_DEVTREE_ENABLED);

  pkCpus = FDTGetNodeByName("cpus");
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_TREE_NAME0,
                            pkCpus != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pkCpus,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_NAME1,
                         strcmp(pkCpus->pName, "cpus") == 0,
                         0,
                         strcmp(pkCpus->pName, "cpus"),
                         TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_TREE_NAME2,
                            FDTGetNodeByName(pkCpus->pName) == pkCpus,
                            (uintptr_t)pkCpus,
                            (uintptr_t)FDTGetNodeByName(pkCpus->pName),
                            TEST_DEVTREE_ENABLED);

  propCount = _CountProperties(pkCpus);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_PROP0,
                         propCount >= 2,
                         2,
                         propCount,
                         TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_BYTE(TEST_DEVTREE_TREE_PROP1,
                         _HasProperty(pkCpus, "#address-cells") == true,
                         1,
                         _HasProperty(pkCpus, "#address-cells") == true,
                         TEST_DEVTREE_ENABLED);

  pkMissingNode = FDTGetNodeByHandle(0xFFFFFFFFU);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_TREE_HANDLE0,
                            pkMissingNode == NULL,
                            (uintptr_t)NULL,
                            (uintptr_t)pkMissingNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_TREE_HANDLE1,
                            FDTGetNodeByHandle(1) != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)FDTGetNodeByHandle(1),
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_HANDLE2,
                         strcmp(FDTGetNodeByHandle(1)->pName, "acpi@E0000") == 0,
                         0,
                         strcmp(FDTGetNodeByHandle(1)->pName, "acpi@E0000"),
                         TEST_DEVTREE_ENABLED);

  pMemNode = FDTGetMemory();
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_TREE_MEM0,
                            pMemNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pMemNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_MEM1,
                         pMemNode->baseAddress == 0x0,
                         0x0,
                         (uint64_t)pMemNode->baseAddress,
                         TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_MEM2,
                         pMemNode->size != 0,
                         1,
                         (uint64_t)(pMemNode->size != 0),
                         TEST_DEVTREE_ENABLED);

  pReservedMemNode = FDTGetReservedMemory();
  TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_TREE_RES0,
                            pReservedMemNode != NULL,
                            (uintptr_t)0xDEADC0DE,
                            (uintptr_t)pReservedMemNode,
                            TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_RES1,
                         pReservedMemNode->size != 0,
                         1,
                         (uint64_t)(pReservedMemNode->size != 0),
                         TEST_DEVTREE_ENABLED);
  TEST_POINT_ASSERT_UINT(TEST_DEVTREE_TREE_RES2,
                         pReservedMemNode->baseAddress <= 0xFFFFFFFFU,
                         0xFFFFFFFFU,
                         (uint64_t)pReservedMemNode->baseAddress,
                         TEST_DEVTREE_ENABLED);

  TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/