/*******************************************************************************
 * @file CPU.h
 *
 * @see CPU.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Generic CPU management functions
 *
 * @details Generic CPU manipulation functions. The underlying platform should
 * implement those calls.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_CPU_H_
#define __CPU_CPU_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <CtrlBlock.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief CPU IPI send flag: send to all CPUs but the calling one. */
#define CPU_IPI_BROADCAST_TO_OTHER 0x10000

/** @brief CPU IPI send flag: send to all CPUs including the calling one. */
#define CPU_IPI_BROADCAST_TO_ALL 0x30000

/** @brief CPU IPI send flag: send to a specific CPU using its id. */
#define CPU_IPI_SEND_TO(X) ((X) & 0xFFFF)


/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Structure that contains the CPU interrupts configuration. */
typedef struct
{
  /** @brief Minimal interrupt line */
  uint32_t minInterruptLine;
  /** @brief Maximal interrupt line */
  uint32_t maxInterruptLine;
  /** @brief Kernel scheduling interrupt line */
  uint32_t schedulerInterruptLine;
  /** @brief Spurious interrupts line id */
  uint32_t spuriousInterruptLine;
  /** @brief IPI interrupt line id */
  uint32_t ipiInterruptLine;
} S_CPUInterruptConfiguration;

/** @brief Defines the IPI functions */
typedef enum
{
  /** @brief Panic function */
  IPI_FUNC_PANIC,
  /** @brief TLB invalidation function */
  IPI_FUNC_TLB_INVAL,
  /** @brief Scheduler call function */
  IPI_FUNC_SCHEDULE,
} E_IPIFunction;

/** @brief Defines the IPI parameters structure. */
typedef struct
{
  /** @brief IPI function to be used. */
  E_IPIFunction function;
  /** @brief Data for the function to be used */
  void* pData;
} S_IPIParameters;

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
 * @brief Initializes the CPU.
 *
 * @details Initializes the CPU registers and relevant structures.
 */
void CPUInit(void);

/**
 * @brief Initializes the core manager.
 *
 * @details Intializes the core manager. During initialization, secondary CPUs
 * detection and enabling is done if possible. After this call, it is possible
 * that more core execute in the system.
 */
void CPUStartSMP(void);

/**
 * @brief Entry C function for secondary cores.
 *
 * @details Entry C function for secondary cores. This function is called by the
 * secondary cores after initializing their state in the secondary core
 * startup function.
 *
 * @param[in] kCpuId The booted CPU identifier that call the function.
 *
 * @warning This function should never be called by the user, only the assembly
 * startup should call it.
 */
void CPUAPInit(const uint8_t kCpuId);

/**
 * @brief Returns the CPU's interrupt configuration.
 *
 * @details Returns the CPU's interrupt configuration. The configuration is
 * initialized during the CPU initialization.
 *
 * @return The CPU's interrupt configuration is returned.
 */
const S_CPUInterruptConfiguration* CPUGetInterruptConfig(void);

/**
 * @brief Updates the memory configuration for the running thread.
 *
 * @details Updates the memory configuration for the running thread. Depending
 * on the architecture, this might entails MMU/MPU configuration changes for
 * instance.
 *
 * @param[in, out] kpThread The current thread to update the memory
 * configuration for.
 */
void CPUUpdateMemoryConfig(const S_KernelThread* kpThread);

/**
 * @brief Returns the number of CPUs used in the current system.
 *
 * @details Returns the number of CPUs used in the current system.
 *
 * @return Returns the number of CPUs used in the current system.
 */
uint32_t CPUGetCount(void);

/**
 * @brief Returns the CPU main stack end address.
 *
 * @details Returns the CPU main stack end address.
 *
 * @param[in] kCPUId The CPU identifier for which the stack address is
 * requested.
 *
 * @return Returns the CPU main stack address.
 */
uintptr_t CPUGetStackEnd(const uint32_t kCPUId);

/**
 * @brief Returns the CPU main stack size.
 *
 * @details Returns the CPU main stack size.
 *
 * @return Returns the CPU main stack size.
 */
size_t CPUGetStackSize(void);

/**
 * @brief Creates a thread's virtual CPU.
 *
 * @details Initializes the thread's virtual CPU structure later used by the
 * CPU. This structure is private to the architecture and should not be used
 * elsewhere than the CPU module.
 *
 * @param[in] pThread The thread to initialize the vCPU of.
 *
 * @return The address of the newly created VCPU is returned.
 */
