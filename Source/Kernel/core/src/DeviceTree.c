/*******************************************************************************
 * @file DeviceTree.c
 *
 * @see DeviceTree.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/05/2024
 *
 * @version 1.0
 *
 * @brief Device file tree driver.
 *
 * @details  This file provides the device file tree driver used by roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <Panic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <KernelHeap.h>
#include <KernelError.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <DeviceTree.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "DEVTREE"

/** @brief FDT Magic number */
#define FDT_MAGIC_NUMBER 0xD00DFEED
/** @brief FDT Begin node identifier */
#define FDT_BEGIN_NODE 0x00000001
/** @brief FDT End node identifier */
#define FDT_END_NODE 0x00000002
/** @brief FDT Property identifier */
#define FDT_PROP 0x00000003

/** @brief Maximum length for a FDT property name */
#define FDT_PROPERTY_NAME_MAX_LENGTH 64
/** @brief Maximum length for a FDT node name */
#define FDT_NODE_NAME_MAX_LENGTH 64

/** @brief Initial address cells value */
#define INIT_ADDR_CELLS 2
/** @brief Initial size cells value */
#define INIT_SIZE_CELLS 1

/** @brief FDT cells size  */
#define FDT_CELL_SIZE sizeof(uint32_t)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief File Device Tree Header structure. */
typedef struct
{
  /** @brief FDT Magic number */
  uint32_t magic;
  /** @brief FDT total size  */
  uint32_t size;
  /** @brief FDT structures offset */
  uint32_t offStructs;
  /** @brief FDT strings offset */
  uint32_t offStrings;
  /** @brief FDT reserved memory map */
  uint32_t offMemRsvMap;
  /** @brief FDT version */
  uint32_t version;
  /** @brief FDT last compatible version */
  uint32_t lastCompatVersion;
  /** @brief FDT boot CPU identifier */
  uint32_t bootCPUId;
  /** @brief FDT string table size */
  uint32_t sizeStrings;
  /** @brief FDT structures table size */
  uint32_t sizeStructs;
} S_FDTHeader;

/** @brief pHandle struct */
typedef struct S_PHandle
{
  /** @brief pHandle identifier */
  uint32_t id;
  /** @brief Link to the node */
  void* pLink;

  /** @brief Next phandle in the list */
  struct S_PHandle* pNext;
} S_PHandle;

/** @brief Internal descriptor for the FDT */
typedef struct
{
  /** @brief Number of structures */
  uint32_t nbStructs;
  /** @brief Struct table pointer */
  uint32_t* pStructs;
  /** @brief String table pointer */
  const char* pStrings;
  /** @brief Reserved memory pointer  */
  uintptr_t* pResMemory;

  /** @brief First root node of the FDT */
  S_FDTNode* pFirstNode;

  /** @brief FDT pHandle list */
  S_PHandle* pHandleList;

  /** @brief FDT available memory regions list */
  S_FDTMemoryNode* pFirstMemoryNode;

  /** @brief FDT reserved memory regions list */
  S_FDTMemoryNode* pFirstReservedMemoryNode;
} S_FDTDescriptor;

/** @brief Specific property actions */
typedef struct
{
  /** @brief Action name (property name) */
  const char*  pName;
  /** @brief Property name size */
  const size_t nameSize;
  /** @brief Action function pointer */
  void (*pAction)(S_FDTNode*, S_FDTProperty*);
} S_SpecificPropertyAction;

/** @brief Specific property table */
typedef struct
{
  /** @brief Number of properties with specific action. */
  uint32_t numberOfProps;
  /** @brief Table of actions */
  S_SpecificPropertyAction pSpecProps[3];
} S_SpecificPropertyActionTable;

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/** @brief Align memory with FDT boundaries. */
#define ALIGN(VAL, ALIGN_V) \
  ((((VAL) + ((ALIGN_V) - 1)) / (ALIGN_V)) * (ALIGN_V))


/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define DEVTREE_ASSERT(COND, MSG, ERROR) {              \
  if ((COND) == false)                                   \
  {                                                     \
    PANIC(ERROR, MODULE_NAME, MSG, false);              \
  }                                                     \
}
/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Links a node to another in the FDT tree.
 *
 * @details Links a node to another in the FDT tree. This is used to reverse the
 * FDT read order (First Read Last Seen).
 *
 * @param[in, out] pNode The node to link.
 * @param[in, out] pLinkNode The next node to link.
