/*******************************************************************************
 * @file Panic.c
 *
 * @see Panic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* Included headers */
#include <CPU.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <X64Cpu.h>
#include <Memory.h>
#include <stdbool.h>
#include <Critical.h>
#include <Scheduler.h>
#include <DebugOutput.h>

/* Configuration files */
#include <config.h>

/* Unit test header */
/* TODO */

/* Header file */
#include <Panic.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/** @brief Defines the stack trace size */
#define STACK_TRACE_SIZE 6
/** @brief Defines the panic print buffer size. */
#define PANIC_PRINT_BUFFER_SIZE 512

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Prints a formated string to the screen. Used in panic for its lack
 * of locks.
 *
 * @details Prints the desired string to the screen. This uses the generic
 * graphic driver to output data. Used in panic for its lack of locks.
 *
 * @param[in] kpFmt The format string to output.
 * @param[in] ... format's parameters.
 */
static void _PanicPrintf(const char* kpFmt, ...);

/**
 * @brief Prints the kernel panic screen header.
 *
 * @details Prints the kernel panic screen header. The header contains the
 * title, the interrupt number, the error code and the error string that caused
 * the panic.
 *
 * @param[in] kpVCpu The pointer to the VCPU state at the moment of the panic.
 * @param[in] kInterrupt Tells if the panic occured in an interrupt.
 */
static void _PrintHeader(const S_VirtualCPU* kpVCpu, const bool kInterrupt);

/**
 * @brief Prints the CPU state at the moment of the panic.
 *
 * @details Prints the CPU state at the moment of the panic. All CPU registers
 * are dumped.
 *
 * @param[in] kpVCpu The pointer to the VCPU state at the moment of the panic.
 */
static void _PrintCpuState(const S_VirtualCPU* kpVCpu);

/**
 * @brief Prints the CPU flags at the moment of the panic.
 *
 * @details Prints the CPU flags at the moment of the panic. The flags are
 * pretty printed for better reading.
 *
 * @param[in] kpVCpu The pointer to the VCPU state at the moment of the panic.
 */
static void _PrintCpuFlags(const S_VirtualCPU* kpVCpu);

/**
 * @brief Prints the stack frame rewind at the moment of the panic.
 *
 * @details Prints the stack frame rewind at the moment of the panic. The frames
 * will be unwinded and the symbols printed based on the information passed by
 * multiboot at initialization time.
 *
 * @param[in] lastRPB The last RBP in the stack to display.
 */
static void _PrintStackTrace(uintptr_t* lastRPB);

/**
 * @brief Builds a VCPU when the panic is not called from an interrupt context.
 *
 * @details Builds a VCPU when the panic is not called from an interrupt
 * context. The VCPU is a copy of the current CPU state.
 *
 * @param[out] pVCPU The virtual CPU to create.
 */
static void _BuildVCPU(S_VirtualCPU* pVCPU);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the current kernel panic error code. */
static uint32_t sPanicCode = 0;

/** @brief Stores the line at which the kernel panic was called. */
static uint32_t sPanicLine = 0;

/** @brief Stores the file from which the panic was called. */
static const char* skpPanicFile = NULL;

/** @brief Stores the module related to the panic. */
static const char* skpPanicModule = NULL;

/** @brief Stores the message related to the panic. */
static const char* skpPanicMsg = NULL;

/** @brief Panic lock */
static T_Spinlock sLock = SPINLOCK_INIT_VALUE;

/** @brief Panic print buffer cursor. */
static size_t sPanicBufferSize = 0;

/** @brief Panic print buffer. */
static char sPrintBuffer[PANIC_PRINT_BUFFER_SIZE];

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static void _PanicPrintf(const char* kpFmt, ...)
{
  __builtin_va_list args;

  if (kpFmt == NULL)
  {
    return;
  }

  __builtin_va_start(args, kpFmt);
  sPanicBufferSize = vsnprintf(sPrintBuffer,
                               PANIC_PRINT_BUFFER_SIZE,
                               kpFmt,
                               args);
  __builtin_va_end(args);

  /* Display */
  sPrintBuffer[sPanicBufferSize] = 0;
#if OUTPUT_DEBUG_ENABLE
  /* Enable the debug output port */
  DebugOutputPutString(sPrintBuffer);
#endif
  sPanicBufferSize = 0;
}

