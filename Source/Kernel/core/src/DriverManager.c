/*******************************************************************************
 * @file DriverManager.c
 *
 * @see DriverManager.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/05/2024
 *
 * @version 1.0
 *
 * @brief Kernel's driver and device manager.
 *
 * @details Kernel's driver and device manager. Used to register, initialize and
 * manage the drivers used in the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Includes */
#include <Panic.h>
#include <string.h>
#include <DeviceTree.h>
#include <KernelError.h>
#include <KernelOutput.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* No unit test: this module is tested in real-world conditions. */

/* Header file */
#include <DriverManager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "DRIVER_MGR"

/** @brief Compatible property name in FDT */
#define COMPATIBLE_PROP_NAME "compatible"

/** @brief Pre init attach property name in FDT */
#define PRE_INIT_PROP_NAME "pre-init-attach"

/** @brief Status property name in FDT */
#define STATUS_PROP_NAME "status"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Assert macro used by the driver manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the driver manager to ensure correctness of
 * execution. Due to the critical nature of the driver manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define DRVMGR_ASSERT(COND, MSG, ERROR) {          \
  if ((COND) == false)                             \
  {                                                \
    PANIC(ERROR, MODULE_NAME, MSG, false, false);  \
  }                                                \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Walks the FDT nodes starting from the current node.
 *
 * @details Walks the FDT nodes starting from the current node. The walk is
 * performed as a depth first search walk. For each node, the compatible is
 * compared to the list of registered drivers. If a driver is compatible, its
 * attach function is called.
 *
 * @param[in] kpNode The node to start the walk from.
 * @param[in] kPreInit Tells if the pre-init attach nodes should be attached.
 */
static void _WalkFdtNodes(const S_FDTNode* kpNode, const bool kPreInit);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the registered kernel drivers table */
extern uintptr_t _START_DRV_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _WalkFdtNodes(const S_FDTNode* kpNode, const bool kPreInit)
{
  const char*     kpCompatible;
  const char*     kpStatus;
  const uint32_t* kpPreInit;
  S_Driver*       pDriver;
  uintptr_t       driverTableCursor;
  size_t          propLen;
  E_Return        retCode;
  if (kpNode != NULL)
  {
    /* Manage disabled nodes */
    kpStatus = FDTGetProp(kpNode, STATUS_PROP_NAME, &propLen);
    if (kpStatus == NULL || (propLen == 5 && strcmp(kpStatus, "okay") == 0))
    {
      /* Check pre-init state */
      kpPreInit = FDTGetProp(kpNode, PRE_INIT_PROP_NAME, &propLen);
      if ((kpPreInit != NULL && kPreInit == true) ||
          (kpPreInit == NULL && kPreInit == false))
      {
        /* Get the node compatible */
        kpCompatible = FDTGetProp(kpNode, COMPATIBLE_PROP_NAME, &propLen);
        if (kpCompatible != NULL && propLen > 0)
        {
          /* Get the head of the registered drivers section */
          driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
          pDriver = *(S_Driver**)driverTableCursor;

          /* Compare with the list of registered drivers */
          while (pDriver != NULL)
          {
            if (strcmp(pDriver->pCompatible, kpCompatible) == 0)
            {
              retCode =pDriver->pDriverAttach(kpNode);
              if (retCode != NO_ERROR)
              {
                KERNEL_ERROR("%s failed to attach to node %s with error %d\n",
                             pDriver->pName,
                             kpNode->pName,
                             retCode);
              }
              else
              {
                KERNEL_INFO("%s attached to node %s\n",
                            pDriver->pName,
                            kpNode->pName);
              }
            }
            driverTableCursor += sizeof(uintptr_t);
            pDriver = *(S_Driver**)driverTableCursor;
          }
        }
      }
    }

    /* Got to next nodes */
    _WalkFdtNodes(FDTGetChild(kpNode), kPreInit);
    _WalkFdtNodes(FDTGetNextNode(kpNode), kPreInit);
  }
}

void DriverManagerInit(const bool kPreInit)
{
  const S_FDTNode* kpFdtRootNode;
  S_Driver*        pDriver;
  uintptr_t        driverTableCursor;

  /* Display list of registered drivers */
  driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
  pDriver           = *(S_Driver**)driverTableCursor;

  while (pDriver != NULL)
  {
    KERNEL_DEBUG(DRIVER_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "%s -> %s - %s",
                 kPreInit ? "Pre Init:" : "Init:",
                 pDriver->pName,
                 pDriver->pDescription);
    driverTableCursor += sizeof(uintptr_t);
    pDriver           = *(S_Driver**)driverTableCursor;
  }

  /* Get the FDT root node and walk it to register drivers */
  kpFdtRootNode = FDTGetRoot();
  if (kpFdtRootNode != NULL)
  {
    /* Perform the registration */
    _WalkFdtNodes(kpFdtRootNode, kPreInit);
  }
}

E_Return DriverManagerSetDeviceData(const S_FDTNode* kpFdtNode,
                                    void*            pData)
{
  E_Return retCode;

  /* Check parameters */
  if (kpFdtNode != NULL)
  {
    /* Not clean, but this avoids a lot of function calls and
     * processing.
     */
    ((S_FDTNode*)kpFdtNode)->pDevData = pData;
    retCode = NO_ERROR;
  }
  else
  {
    retCode = ERR_INVALID_PARAMETER;
  }

  return retCode;
}

void* DriverManagerGetDeviceData(const uint32_t kHandle)
{
  void*            pDevData;
  const S_FDTNode* kpNode;

  /* Get the node and return the device data */
  pDevData = NULL;
  kpNode = FDTGetNodeByHandle(kHandle);
  if (kpNode != NULL)
  {
    pDevData = kpNode->pDevData;
  }

  return pDevData;
}

/************************************ EOF *************************************/