*/
static inline void _LinkNode(S_FDTNode* pNode, S_FDTNode* pLinkNode);

/**
 * @brief Links a property to another in the FDT tree.
 *
 * @details Links a property to another in the FDT tree. This is used to reverse
 * the FDT read order (First Read Last Seen).
 *
 * @param[in, out] pProp The property to link.
 * @param[in, out] pLinkProp The next property to link.
*/
static inline void _LinkProperty(S_FDTProperty* pProp,
                                 S_FDTProperty* pLinkProp);

/**
 * @brief Parses a node in the FDT.
 *
 * @details Parses a node in the FDT based on the current offset. The function
 * populates the node and returns it.
 *
 * @param[in, out] pOffset The current offset in the FDT. The offset is updated
 * by this function.
 * @param[in] kAddrCells Current address-cells value.
 * @param[in] kSizeCells Current size-cells value.
 * @param[in] kIsResMemSubNode Tells if the current node is a reserved-memory
 * sub-node.
 *
 * @return A new node parsed from the FDT is returned.
*/
static S_FDTNode* _ParseNode(uint32_t*     pOffset,
                             const uint8_t kAddrCells,
                             const uint8_t kSizeCells,
                             const bool    kIsResMemSubNode);

/**
 * @brief Parses a property in the FDT.
 *
 * @details Parses a property in the FDT based on the current offset. The
 * function populates the property and returns it.
 *
 * @param[in, out] pOffset The current offset in the FDT. The offset is updated
 * by this function.
 * @param[in] pNode The node from which we should parse the property from.
 *
 * @return A new property parsed from the FDT is returned.
*/
static S_FDTProperty* _ParseProperty(uint32_t* pOffset, S_FDTNode* pNode);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void _ApplyPropertyAction(S_FDTNode* pNode, S_FDTProperty* pProperty);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void _ApplyActionAddressCells(S_FDTNode*     pNode,
                                     S_FDTProperty* pProperty);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void _ApplyActionSizeCells(S_FDTNode* pNode, S_FDTProperty* pProperty);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void _ApplyActionPhandle(S_FDTNode*     pNode,
                                S_FDTProperty* pProperty);

/**
 * @brief Reads a property from the internal FDT.
 *
 * @details Reads a property from the internal FDT. The function reads the
 * property and gets its size. The size is updated in pReadSize.
 *
 * @param[in] kpProperty The property to read.
 * @param[out] pReadSize The buffer that receives the property size.
 *
 * @return The pointer to the property cells is returned.
*/
static inline const void* _FDTInternalReadProp(const S_FDTProperty* kpProperty,
                                               size_t*              pReadSize);

/**
 * @brief Parses the reserved memory section of the FDT.
 *
 * @details Parses the reserved memory section of the FDT. A list of reserved
 * memory region will be created in teh FDT descriptor.
 */
static void _ParseReservedMemory(void);

/**
 * @brief Walks the FDT nodes starting from the root node and returns the
 * node with the requested name.
 *
 * @details Walks the FDT nodes starting from the root node and returns the
 * node with the requested name. The walk is
 * performed as a depth first search walk. For each node, the compatible is
 * compared to the list of registered drivers. If a driver is compatible, its
 * attach function is called. NULL is returned is the node is not found.
 *
 * @param[in] kpRootNode The root node to use.
 * @param[in] kpName The name of the node to find.
 *
 * @return The node with the requested name is returned or NULL if not found.
 */
static const S_FDTNode* _FindFDTNode(const S_FDTNode* kpRootNode,
                                     const char*      kpName);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief The current contructed FDT desriptor. */
static S_FDTDescriptor sFDTDesc;

