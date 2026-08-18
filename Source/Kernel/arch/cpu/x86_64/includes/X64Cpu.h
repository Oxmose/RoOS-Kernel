/*******************************************************************************
 * @file X64Cpu.h
 *
 * @see Cpu.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief X86 64 CPU management functions and definitions.
 *
 * @details X86 64 CPU manipulation functions and definitions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
#ifndef __CPU_X86_64_X64_CPU_H_
#define __CPU_X86_64_X64_CPU_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <LAPIC.h>
#include <stdint.h>
#include <CtrlBlock.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Kernel's CPU GDT size in bytes. */
#define CPU_GDT_SIZE 0x40

/** @brief Number of entries in the kernel's IDT. */
#define IDT_ENTRY_COUNT 256

/** @brief FX data region size, increased with padding for alignement */
#define FXDATA_REGION_SIZE 528

/** @brief Defines the division by zero exception line */
#define DIVISION_BY_ZERO_EXC_LINE 0x00
/** @brief Defines debug exception line */
#define DEBUG_EXC_LINE 0x01
/** @brief Defines NMI interrupt exception line */
#define NMI_INTERRUPT_EXC_LINE 0x02
/** @brief Defines breakpoint exception line */
#define BREAKPOINT_EXC_LINE 0x03
/** @brief Defines overflow exception line */
#define OVERFLOW_EXC_LINE 0x04
/** @brief Defines bound range exceeded exception line */
#define BOUND_RANGE_EXCEEDED_EXC_LINE 0x05
/** @brief Defines invalid instruction exception line */
#define INVALID_INSTRUCTION_EXC_LINE 0x06
/** @brief Defines device not available exception line */
#define DEVICE_NOT_AVAILABLE_EXC_LINE 0x07
/** @brief Defines double fault exception line */
#define DOUBLE_FAULT_EXC_LINE 0x08
/** @brief Defines coprocessor segment overrun exception line */
#define COPROC_SEGMENT_OVERRUN_EXC_LINE 0x09
/** @brief Defines invalid TSS exception line */
#define INVALID_TSS_EXC_LINE 0x0A
/** @brief Defines segment not present exception line */
#define SEGMENT_NOT_PRESENT_EXC_LINE 0x0B
/** @brief Defines stack segment fault exception line */
#define STACK_SEGMENT_FAULT_EXC_LINE 0x0C
/** @brief Defines general protection fault exception line */
#define GENERAL_PROTECTION_FAULT_EXC_LINE 0x0D
/** @brief Defines the page fault exception line */
#define PAGE_FAULT_EXC_LINE 0x0E
/** @brief Defines x87 floating point exception line */
#define X87_FLOATING_POINT_EXC_LINE 0x10
/** @brief Defines alignement check exception line */
#define ALIGNEMENT_CHECK_EXC_LINE 0x11
/** @brief Defines machine check exception line */
#define MACHINE_CHECK_EXC_LINE 0x12
/** @brief Defines SIMD floating point exception line */
#define SIMD_FLOATING_POINT_EXC_LINE 0x13
/** @brief Defines virtualization exception line */
#define VIRTUALIZATION_EXC_LINE 0x14
/** @brief Defines control protection exception line */
#define CONTROL_PROTECTION_EXC_LINE 0x15
/** @brief Defines hypervisor injection exception line */
#define HYPERVISOR_INJECTION_EXC_LINE 0x1C
/** @brief Defines VMM communication exception line */
#define VMM_COMMUNICATION_EXC_LINE 0x1D
/** @brief Defines security exception line */
#define SECURITY_EXC_LINE 0x1E

/** @brief Defines the virtual address with supported */
#define KERNEL_VIRTUAL_ADDR_WIDTH 48

/** @brief Defines the limit address allocable by the kernel (excludes recursive
 * mapping). */
#define KERNEL_VIRTUAL_ADDR_MAX 0xFFFFFFFFFFFFEFFFULL

/**
 * @brief Kernel virtual memory offset
 * @warning This value should be updated to fit other configuration files
 */
#define KERNEL_MEM_OFFSET 0xFFFFFFFF80000000ULL

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Holds the CPU register values */
typedef struct
{
  /** @brief CPU's rsp register. */
  uint64_t rsp;
  /** @brief CPU's rbp register. */
  uint64_t rbp;
  /** @brief CPU's rdi register. */
  uint64_t rdi;
  /** @brief CPU's rsi register. */
  uint64_t rsi;
  /** @brief CPU's rdx register. */
  uint64_t rdx;
  /** @brief CPU's rcx register. */
  uint64_t rcx;
  /** @brief CPU's rbx register. */
  uint64_t rbx;
  /** @brief CPU's rax register. */
  uint64_t rax;
  /** @brief CPU's r8 register. */
  uint64_t r8;
  /** @brief CPU's r9 register. */
  uint64_t r9;
  /** @brief CPU's r10 register. */
  uint64_t r10;
  /** @brief CPU's r11 register. */
  uint64_t r11;
  /** @brief CPU's r12 register. */
  uint64_t r12;
  /** @brief CPU's r13 register. */
  uint64_t r13;
  /** @brief CPU's r14 register. */
  uint64_t r14;
  /** @brief CPU's r15 register. */
  uint64_t r15;
  /** @brief CPU's ss register. */
  uint64_t ss;
  /** @brief CPU's gs register. */
  uint64_t gs;
  /** @brief CPU's fs register. */
  uint64_t fs;
  /** @brief CPU's es register. */
  uint64_t es;
  /** @brief CPU's ds register. */
  uint64_t ds;
} __attribute__((packed)) S_CPUState;

