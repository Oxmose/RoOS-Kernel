/*******************************************************************************
 * @file CPUExceptions.c
 *
 * @see X64CPU.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 28/06/2026
 *
 * @version 1.0
 *
 * @brief x64 CPU exception handlers.
 *
 * @details x64 CPU exception handlers.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <Panic.h>
#include <X64Cpu.h>
#include <stdbool.h>
#include <CtrlBlock.h>
#include <Scheduler.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* None TODO */

/* Header file */
#include <X64Cpu.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Current module name */
#define MODULE_NAME "CPU_X64"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define CPU_ASSERT(COND, MSG, ERROR) {            \
  if ((COND) == false)                            \
  {                                               \
    PANIC(ERROR, MODULE_NAME, MSG, false);        \
  }                                               \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Handles a division by zero exception.
 *
 * @details Handles a divide by zero exception raised by the cpu. The thread
 * will be signaled.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _FPExceptionHandler(void);

/**
 * @brief Handles an invalid instruction exception.
 *
 * @details Handles an invalid instruction exception raised by the cpu. The
 * thread will be signaled.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _InvalidInstructionHandler(void);

/**
 * @brief Handles a debug CPU exception.
 *
 * @details Handles a debug CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _DebugExceptionHandler(void);

/**
 * @brief Handles a breakpoint CPU exception.
 *
 * @details Handles a breakpoint CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _BreakpointExceptionHandler(void);

/**
 * @brief Handles an overflow CPU exception.
 *
 * @details Handles a overflow CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _OverflowExceptionHandler(void);

/**
 * @brief Handles a bound range exceeded CPU exception.
 *
 * @details Handles a bound range exceeded CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _BoundRangeExceptionHandler(void);

/**
 * @brief Handles a device not available CPU exception.
 *
 * @details Handles a device not available CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _DeviceNotAvailableExceptionHandler(void);

/**
 * @brief Handles a double fault CPU exception.
 *
 * @details Handles a double fault CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _DoubleFaultHandler(void);

/**
 * @brief Handles a coprocessor segment overrun CPU exception.
 *
 * @details Handles a coprocessor segment overrun CPU exception raised by the
 * cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _CoprocSegmentOverrunExceptionHandler(void);

/**
 * @brief Handles an invalid TSS CPU exception.
 *
 * @details Handles an invalid TSS CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _InvalidTSSExceptionHandler(void);

/**
 * @brief Handles a segment not present CPU exception.
 *
 * @details Handles a segment not present CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _SegmentNotPresentExceptionHandler(void);

/**
 * @brief Handles a stack segment fault CPU exception.
 *
 * @details Handles a stack segment fault CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _StackSegmentFaultExceptionHandler(void);

/**
 * @brief Handles a general protection fault CPU exception.
 *
 * @details Handles a general protection fault CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _GeneralProtectionExceptionHandler(void);

/**
 * @brief Handles an alignement check CPU exception.
 *
 * @details Handles an alignement check CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _AlignementCheckExceptionHandler(void);

/**
 * @brief Handles a machine check CPU exception.
 *
 * @details Handles a machine check CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _MachineCheckExceptionHandler(void);

/**
 * @brief Handles a SIMD floating point CPU exception.
 *
 * @details Handles a SIMD floating point CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _SIMDFPExceptionHandler(void);

/**
 * @brief Handles a virtualization CPU exception.
 *
 * @details Handles a virtualization CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _VirtualizationExceptionHandler(void);

/**
 * @brief Handles a control protection CPU exception.
 *
 * @details Handles a control protection CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _ControlProtectionExceptionHandler(void);

/**
 * @brief Handles an hypervisor injection CPU exception.
 *
 * @details Handles an hypervisor injection CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _HypervisorInjectionExceptionHandler(void);

/**
 * @brief Handles a VMM communication CPU exception.
 *
 * @details Handles a VMM communication CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _VMMCommunicationExceptionHandler(void);

/**
 * @brief Handles a security CPU exception.
 *
 * @details Handles a security CPU exception raised by the cpu.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _SecurityExceptionHandler(void);

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

static bool _FPExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = DIVISION_BY_ZERO_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _InvalidInstructionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = INVALID_INSTRUCTION_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _DebugExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = DEBUG_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _BreakpointExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = BREAKPOINT_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _OverflowExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = OVERFLOW_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _BoundRangeExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = BOUND_RANGE_EXCEEDED_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _DeviceNotAvailableExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = DEVICE_NOT_AVAILABLE_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _DoubleFaultHandler(void)
{
  volatile bool test = false;
  while(!test){}
  CPURestoreContext(SchedulerGetCurrentThread());
  /* Double faults will directly crash the computer */
  PANIC(ERR_UNAUTHORIZED_ACTION, MODULE_NAME, "Double fault detected", true);

  return true;
}

static bool _CoprocSegmentOverrunExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = COPROC_SEGMENT_OVERRUN_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _InvalidTSSExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = INVALID_TSS_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _SegmentNotPresentExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = SEGMENT_NOT_PRESENT_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _StackSegmentFaultExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = STACK_SEGMENT_FAULT_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _GeneralProtectionExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  while(CPUGetId() != 0) {}

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = GENERAL_PROTECTION_FAULT_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _AlignementCheckExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = ALIGNEMENT_CHECK_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _MachineCheckExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = MACHINE_CHECK_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _SIMDFPExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = SIMD_FLOATING_POINT_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _VirtualizationExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = VIRTUALIZATION_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _ControlProtectionExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = CONTROL_PROTECTION_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _HypervisorInjectionExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = HYPERVISOR_INJECTION_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _VMMCommunicationExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = VMM_COMMUNICATION_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

static bool _SecurityExceptionHandler(void)
{
  S_KernelThread* pCurrThread;

  /* Fill the thread error table */
  pCurrThread = SchedulerGetCurrentThread();
  pCurrThread->errorTable.exceptionId = SECURITY_EXC_LINE;
  pCurrThread->errorTable.instAddr    = CPUGetContextIP(pCurrThread);
  pCurrThread->errorTable.pExecVCpu   = pCurrThread->pVCpu;

  /* Set the thread to non executable */
  SchedulerSetCurrentThreadErrored();

  return true;
}

void CPURegisterExceptions(void)
{
  E_Return error;

  error = InterruptRegister(DIVISION_BY_ZERO_EXC_LINE,
                            _FPExceptionHandler,
                            false);
  error |= InterruptRegister(DEBUG_EXC_LINE,
                             _DebugExceptionHandler,
                             false);
  error |= InterruptRegister(BREAKPOINT_EXC_LINE,
                             _BreakpointExceptionHandler,
                             false);
  error |= InterruptRegister(OVERFLOW_EXC_LINE,
                             _OverflowExceptionHandler,
                             false);
  error |= InterruptRegister(BOUND_RANGE_EXCEEDED_EXC_LINE,
                             _BoundRangeExceptionHandler,
                             false);
  error |= InterruptRegister(INVALID_INSTRUCTION_EXC_LINE,
                             _InvalidInstructionHandler,
                             false);
  error |= InterruptRegister(DEVICE_NOT_AVAILABLE_EXC_LINE,
                             _DeviceNotAvailableExceptionHandler,
                             false);
  error |= InterruptRegister(DOUBLE_FAULT_EXC_LINE,
                             _DoubleFaultHandler,
                             false);
  error |= InterruptRegister(COPROC_SEGMENT_OVERRUN_EXC_LINE,
                             _CoprocSegmentOverrunExceptionHandler,
                             false);
  error |= InterruptRegister(INVALID_TSS_EXC_LINE,
                             _InvalidTSSExceptionHandler,
                             false);
  error |= InterruptRegister(SEGMENT_NOT_PRESENT_EXC_LINE,
                             _SegmentNotPresentExceptionHandler,
                             false);
  error |= InterruptRegister(STACK_SEGMENT_FAULT_EXC_LINE,
                             _StackSegmentFaultExceptionHandler,
                             false);
  error |= InterruptRegister(GENERAL_PROTECTION_FAULT_EXC_LINE,
                             _GeneralProtectionExceptionHandler,
                             false);
  error |= InterruptRegister(X87_FLOATING_POINT_EXC_LINE,
                             _FPExceptionHandler,
                             false);
  error |= InterruptRegister(ALIGNEMENT_CHECK_EXC_LINE,
                             _AlignementCheckExceptionHandler,
                             false);
  error |= InterruptRegister(MACHINE_CHECK_EXC_LINE,
                             _MachineCheckExceptionHandler,
                             false);
  error |= InterruptRegister(SIMD_FLOATING_POINT_EXC_LINE,
                             _SIMDFPExceptionHandler,
                             false);
  error |= InterruptRegister(VIRTUALIZATION_EXC_LINE,
                             _VirtualizationExceptionHandler,
                             false);
  error |= InterruptRegister(CONTROL_PROTECTION_EXC_LINE,
                             _ControlProtectionExceptionHandler,
                             false);
  error |= InterruptRegister(HYPERVISOR_INJECTION_EXC_LINE,
                             _HypervisorInjectionExceptionHandler,
                             false);
  error |= InterruptRegister(VMM_COMMUNICATION_EXC_LINE,
                             _VMMCommunicationExceptionHandler,
                             false);
  error |= InterruptRegister(SECURITY_EXC_LINE,
                             _SecurityExceptionHandler,
                             false);

  CPU_ASSERT(error == NO_ERROR, "Failed to register CPU exceptions.", error);
}

/************************************ EOF *************************************/