/** @brief Talbe that contains the specific properties actions. */
static S_SpecificPropertyActionTable sSpecPropTable =
{
  .numberOfProps = 3,
  .pSpecProps =
  {
    {"phandle",         7, _ApplyActionPhandle},
    {"#address-cells", 14, _ApplyActionAddressCells},
    {"#size-cells",    11, _ApplyActionSizeCells},
  }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static inline void _LinkNode(S_FDTNode* pNode, S_FDTNode* pLinkNode)
{

  while (pNode->pNextNode != NULL)
  {
    pNode = pNode->pNextNode;
  }
  pLinkNode->pNextNode = NULL;
  pNode->pNextNode     = pLinkNode;
}

static inline void _LinkProperty(S_FDTProperty* pProp,
                                 S_FDTProperty* pLinkProp)
{

  while (pProp->pNextProp != NULL)
  {
    pProp = pProp->pNextProp;
  }
  pLinkProp->pNextProp = NULL;
  pProp->pNextProp     = pLinkProp;
}

static inline const void* _FDTInternalReadProp(const S_FDTProperty* kpProperty,
                                               size_t*              pReadSize)
{
  void* pCells;

  if (kpProperty == NULL)
  {
    *pReadSize = 0;
    pCells = NULL;
  }
  else
  {
    *pReadSize = kpProperty->length;
    pCells = kpProperty->pCells;
  }

  return pCells;
}

static void _ApplyActionPhandle(S_FDTNode*     pNode,
                                S_FDTProperty* pProperty)
{
  S_PHandle* pNewHandle;

  pNewHandle = KMalloc(sizeof(S_PHandle), ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);

  pNewHandle->id    = FDTTOCPU32(*pProperty->pCells);
  pNewHandle->pLink = (void*)pNode;
  pNewHandle->pNext = sFDTDesc.pHandleList;

  sFDTDesc.pHandleList = pNewHandle;
}

static void _ApplyActionAddressCells(S_FDTNode*     pNode,
                                     S_FDTProperty* pProperty)
{
  size_t          propertySize;
  const uint32_t* kpPropertyPtr;

  propertySize  = sizeof(uint32_t);
  kpPropertyPtr = _FDTInternalReadProp(pProperty, &propertySize);
  if (propertySize == sizeof(uint32_t))
  {
    pNode->addrCells = FDTTOCPU32(*kpPropertyPtr);
  }
}

static void _ApplyActionSizeCells(S_FDTNode*     pNode,
                                  S_FDTProperty* pProperty)
{
  size_t          propertySize;
  const uint32_t* kpPropertyPtr;

  propertySize  = sizeof(uint32_t);
  kpPropertyPtr = _FDTInternalReadProp(pProperty, &propertySize);
  if (propertySize == sizeof(uint32_t))
  {
    pNode->sizeCells = FDTTOCPU32(*kpPropertyPtr);
  }
}

static void _ApplyPropertyAction(S_FDTNode* pNode, S_FDTProperty* pProperty)
{
  size_t i;
  size_t propNameLength;

  propNameLength = strnlen(pProperty->pName, FDT_PROPERTY_NAME_MAX_LENGTH - 1);

  /* Check all properties */
  for (i = 0; i < sSpecPropTable.numberOfProps; ++i)
  {
    if (propNameLength == sSpecPropTable.pSpecProps[i].nameSize &&
        sSpecPropTable.pSpecProps[i].pAction != NULL &&
        strcmp(pProperty->pName, sSpecPropTable.pSpecProps[i].pName) == 0)
    {
      sSpecPropTable.pSpecProps[i].pAction(pNode, pProperty);
    }
  }
}

static S_FDTProperty* _ParseProperty(uint32_t* pOffset, S_FDTNode* pNode)
{
  S_FDTProperty*  pProperty;
  const char*     kpName;
  size_t          length;
  const uint32_t* kpInitProperty;

  /* Check if start property */
  if (FDTTOCPU32(sFDTDesc.pStructs[*pOffset]) == FDT_PROP)
  {
    ++(*pOffset);

    /* Allocate new property */
    pProperty = KMalloc(sizeof(S_FDTProperty),
                        ALIGN_ADDRESS,
                        KMALLOC_NO_FREE_POOL);
    memset(pProperty, 0, sizeof(S_FDTProperty));

    kpInitProperty = (uint32_t*)&sFDTDesc.pStructs[*pOffset];
    *pOffset += 2;

    /* Get the length and name */
    pProperty->length = FDTTOCPU32(kpInitProperty[0]);
    kpName = sFDTDesc.pStrings + FDTTOCPU32(kpInitProperty[1]);
    length = strnlen(kpName, FDT_PROPERTY_NAME_MAX_LENGTH - 1);

    pProperty->pName = KMalloc(length + 1, ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);
    memcpy(pProperty->pName, kpName, length);
    pProperty->pName[length] = 0;

    /* Copy the property cells */
    if (pProperty->length != 0)
    {
      pProperty->pCells = KMalloc(pProperty->length,
                                  ALIGN_ADDRESS,
                                  KMALLOC_NO_FREE_POOL);
      memcpy(pProperty->pCells,
             sFDTDesc.pStructs + *pOffset,
             pProperty->length);
    }
    else
    {
      pProperty->pCells = NULL;
    }

    *pOffset += ALIGN(pProperty->length, FDT_CELL_SIZE) / FDT_CELL_SIZE;

    _ApplyPropertyAction(pNode, pProperty);
  }
  else
  {
    pProperty = NULL;
  }

  return pProperty;
}

static S_FDTNode* _ParseNode(uint32_t*     pOffset,
                             const uint8_t kAddrCells,
                             const uint8_t kSizeCells,
                             const bool    kIsResMemSubNode)
{
  S_FDTNode*       pNode;
  S_FDTNode*       pChildNode;
  S_FDTNode*       retNode;
  S_FDTMemoryNode* pMemNode;
  S_FDTMemoryNode* pMemNodeCursor;
  S_FDTProperty*   pProperty;
  const char*      kpInitName;
  size_t           length;
  uint32_t         cursor;
  size_t           i;
  bool             isResMem;

  /* Check if start node */
  retNode = NULL;
  if (FDTTOCPU32(sFDTDesc.pStructs[*pOffset]) == FDT_BEGIN_NODE)
  {
    ++(*pOffset);

    /* Allocate new node */
    pNode   = KMalloc(sizeof(S_FDTNode), ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);
    retNode = pNode;
    memset(pNode, 0, sizeof(S_FDTNode));

    pNode->addrCells = kAddrCells;
    pNode->sizeCells = kSizeCells;

    /* Get name and copy */
    kpInitName = (char*)&sFDTDesc.pStructs[*pOffset];
    length    = strnlen(kpInitName, FDT_NODE_NAME_MAX_LENGTH - 1);

    pNode->pName = KMalloc(length + 1, ALIGN_ADDRESS, KMALLOC_NO_FREE_POOL);
    memcpy(pNode->pName, kpInitName, length);
    pNode->pName[length] = 0;

    isResMem = (strcmp(pNode->pName, "reserved-memory") == 0);

    /* Update offset */
    *pOffset += ALIGN(length + 1, FDT_CELL_SIZE) / FDT_CELL_SIZE;

    /* Walk the node until we reache the number of structs */
    while (sFDTDesc.nbStructs > *pOffset)
    {
      cursor = FDTTOCPU32(sFDTDesc.pStructs[*pOffset]);
      if (cursor == FDT_BEGIN_NODE)
      {
        /* New Child */
        pChildNode = _ParseNode(pOffset,
                                pNode->addrCells,
                                pNode->sizeCells,
                                isResMem);
        if (pChildNode != NULL)
        {
          if (pNode->pFirstChildNode == NULL)
          {
            pChildNode->pNextNode = NULL;
            pNode->pFirstChildNode = pChildNode;
          }
          else
          {
            _LinkNode(pNode->pFirstChildNode, pChildNode);
          }
        }
      }
      else if (cursor == FDT_PROP)
      {
        pProperty = _ParseProperty(pOffset, pNode);
        if (pProperty != NULL)
        {
          if (pNode->pProps == NULL)
          {
            pProperty->pNextProp = NULL;
            pNode->pProps = pProperty;
          }
          else
          {
            _LinkProperty(pNode->pProps, pProperty);
          }

          /* If node is memory */
          if (strncmp(pNode->pName, "memory@", 7) == 0 &&
              strcmp(pProperty->pName, "reg") == 0)
          {
            for (i = 0; i < pProperty->length / sizeof(uintptr_t); i += 2)
            {
              pMemNode = KMalloc(sizeof(S_FDTMemoryNode),
                                  ALIGN_ADDRESS,
                                  KMALLOC_NO_FREE_POOL);
              pMemNode->baseAddress = *(((uintptr_t*)pProperty->pCells) + i);
              pMemNode->size = *(((uintptr_t*)pProperty->pCells) + 1 + i);

              pMemNode->pNextNode = NULL;

              if (sFDTDesc.pFirstMemoryNode != NULL)
              {
                pMemNodeCursor = sFDTDesc.pFirstMemoryNode;
                while (pMemNodeCursor->pNextNode != NULL)
                {
                  pMemNodeCursor = pMemNodeCursor->pNextNode;
                }
                pMemNodeCursor->pNextNode = pMemNode;
              }
              else
              {
                sFDTDesc.pFirstMemoryNode = pMemNode;
              }
            }
          }
          /* If node is subnode of reserved-memory */
          else if (kIsResMemSubNode)
          {
            if (strcmp(pProperty->pName, "reg") == 0)
            {
              pMemNode = KMalloc(sizeof(S_FDTMemoryNode),
                                 ALIGN_ADDRESS,
                                 KMALLOC_NO_FREE_POOL);
              pMemNode->baseAddress = *((uintptr_t*)pProperty->pCells);
              pMemNode->size = *(((uintptr_t*)pProperty->pCells) + 1);

              pMemNode->pNextNode = NULL;

              if (sFDTDesc.pFirstReservedMemoryNode != NULL)
              {
                pMemNodeCursor = sFDTDesc.pFirstReservedMemoryNode;
                while (pMemNodeCursor->pNextNode != NULL)
                {
                  pMemNodeCursor = pMemNodeCursor->pNextNode;
                }
                pMemNodeCursor->pNextNode = pMemNode;
              }
              else
              {
                sFDTDesc.pFirstReservedMemoryNode = pMemNode;
              }
            }
          }
        }
      }
      else
      {
        ++(*pOffset);
        if (cursor == FDT_END_NODE)
        {
          retNode = pNode;
          break;
        }
      }
    }
  }

  return retNode;
}

static void _ParseReservedMemory(void)
{
    uint64_t*        pCursor;
    uintptr_t        startAddr;
    uintptr_t        size;
    S_FDTMemoryNode* pNode;
    S_FDTMemoryNode* pNodeCursor;

    /* Register the first node */
    pCursor   = (uint64_t*)sFDTDesc.pResMemory;
    startAddr = (uintptr_t)FDTTOCPU64(pCursor[0]);
    size      = (uintptr_t)FDTTOCPU64(pCursor[1]);

    while (startAddr != 0 && size != 0)
    {
      pNode = KMalloc(sizeof(S_FDTMemoryNode),
                      ALIGN_ADDRESS,
                      KMALLOC_NO_FREE_POOL);

      pNode->baseAddress = startAddr;
      pNode->size        = size;

      pNode->pNextNode = NULL;

      if (sFDTDesc.pFirstReservedMemoryNode != NULL)
      {
        pNodeCursor = sFDTDesc.pFirstMemoryNode;
        while (pNodeCursor->pNextNode != NULL)
        {
          pNodeCursor = pNodeCursor->pNextNode;
        }
        pNodeCursor->pNextNode = pNode;
      }
      else
      {
        sFDTDesc.pFirstMemoryNode = pNode;
      }

      pCursor   += 2;
      startAddr = (uintptr_t)FDTTOCPU64(pCursor[0]);
      size      = (uintptr_t)FDTTOCPU64(pCursor[1]);
  }
}

static const S_FDTNode* _FindFDTNode(const S_FDTNode* kpRootNode,
                                     const char*      kpName)
{
  const S_FDTNode* kpRetNode;

  if (kpRootNode != NULL)
  {
    /* Check if the name is not found */
    if (strcmp(kpRootNode->pName, kpName) != 0)
    {
      /* Got to next nodes */
      kpRetNode = _FindFDTNode(FDTGetChild(kpRootNode), kpName);
      if (kpRetNode == NULL)
      {
        kpRetNode = _FindFDTNode(FDTGetNextNode(kpRootNode), kpName);
      }
    }
    else
    {
      kpRetNode = kpRootNode;
    }
  }
  else
  {
    kpRetNode = NULL;
  }

  return kpRetNode;
}

void DeviceTreeInit(const uintptr_t kStartAddr)
{
  const S_FDTHeader* kpHeader;
  S_FDTNode*         pNode;
  uint32_t           i;

  kpHeader = (S_FDTHeader*)kStartAddr;

  /* Check magic */
  DEVTREE_ASSERT(FDTTOCPU32(kpHeader->magic) == FDT_MAGIC_NUMBER,
                 "Invalid device tree magic number.",
                 ERR_INVALID_PARAMETER);

  memset(&sFDTDesc, 0, sizeof(S_FDTDescriptor));

  /* Get the structs and strings addresses */
  sFDTDesc.pStructs   = (uint32_t*)(kStartAddr +
                                    FDTTOCPU32(kpHeader->offStructs));
  sFDTDesc.pStrings   = (char*)(kStartAddr + FDTTOCPU32(kpHeader->offStrings));
  sFDTDesc.pResMemory = (uintptr_t*)(kStartAddr +
                                  FDTTOCPU32(kpHeader->offMemRsvMap));

  sFDTDesc.nbStructs = FDTTOCPU32(kpHeader->sizeStructs) / sizeof(uint32_t);

  /* Get the reseved memory regions */
  _ParseReservedMemory();

  /* Now start parsing the first level */
  for (i = 0; i < sFDTDesc.nbStructs; ++i)
  {
    /* Get the node and add to root */
    pNode = _ParseNode(&i, INIT_ADDR_CELLS, INIT_SIZE_CELLS, false);
    if (pNode != NULL)
    {
      /* Link node */
      if (sFDTDesc.pFirstNode == NULL)
      {
        pNode->pNextNode = NULL;
        sFDTDesc.pFirstNode = pNode;
      }
      else
      {
        _LinkNode(sFDTDesc.pFirstNode, pNode);
      }
    }
  }
}

const void* FDTGetProp(const S_FDTNode* kpFDTNode,
                       const char*      kpName,
                       size_t*          pReadSize)
{
  const S_FDTProperty* kpProp;
  void*                retVal;

  if (kpFDTNode != NULL && kpName != NULL)
  {
    kpProp = kpFDTNode->pProps;
    retVal = NULL;
    while (kpProp != NULL)
    {
      /* Check name */
      if (strcmp(kpProp->pName, kpName) == 0)
      {
        /* Setup return, on NULL to tell the property is here, return 1 */
        if (kpProp->pCells == NULL)
        {
          retVal = (void*)1;
          *pReadSize = 0;
        }
        else
        {
          retVal = (void*)kpProp->pCells;
          *pReadSize = kpProp->length;
        }

        break;
      }

      kpProp = kpProp->pNextProp;
    }
  }
  else
  {
    *pReadSize = 0;
    retVal = NULL;
  }

  return retVal;
}

const S_FDTNode* FDTGetRoot(void)
{
  return sFDTDesc.pFirstNode;
}

const S_FDTNode* FDTGetNextNode(const S_FDTNode* kpNode)
{
  if (kpNode != NULL)
  {
    return kpNode->pNextNode;
  }

  return NULL;
}

const S_FDTNode* FDTGetChild(const S_FDTNode* kpNode)
{
  if (kpNode != NULL)
  {
    return kpNode->pFirstChildNode;
  }

  return NULL;
}

const S_FDTProperty* FDTGetFirstProp(const S_FDTNode* kpNode)
{
  if (kpNode != NULL)
  {
    return kpNode->pProps;
  }

  return NULL;
}
const S_FDTProperty* FDTGetNextProp(const S_FDTProperty* kpProp)
{
  if (kpProp != NULL)
  {
    return kpProp->pNextProp;
  }

  return NULL;
}

const S_FDTNode* FDTGetNodeByHandle(const uint32_t kHandleId)
{
  const S_PHandle* kpHandle;
  const S_FDTNode* kpReturnNode;

  kpHandle     = sFDTDesc.pHandleList;
  kpReturnNode = NULL;
  while (kpHandle != NULL)
  {
    if (kpHandle->id == kHandleId)
    {
      kpReturnNode = (S_FDTNode*)kpHandle->pLink;
      break;
    }
    kpHandle = kpHandle->pNext;
  }

  return kpReturnNode;
}

const S_FDTMemoryNode* FDTGetMemory(void)
{
  return sFDTDesc.pFirstMemoryNode;
}

const S_FDTMemoryNode* FDTGetReservedMemory(void)
{
  return sFDTDesc.pFirstReservedMemoryNode;
}

const S_FDTNode* FDTGetNodeByName(const char* kpName)
{
  return _FindFDTNode(sFDTDesc.pFirstNode, kpName);
}

/************************************ EOF *************************************/