/** @brief Holds the interrupt context */
typedef struct
{
  /** @brief Interrupt's index */
  uint64_t intId;
  /** @brief Interrupt's error code. */
  uint64_t errorCode;
  /** @brief RIP of the faulting instruction. */
  uint64_t rip;
  /** @brief CS before the interrupt. */
  uint64_t cs;
  /** @brief RFLAGS before the interrupt. */
  uint64_t rflags;
} __attribute__((packed)) S_InterruptContext;

/** @brief Virtual CPU structure */
typedef struct
{
  /** @brief Stores the current CPU interrupt context */
  S_InterruptContext intContext;
  /** @brief Stores the current CPU state */
  S_CPUState cpuState;
  /** @brief FXSAVE / FXRSTOR data region */
    uint8_t fxData[FXDATA_REGION_SIZE];
} S_VirtualCPU;

/** @brief Defines the memory layout of the FXData region */
typedef struct
{
  /** @brief FPU Control Word */
  uint16_t fcw;
  /** @brief FPU Status Word */
  uint16_t fsw;
  /** @brief FPU Tag Word */
  uint16_t ftw;
  /** @brief FPU Final Opcode */
  uint16_t fop;
  /** @brief FPU Intruction Pointer */
  uint32_t fip;
  /** @brief FPU Control Status */
  uint16_t fcs;
  /** @brief FPU Reserved */
  uint16_t reserved0;
  /** @brief FPU Data Pointer */
  uint32_t fdp;
  /** @brief FPU Data Pointer Selector */
  uint16_t fds;
  /** @brief FPU Reserved */
  uint16_t reserved1;
  /** @brief MXCSR Register */
  uint32_t mxcsr;
  /** @brief MXCSR Mask Register */
  uint32_t mxcsrMask;
  /** @brief Other SSE/FPU Work Register */
  uint8_t registers[];
} S_FXData;

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
 * @brief Returns the virtual CPU of the thread.
 *
 * @details Returns the virtual CPU of the thread.
 *
 * @return The current virtual CPU of the thread is returned.
 */
const S_VirtualCPU* CPUGetVirtualCPU(const S_KernelThread* kpThread);

/**
 * @brief Writes byte on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
static inline void CPUPortWriteByte(const uint8_t kValue, const uint16_t kPort)
{
  __asm__ __volatile__("outb %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Writes word on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
static inline void CPUPortWriteHalf(const uint16_t kValue, const uint16_t kPort)
{
  __asm__ __volatile__("outw %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Writes long on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
static inline void CPUPortWriteWord(const uint32_t kValue, const uint16_t kPort)
{
  __asm__ __volatile__("outl %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Reads byte on port.
 *
 * @return The value read from the port.
 *
 * @param[in] kPort The port to which the value has to be read.
 */
static inline uint8_t CPUPortReadByte(const uint16_t kPort)
{
  uint8_t rega;
  __asm__ __volatile__("inb %1,%0" : "=a" (rega) : "Nd" (kPort));
  return rega;
}

/**
 * @brief Reads word on port.
 *
 * @return The value read from the port.
 *
 * @param[in] kPort The port to which the value has to be read.
 */
static inline uint16_t CPUPortReadHalf(const uint16_t kPort)
{
  uint16_t rega;
  __asm__ __volatile__("inw %1,%0" : "=a" (rega) : "Nd" (kPort));
  return rega;
}

/**
 * @brief Reads long on port.
 *
 * @return The value read from the port.
 *
 * @param[in] kPort The port to which the value has to be read.
 */
static inline uint32_t CPUPortReadWord(const uint16_t kPort)
{
  uint32_t rega;
  __asm__ __volatile__("inl %1,%0" : "=a" (rega) : "Nd" (kPort));
  return rega;
}

/**
 * @brief Registers the LAPIC driver.
 *
 * @details Registers the LAPIC driver used by the core manager to end IPI,
 * start other CPUs and get the LAPIC Ids. This function must be called
 * before any other in the core manager. The function should only be called
 * once.
 *
 * @param[in] kpLAPICDriver The LAPIC driver to register.
 */
void CPURegisterLAPICDriver(const S_LAPICDriver* kpLAPICDriver);

/**
 * @brief Registers the LAPIC Timer driver.
 *
 * @details Registers the LAPIC Timer driver used by the core manager to
 * initialize the LAPIC Timer for secondary cores
 *
 * @param[in] kpLAPICDriver The LAPIC driver to register.
 */
void CPURegisterLAPICTimerDriver(const S_LAPICTimerDriver* kpLAPICTimerDrv);

/**
 * @brief Returns the CPU 1GB page support.
 *
 * @details Returns the CPU 1GB page support.
 *
 * @return Returns the CPU 1GB page support.
 */
bool CPUGet1GBPageSupport(void);

#endif /* #ifndef __CPU_X86_64_X64_CPU_H_ */

/************************************ EOF *************************************/