static void _PrintHeader(const S_VirtualCPU* kpVCpu, const bool kInterrupt)
{
  const S_InterruptContext* kpIntState;

  kpIntState = &kpVCpu->intContext;
  _PanicPrintf("\n/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\"
               "    KERNEL PANIC    "
               "/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n");
  _PanicPrintf("Interrupt: 0x%02X | %s | CPU %d | ",
               kpIntState->intId,
               kInterrupt ? "Int" : "Soft",
               CPUGetId());
  _PanicPrintf("Code: 0x%X | ", kpIntState->errorCode);
  switch(kpIntState->intId)
  {
    case 0:
      _PanicPrintf("Division by zero                  ");
      break;
    case 1:
      _PanicPrintf("Single-step interrupt             ");
      break;
    case 2:
      _PanicPrintf("Non maskable interrupt            ");
      break;
    case 3:
      _PanicPrintf("Breakpoint                        ");
      break;
    case 4:
      _PanicPrintf("Overflow                          ");
      break;
    case 5:
      _PanicPrintf("Bounds                            ");
      break;
    case 6:
      _PanicPrintf("Invalid Opcode                    ");
      break;
    case 7:
      _PanicPrintf("Coprocessor not available         ");
      break;
    case 8:
      _PanicPrintf("Double fault                      ");
      break;
    case 9:
      _PanicPrintf("Coprocessor Segment Overrun       ");
      break;
    case 10:
      _PanicPrintf("Invalid Task State Segment        ");
      break;
    case 11:
      _PanicPrintf("Segment not present               ");
      break;
    case 12:
      _PanicPrintf("Stack Fault                       ");
      break;
    case 13:
      _PanicPrintf("General protection fault          ");
      break;
    case 14:
      _PanicPrintf("Page fault                        ");
      break;
    case 16:
      _PanicPrintf("Math Fault                        ");
      break;
    case 17:
      _PanicPrintf("Alignment Check                   ");
      break;
    case 18:
      _PanicPrintf("Machine Check                     ");
      break;
    case 19:
      _PanicPrintf("SIMD Floating-Point Exception     ");
      break;
    case 20:
      _PanicPrintf("Virtualization Exception          ");
      break;
    case 21:
      _PanicPrintf("Control Protection Exception      ");
      break;
    default:
      _PanicPrintf("Panic generated by the kernel     ");
  }
  _PanicPrintf("\nFaulted at 0x%p\n", kpIntState->rip);
}