void* CPUCreateVirtualCPU(S_KernelThread* pThread);

/**
 * @brief Detroys a thread's virtual CPU.
 *
 * @details Detroys the thread's virtual CPU structure.
 *
 * @param[out] pThread The thread's structure to update.
 */
void CPUDestroyVirtualCPU(S_KernelThread* pThread);

/**
 * @brief Halts the CPU for lower energy consuption.
 *
 * @details Halts the CPU for lower energy consuption.
 */
void CPUHalt(void);

/**
 * @brief Sets the new page directory for the calling CPU.
 *
 * @details Sets the new page directory for the calling CPU. The page directory
 * address passed as parameter must be a physical address.
 *
 * @param[in] kNewPgDir The physical address of the new page directory.
 */
void CPUSetPageDirectory(const uintptr_t kNewPgDir);

/**
 * @brief Invalidates a page in the TLB that contains the virtual address given
 * as parameter.
 *
 * @details Invalidates a page in the TLB that contains the virtual address
 * given as parameter. This macro uses inline assembly to invocate the INVLPG
 * instruction.
 *
 * @param[in] kVirtAddress The virtual address contained in the page to
 * invalidate.
 */
void CPUInvalidateTLBEntry(const uintptr_t kVirtAddress);

/**
 * @brief Returns the CPU identifier of the calling code.
 *
 * @details Returns the CPU identifier of the calling code.
 *
 * @return The CPU identifier of the calling core is returned.
 */
uint32_t CPUGetId(void);

/**
 * @brief Returns the last interrupt registered for the virtual CPU.
 *
 * @details Returns the last interrupt registered for the virtual CPU.
 *
 * @param[in] pThread The thread that got interrupted.
 *
 * @return The current saved interrupt number.
 */
uint32_t CPUGetContextInterruptNumber(const S_KernelThread* kpThread);

/**
 * @brief Returns the last interrupt instruction pointer for the virtual CPU.
 *
 * @details Returns the last interrupt instruction pointer for the virtual CPU.
 *
 * @param[in] pThread The thread that got interrupted.
 *
 * @return The current saved interrupt instruction pointer.
 */
uintptr_t CPUGetContextIP(const S_KernelThread* kpThread);

/**
 * @brief Saves the CPU context of a thread.
 *
 * @details Saves the CPU context of a thread. This function will load the
 * current thread's vCPU and save the current CPU context.
 */
void CPUSaveContext(void);

/**
 * @brief Restores the CPU context of a thread.
 *
 * @details Restores the CPU context of a thread. This function will load the
 * thread's vCPU and replace the current context.
 *
 * @param[in] kpThread The thread of which the CPU should restore the context.
 */
void CPURestoreContext(const S_KernelThread* kpThread);

/**
 * @brief Registers the CPU exceptions.
 *
 * @details Registers the CPU exceptions. Since CPUs can have different
 * exceptions, its is up to the CPU to manage its own.
 *
 * @return The success or error state is returned.
 */
void CPURegisterExceptions(void);

/**
 * @brief Peforms a memory fence for acquire operations.
 *
 * @details Peforms a memory fence for acquire operations. This ensures all
 * accesses are performed after the fence.
 */
void CPUMemoryFenceAcquire(void);

/**
 * @brief Peforms a memory fence for release operations.
 *
 * @details Peforms a memory fence for release operations. This ensures all
 * accesses are performed before the fence.
 */
void CPUMemoryFenceRelease(void);

/**
 * @brief Sends an IPI to the cores.
 *
 * @details Sends an IPI to the cores. The flags define the nature of the
 * IPI, if it should be broadcasted, including the calling core, etc.
 *
 * @param[in] kFlags The flags to use, see IPI flags definition.
 * @param[in] kpParams The IPI parameters to use.
 */
void CPUSendIPI(const uint32_t kFlags, const S_IPIParameters* kpParams);

/**
 * @brief Returns the CPU physical address width.
 *
 * @details Returns the CPU physical address width.
 *
 * @return Returns the CPU physical address width.
 */
uint8_t CPUGetPhysicalAddressWidth(void);

/**
 * @brief Returns the CPU virtual address width.
 *
 * @details Returns the CPU virtual address width.
 *
 * @return Returns the CPU virtual address width.
 */
uint8_t CPUGetVirtualAddressWidth(void);

#endif /* #ifndef __CPU_CPU_H_ */

/************************************ EOF *************************************/