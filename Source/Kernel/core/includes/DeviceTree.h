/*******************************************************************************
 * @file DeviceTree.h
 *
 * @see DeviceTree.c
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

#ifndef __CORE_DEVTREE_H_
#define __CORE_DEVTREE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stddef.h>
#include <stdint.h>

/* Configuration files */
#include <config.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief FDT property descriptor */
typedef struct S_FDTProperty
{
  /** @brief Property name */
  char* pName;
  /** @brief Property length */
  size_t length;
  /** @brief Property address in memory */
  uint32_t* pCells;
  /** @brief Next property of the node */
  struct S_FDTProperty* pNextProp;
} S_FDTProperty;

/** @brief FDT node descriptor */
typedef struct S_FDTNode
{
  /** @brief Node name */
  char* pName;
  /** @brief Node address cells */
  uint8_t addrCells;
  /** @brief Node size cells */
  uint8_t sizeCells;
  /** @brief Node properties list */
  S_FDTProperty* pProps;
  /** @brief Parent node */
  struct S_FDTNode* pParentNode;

  /** @brief Next sibling node */
  struct S_FDTNode* pNextNode;
  /** @brief First child node */
  struct S_FDTNode* pFirstChildNode;

  /** @brief Device data associated to the node */
  void* pDevData;
} S_FDTNode;

/** @brief FDT memory node descriptor */
typedef struct S_FDTMemoryNode
{
  /** @brief Memory region base address */
  uintptr_t baseAddress;
  /** @brief Memory region size */
  size_t size;
  struct S_FDTMemoryNode* pNextNode;
} S_FDTMemoryNode;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#ifndef ARCH_LITTLE_ENDIAN
#define FDTTOCPU32(X) (X)
#define FDTTOCPU64(X) (X)
#else
/**
 * @brief Swaps FDT 32 bits big endian to little endian values.
 * @param[in] X The value to swap.
 */
#define FDTTOCPU32(X) (__builtin_bswap32(X))
/**
 * @brief Swaps FDT 64 bits big endian to little endian values.
 * @param[in] X The value to swap.
 */
#define FDTTOCPU64(X) (__builtin_bswap64(X))
#endif

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
 * @brief Initializes the FDT driver.
 *
 * @details Initializes the FDT driver. This function parses the FDT in memory
 * and creates a copy with a tree structure in memory.
 *
 * @param[in] kStartAddr The virtual address of the FDT in memory.
*/
void DeviceTreeInit(const uintptr_t kStartAddr);

/**
 * @brief Gets the property for a given node.
 *
 * @brief Gets the property for a given node base on the property name. The size
 * of the property is also stored in the pReadSize buffer. If the property is
 * not found, NULL is returned anf the size of the property is set to 0.
 *
 * @param[in] kpFdtNode The node where to extract the property.
 * @param[in] kpName The name of the property to extract.
 * @param[out] pReadSize The buffer to store the size of the extracted property.
 *
 * @return A constand pointer to the property value is returned.
*/
const void* FDTGetProp(const S_FDTNode* kpFdtNode,
                       const char*      kpName,
                       size_t*          pReadSize);

/**
 * @brief Returns the root node of the FDT.
 *
 * @return The root node of the FDT is returned.
 */
const S_FDTNode* FDTGetRoot(void);

/**
 * @brief Returns the next simbling node of a given node.
 *
 * @details Returns the next simbling node of a given node. NULL is returned if
 * the node has no simbling node.
 *
 * @param[in] kpNode The node of which the next node should be returned.
 *
 * @return The next simbling node of the node given as parameter is returned.
 * NULL is returned if the node has no simbling node.
*/
const S_FDTNode* FDTGetNextNode(const S_FDTNode* kpNode);

/**
 * @brief Returns the first child node of a given node.
 *
 * @details Returns the first child node of a given node. NULL is returned if
 * the node has no child node.
 *
 * @param[in] kpNode The node of which the child node should be returned.
 *
 * @return The first child node of the node given as parameter is returned.
 * NULL is returned if the node has no child node.
*/
const S_FDTNode* FDTGetChild(const S_FDTNode* kpNode);

/**
 * @brief Returns the first property of a given node.
 *
 * @details Returns the first property of a given node. NULL is returned if
 * the node has no property.
 *
 * @param[in] kpNode The node of which the first property should be returned.
 *
 * @return The first property of the node given as parameter is returned.
 * NULL is returned if the node has no property.
*/
const S_FDTProperty* FDTGetFirstProp(const S_FDTNode* kpNode);

/**
 * @brief Returns the next property of a given property.
 *
 * @details Returns the next property of a given property. NULL is returned if
 * the property has no next property.
 *
 * @param[in] kpProp The property of which the next property should be returned.
 *
 * @return The first property of the property given as parameter is returned.
 * NULL is returned if the property has no next property.
*/
const S_FDTProperty* FDTGetNextProp(const S_FDTProperty* kpProp);

/**
 * @brief Returns a node based on its phandle value.
 *
 * @details Returns a node based on its phandle value. NULL is returned if
 * the node cannot be found.
 *
 * @param[in] kHandleId The phandle value to lookup.
 *
 * @return The node that corresponds to the phandle value is returned. NULL is
 * returned is the node cannot be found.
*/
const S_FDTNode* FDTGetNodeByHandle(const uint32_t kHandleId);

/**
 * @brief Returns the list of available memory regions.
 *
 * @brief Returns the list of available memory regions present in the system.
 *
 * @return The list of available memory regions is returned.
 */
const S_FDTMemoryNode* FDTGetMemory(void);

/**
 * @brief Returns the list of reserved memory regions.
 *
 * @brief Returns the list of reserved memory regions that should not be used
 * for general purpose memory usage.
 *
 * @return The list of reserved memory regions is returned.
 */
const S_FDTMemoryNode* FDTGetReservedMemory(void);

/**
 * @brief Returns a node based on its name.
 *
 * @details Returns a node based on its name. NULL is returned if
 * the node cannot be found.
 *
 * @param[in] kpName The name to lookup.
 *
 * @return The node that corresponds to the name is returned. NULL is
 * returned is the node cannot be found.
*/
const S_FDTNode* FDTGetNodeByName(const char* kpName);

#endif /* #ifndef __CORE_DEVTREE_H_ */

/************************************ EOF *************************************/