static void _PrintCpuState(const S_VirtualCPU* kpVCpu)
{
  const S_CPUState*         kpCPUState;
  const S_InterruptContext* kpIntState;
  uint64_t                  CR0;
  uint64_t                  CR2;
  uint64_t                  CR3;
  uint64_t                  CR4;

  kpCPUState = &kpVCpu->cpuState;
  kpIntState = &kpVCpu->intContext;

  __asm__ __volatile__ ("mov %%cr0, %%rax\n\t"
                        "mov %%rax, %0\n\t"
                        "mov %%cr2, %%rax\n\t"
                        "mov %%rax, %1\n\t"
                        "mov %%cr3, %%rax\n\t"
                        "mov %%rax, %2\n\t"
                        "mov %%cr4, %%rax\n\t"
                        "mov %%rax, %3\n\t"
                        : "=m" (CR0), "=m" (CR2), "=m" (CR3), "=m" (CR4)
                        : /* no input */
                        : "%rax");
  _PanicPrintf("                              ==== Core Dump ====\n");
  _PanicPrintf("RAX: 0x%p | RBX: 0x%p | RCX: 0x%p\n",
                kpCPUState->rax,
                kpCPUState->rbx,
                kpCPUState->rcx);
  _PanicPrintf("RDX: 0x%p | RSI: 0x%p | RDI: 0x%p \n",
                kpCPUState->rdx,
                kpCPUState->rsi,
                kpCPUState->rdi);
  _PanicPrintf("RBP: 0x%p | RSP: 0x%p | R8:  0x%p\n",
                kpCPUState->rbp,
                kpCPUState->rsp,
                kpCPUState->r8);
  _PanicPrintf("R9:  0x%p | R10: 0x%p | R11: 0x%p\n",
                kpCPUState->r9,
                kpCPUState->r10,
                kpCPUState->r11);
  _PanicPrintf("R12: 0x%p | R13: 0x%p | R14: 0x%p\n",
                kpCPUState->r12,
                kpCPUState->r13,
                kpCPUState->r14);
  _PanicPrintf("R15: 0x%p\n", kpCPUState->r15);
  _PanicPrintf("CR0: 0x%p | CR2: 0x%p | CR3: 0x%p\nCR4: 0x%p\n",
                CR0,
                CR2,
                CR3,
                CR4);
  _PanicPrintf("CS: 0x%04X | DS: 0x%04X | SS: 0x%04X | ES: 0x%04X | "
                "FS: 0x%04X | GS: 0x%04X\n",
                kpIntState->cs & 0xFFFF,
                kpCPUState->ds & 0xFFFF,
                kpCPUState->ss & 0xFFFF,
                kpCPUState->es & 0xFFFF ,
                kpCPUState->fs & 0xFFFF ,
                kpCPUState->gs & 0xFFFF);
}

static void _PrintCpuFlags(const S_VirtualCPU* kpVCpu)
{
  const S_InterruptContext* kpIntState;

  kpIntState = &kpVCpu->intContext;

  int8_t cf_f = (kpIntState->rflags & 0x1);
  int8_t pf_f = (kpIntState->rflags & 0x4) >> 2;
  int8_t af_f = (kpIntState->rflags & 0x10) >> 4;
  int8_t zf_f = (kpIntState->rflags & 0x40) >> 6;
  int8_t sf_f = (kpIntState->rflags & 0x80) >> 7;
  int8_t tf_f = (kpIntState->rflags & 0x100) >> 8;
  int8_t if_f = (kpIntState->rflags & 0x200) >> 9;
  int8_t df_f = (kpIntState->rflags & 0x400) >> 10;
  int8_t of_f = (kpIntState->rflags & 0x800) >> 11;
  int8_t nf_f = (kpIntState->rflags & 0x4000) >> 14;
  int8_t rf_f = (kpIntState->rflags & 0x10000) >> 16;
  int8_t vm_f = (kpIntState->rflags & 0x20000) >> 17;
  int8_t ac_f = (kpIntState->rflags & 0x40000) >> 18;
  int8_t id_f = (kpIntState->rflags & 0x200000) >> 21;
  int8_t iopl0_f = (kpIntState->rflags & 0x1000) >> 12;
  int8_t iopl1_f = (kpIntState->rflags & 0x2000) >> 13;
  int8_t vif_f = (kpIntState->rflags & 0x8000) >> 19;
  int8_t vip_f = (kpIntState->rflags & 0x100000) >> 20;

  _PanicPrintf("RFLAGS: 0x%p | ", kpIntState->rflags);

  if (cf_f != 0)
  {
    _PanicPrintf("CF ");
  }
  if (pf_f != 0)
  {
    _PanicPrintf("PF ");
  }
  if (af_f != 0)
  {
    _PanicPrintf("AF ");
  }
  if (zf_f != 0)
  {
    _PanicPrintf("ZF ");
  }
  if (sf_f != 0)
  {
    _PanicPrintf("SF ");
  }
  if (tf_f != 0)
  {
    _PanicPrintf("TF ");
  }
  if (if_f != 0)
  {
    _PanicPrintf("IF ");
  }
  if (df_f != 0)
  {
    _PanicPrintf("DF ");
  }
  if (of_f != 0)
  {
    _PanicPrintf("OF ");
  }
  if (nf_f != 0)
  {
    _PanicPrintf("NT ");
  }
  if (rf_f != 0)
  {
    _PanicPrintf("RF ");
  }
  if (vm_f != 0)
  {
    _PanicPrintf("VM ");
  }
  if (ac_f != 0)
  {
    _PanicPrintf("AC ");
  }
  if (vif_f != 0)
  {
    _PanicPrintf("VF ");
  }
  if (vip_f != 0)
  {
    _PanicPrintf("VP ");
  }
  if (id_f != 0)
  {
    _PanicPrintf("ID ");
  }
  if ((iopl0_f | iopl1_f) != 0)
  {
    _PanicPrintf("IO: %d ", (iopl0_f | iopl1_f << 1));
  }
  _PanicPrintf("\n");
}

