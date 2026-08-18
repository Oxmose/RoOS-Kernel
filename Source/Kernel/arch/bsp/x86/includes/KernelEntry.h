/*******************************************************************************
 * @file KernelEntry.c
 *
 * @see KernelEntry.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief Kernel's main boot sequence.
 *
 * @warning At this point interrupts should be disabled.
 *
 * @details Kernel's booting sequence. Initializes the rest of the kernel and
 * performs GDT, IDT and TSS initialization. Initializes the hardware and
 * software core of the kernel before calling the scheduler.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X86_KERNEL_ENTRY_H_
#define __X86_KERNEL_ENTRY_H_

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
 * @brief X86 64 CPU kernel entry point.
 *
 * @details X86 64 CPU kernel entry point. Initializes the hardware and software
 * before starting the scheduler.
 */
void X64KernelEntry(void);

#endif /* #ifndef __X86_KERNEL_ENTRY_H_ */

/************************************ EOF *************************************/
