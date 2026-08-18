/*******************************************************************************
 * @file KernelError.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Kernel error definitions.
 *
 * @details Kernel error definitions. Contains the roOs error codes definition.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_KERROR_H_
#define __CORE_KERROR_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief System return states enumeration. */
typedef enum
{
  /** @brief No error occured. */
  NO_ERROR,
  /** @brief Invalid parameter was provided. */
  ERR_INVALID_PARAMETER,
  /** @brief Exceeded limit. */
  ERR_EXCEEDED_LIMIT,
  /** @brief Unauthorized action from kernel. */
  ERR_UNAUTHORIZED_ACTION,
  /** @brief Not enough memory. */
  ERR_NO_MEMORY,
  /** @brief Feature or action not supported. */
  ERR_NOT_SUPPORTED,
  /** @brief Invalid value detected */
  ERR_INVALID_VALUE,
  /** @brief Value not found. */
  ERR_NOT_FOUND
} E_Return;

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
 * FUNCTIONS
 ******************************************************************************/
/* None */

#endif /* #ifndef __CORE_KERROR_H_ */

/************************************ EOF *************************************/

