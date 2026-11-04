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

/**************************** MEMORY CONFIGURATION ****************************/
/* Kernel stack default size
 * WARNING This value should be updated to fit other configuration files
 */
#define KERNEL_STACK_SIZE 0x1000
/* Kernel process heap default size */
#define KERNEL_PROCESS_HEAP_SIZE 0x80000

/**************************** SYSTEM CONFIGURATION ****************************/
/* Maximal number of CPU supported by the architecture */
#define SOC_MAX_CPU_COUNT 64
/* Scheduler tick frequency in HZ */
#define CONFIG_SCHEDULER_TICK_FREQUENCY_HZ 200
/* UART configuration */
#define CONFIG_COMM0_BAUDRATE 115200

/**************************** DISPLAY CONFIGURATION ***************************/
/* VESA resolution configuration X*/
#define CONFIG_VESA_RESOLUTION_X 1024
/* VESA resolution configuration Y*/
#define CONFIG_VESA_RESOLUTION_Y 768
/* VESA depth configuration */
#define CONFIG_VESA_DEPTH 32
/* VESA refresh rate configuration */
#define CONFIG_VESA_REFRESH_RATE 60
/* VESA device name configuration */
#define CONFIG_VESA_DEVICE_NAME "/dev/vesa"

/* VGA text mode configuration */
#define CONFIG_VGA_TEXT_REFRESH_RATE 60
/* VGA text mode device path configuration */
#define CONFIG_VGA_TEXT_DEVICE_PATH "/dev/vga-text"

/* Use VESA mode, if set to 0, VGA text mode will be used */
#define CONFIG_USE_VESA 1

/**************************** CONSOLE CONFIGURATION ***************************/
/* Console output device configuration */
#if CONFIG_USE_VESA == 0
  #define CONFIG_CONSOLE_OUTPUT_DEV CONFIG_VGA_TEXT_DEVICE_PATH
#else
  #define CONFIG_CONSOLE_OUTPUT_DEV CONFIG_VESA_DEVICE_NAME
#endif
/* Console input device configuration */
#define CONFIG_CONSOLE_INPUT_DEV "/dev/kbd"

/*************************** LOGGING CONFIGURATION ****************************/
/* Enable output debug through UART */
#define OUTPUT_DEBUG_ENABLE 1
/* Kernel log on UART */
#define DEBUG_LOG_UART      0
#define DEBUG_LOG_UART_RATE BAUDRATE_115200
/* Kernel log level */
#define KERNEL_LOG_LEVEL DEBUG_LOG_LEVEL
/* Enable stack protection */
#define _STACK_PROT 1

/*******************************************************************************
 * DEBUG Configuration
 *
 * Set to 0 to disable debug output for a specific module
 * Set to 1 to enable debug output for a specific module
 ******************************************************************************/
#define ACPI_DRIVER_DEBUG_ENABLED 0
#define MPTABLE_DRIVER_DEBUG_ENABLED 0
#define PCIE_DRIVER_DEBUG_ENABLED 0
#define DRIVER_MGR_DEBUG_ENABLED 0
#define VESA_DERIVER_DEBUG_ENABLED 0
#define AHCI_DRIVER_DEBUG_ENABLED 0
#define DISK_MGR_DEBUG_ENABLED 0
#define E1000_DRIVER_DEBUG_ENABLED 0
#define NET_MGR_DEBUG_ENABLED 0
#define IOAPIC_DRIVER_DEBUG_ENABLED 0
#define HPET_DRIVER_DEBUG_ENABLED 0

#endif /* ifndef __GLOBAL_CONFIG_H_ */