static void _PrintStackTrace(uintptr_t* lastRBP)
{
  size_t                  i;
  uintptr_t               callAddr;
  S_KernelProcess         dummyProcess;
  S_ProcessMemoryMetadata metadata;
  bool                    isMapped;

  __asm__ __volatile__ ("mov %%cr3, %%rax\n\t"
                        "mov %%rax, %0\n\t"
                        : "=m" (metadata.PDPhysAddress)
                        : /* no input */
                        : "%rax");

  _PanicPrintf("                              ==== Stack Trace ====\n");
  /* Get the return address */
  _PanicPrintf("[0] 0x%p", lastRBP);

  dummyProcess.pMemoryData = &metadata;
  isMapped = MemoryIsMapped((uintptr_t)lastRBP,
                              2 * sizeof(uintptr_t),
                              &dummyProcess,
                              true);
  for (i = 1; i < STACK_TRACE_SIZE && isMapped == true; ++i)
  {
    callAddr = *(lastRBP + 1);

    if (callAddr == 0x0) break;

    if (i != 0 && i % 3 == 0)
    {
      _PanicPrintf("\n");
    }
    else if (i != 0)
    {
      _PanicPrintf(" | ");
    }

    _PanicPrintf("[%u] 0x%p", i, callAddr);
    lastRBP  = (uintptr_t*)*lastRBP;
    isMapped = MemoryIsMapped((uintptr_t)lastRBP,
                              2 * sizeof(uintptr_t),
                              &dummyProcess,
                              true);
  }
}

static void _BuildVCPU(S_VirtualCPU* pVCPU)
{
  /* Get the CPU registers */
  __asm__ __volatile__ ("mov %%rsp, %0\n\t" : "=m" (pVCPU->cpuState.rbp));
  __asm__ __volatile__ ("mov %%rbp, %0\n\t" : "=m" (pVCPU->cpuState.rsp));
  __asm__ __volatile__ ("mov %%rdi, %0\n\t" : "=m" (pVCPU->cpuState.rdi));
  __asm__ __volatile__ ("mov %%rsi, %0\n\t" : "=m" (pVCPU->cpuState.rsi));
  __asm__ __volatile__ ("mov %%rdx, %0\n\t" : "=m" (pVCPU->cpuState.rdx));
  __asm__ __volatile__ ("mov %%rcx, %0\n\t" : "=m" (pVCPU->cpuState.rcx));
  __asm__ __volatile__ ("mov %%rbx, %0\n\t" : "=m" (pVCPU->cpuState.rbx));
  __asm__ __volatile__ ("mov %%rax, %0\n\t" : "=m" (pVCPU->cpuState.rax));
  __asm__ __volatile__ ("mov %%r8, %0\n\t"  : "=m" (pVCPU->cpuState.r8));
  __asm__ __volatile__ ("mov %%r9, %0\n\t"  : "=m" (pVCPU->cpuState.r9));
  __asm__ __volatile__ ("mov %%r10, %0\n\t" : "=m" (pVCPU->cpuState.r10));
  __asm__ __volatile__ ("mov %%r11, %0\n\t" : "=m" (pVCPU->cpuState.r11));
  __asm__ __volatile__ ("mov %%r12, %0\n\t" : "=m" (pVCPU->cpuState.r12));
  __asm__ __volatile__ ("mov %%r13, %0\n\t" : "=m" (pVCPU->cpuState.r13));
  __asm__ __volatile__ ("mov %%r14, %0\n\t" : "=m" (pVCPU->cpuState.r14));
  __asm__ __volatile__ ("mov %%r15, %0\n\t" : "=m" (pVCPU->cpuState.r15));
  __asm__ __volatile__ ("mov %%ss, %0\n\t"  : "=m" (pVCPU->cpuState.ss));
  __asm__ __volatile__ ("mov %%gs, %0\n\t"  : "=m" (pVCPU->cpuState.gs));
  __asm__ __volatile__ ("mov %%es, %0\n\t"  : "=m" (pVCPU->cpuState.es));
  __asm__ __volatile__ ("mov %%ds, %0\n\t"  : "=m" (pVCPU->cpuState.ds));


  /* Set dummy interrupt values */
  __asm__ __volatile__ ("mov %%cs, %0\n\t"  : "=m" (pVCPU->intContext.cs));
  pVCPU->intContext.rip = (uintptr_t)KernelPanic;
  pVCPU->intContext.errorCode = 0xFFFF;
  pVCPU->intContext.intId     = 0xFFFF;

  /* Save current state */
  __asm__ __volatile__("pushfq\n\t"
                       "pop %0\n\t"
                       : "=g" (pVCPU->intContext.rflags)
                       :
                       : "memory");
}

