/*******************************************************************************
 * @file Panic.h
 *
 * @see Panic.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/04/2023
 *
 * @version 1.0
 *
 * @brief Panic feature of the kernel.
 *
 * @details Kernel panic functions. Displays the CPU registers, the faulty
 * instruction, the interrupt ID and cause for a kernel panic. For a process
 * panic the panic will kill the process.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_PANIC_H_
#define __CPU_PANIC_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
/**
 * @brief Raises a kernel panic with error code and collect other data.
 *
 * @param[in] ERROR The error code forthe panic.
 * @param[in] MODULE The module that generated the panic. Can be empty when not
 * relevant.
 * @param[in] MSG The panic message used for kernel panic.
 */
#define PANIC(ERROR, MODULE, MSG, INT_CTX) {                    \
  KernelPanic(ERROR, MODULE, MSG, __FILE__, __LINE__, INT_CTX); \
}

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
 * @brief Causes a kernel panic.
 *
 * @details Causes a kernel panic. This will raise an interrupt to generate the
 * panic.
 *
 * @param[in] kErrorCode The error code to display on the kernel panic's screen.
 * @param[in] kpModule The module that generated the panic. Can be empty when
 * not relevant.
 * @param[in] kpMsg The message to display in the kernel's panig screen.
 * @param[in] kpFile The name of the source file where the panic was called.
 * @param[in] kLine The line at which the panic was called.
 * @param[in] kFromInterrupt Tells if the panic is called from an interrupt
 * context.
 */
void KernelPanic(const uint32_t kErrorCode,
                 const char*    kpModule,
                 const char*    kpMsg,
                 const char*    kpFile,
                 const size_t   kLine,
                 const bool     kFromInterrupt);

/**
 * @brief Called by secondary cores when a primary core enters panic.
 *
 * @details Called by secondary cores when a primary core enters panic. This
 * will put the core to idle and never return.
 */
void KernelPanicSecondary(void);

#endif /* #ifndef __CPU_PANIC_H_ */

/************************************ EOF *************************************/