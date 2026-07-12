/*******************************************************************************
 * @file config.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/03/2023
 *
 * @version 1.0
 *
 * @brief X86_64 global configuration file.
 *
 * @details X86_64 global configuration file. This file contains all the
 * settings that the user can set before generating the kernel's binary.
 ******************************************************************************/

#ifndef __GLOBAL_CONFIG_H_
#define __GLOBAL_CONFIG_H_

/* Architecture definitions */
#define ARCH_64_BITS
#define ARCH_X86_64
#define ARCH_LITTLE_ENDIAN

/* Kernel stack default size
 * WARNING This value should be updated to fit other configuration files
 */
#define KERNEL_STACK_SIZE 0x1000

/* Maximal number of CPU supported by the architecture */
#define SOC_MAX_CPU_COUNT 64

/* Enable output debug through UART */
#define OUTPUT_DEBUG_ENABLE 1

/* Kernel log on UART */
#define DEBUG_LOG_UART      1
#define DEBUG_LOG_UART_RATE BAUDRATE_115200

/* Kernel log level */
#define KERNEL_LOG_LEVEL DEBUG_LOG_LEVEL

/*******************************************************************************
 * DEBUG Configuration
 *
 * Set to 0 to disable debug output for a specific module
 * Set to 1 to enable debug output for a specific module
 ******************************************************************************/
#define ACPI_DRIVER_DEBUG_ENABLED 1

#endif /* ifndef __GLOBAL_CONFIG_H_ */