void KernelPanic(const uint32_t kErrorCode,
                 const char*    kpModule,
                 const char*    kpMsg,
                 const char*    kpFile,
                 const size_t   kLine,
                 const bool     kFromInterrupt)
{
  const S_VirtualCPU* kpVCPU;
  S_VirtualCPU        buildVCPU;
  S_IPIParameters     ipiParams;

  /* We don't need interrupt anymore */
  InterruptDisable();

  /* Lock the panic */
  SpinlockAcquire(&sLock);

  /* Send the cores IPI */
  ipiParams.function = IPI_FUNC_PANIC;
  ipiParams.pData    = NULL;
  CPUSendIPI(CPU_IPI_BROADCAST_TO_OTHER, &ipiParams);

  /* Build the VCPU when not in an interrupt context */
  if (kFromInterrupt == false)
  {
    _BuildVCPU(&buildVCPU);
    kpVCPU = &buildVCPU;
  }
  else
  {
    kpVCPU = CPUGetVirtualCPU(SchedulerGetCurrentThread());
  }

  /* Set the parameters */
  sPanicCode     = kErrorCode;
  skpPanicModule = kpModule;
  skpPanicMsg    = kpMsg;
  skpPanicFile   = kpFile;
  sPanicLine     = kLine;

  _PrintHeader(kpVCPU, kFromInterrupt);
  _PrintCpuState(kpVCPU);
  _PrintCpuFlags(kpVCPU);

  /* Print panic information */
  _PanicPrintf("                              ==== Information ====\n");
  if (skpPanicFile != NULL)
  {
    _PanicPrintf("%s at line %d\n", skpPanicFile, sPanicLine);
  }
  if (skpPanicModule != NULL && skpPanicModule[0] != 0)
  {
    _PanicPrintf("[%s] | ", skpPanicModule);
  }
  if (skpPanicMsg != NULL)
  {
    _PanicPrintf("%s (Error %d)\n", skpPanicMsg, sPanicCode);
  }

  /* Print panic stack trace */
  _PrintStackTrace((uintptr_t*)kpVCPU->cpuState.rbp);

  /* Wait intefinitely */
  while (1)
  {
    InterruptDisable();
    CPUHalt();
  }
}

void KernelPanicSecondary(void)
{
  /* Wait intefinitely */
  while (1)
  {
    InterruptDisable();
    CPUHalt();
  }
}

/************************************ EOF *************************************/