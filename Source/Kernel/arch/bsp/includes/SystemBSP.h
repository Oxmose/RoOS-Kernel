/*******************************************************************************
 * @file SystemBSP.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 26/10/2026
 *
 * @version 1.0
 *
 * @brief System functionalities.
 *
 * @details System functionalities. This file declares the functions that
 * can be used for system operations. Those functions should be implemented by
 * the BSP.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __BSP_SYSTEM_BSP_H_
#define __BSP_SYSTEM_BSP_H_

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
 * FUNCTIONS
 ******************************************************************************/
/**
 * @brief Shutdown the system.
 *
 * @details Shutdown the system. This function send the shutdown command to the
 * BSP and halt the CPU. The function will not return if the shutdown is
 * successful.
 */
void SystemShutdown(void);

#endif /* #ifndef __BSP_SYSTEM_BSP_H_ */

/************************************ EOF *************************************/