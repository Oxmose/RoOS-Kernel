;-------------------------------------------------------------------------------
;
; File: CPUContext.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 29/06/2026
;
; Version: 1.0
;
; CPU synchronization functions
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; INCLUDES
;-------------------------------------------------------------------------------
%include "config.inc"

;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 64]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------
; The following defines shall correspond to the virtual CPU context
%define VCPU_OFF_CTX 0x0
%define VCPU_OFF_FXD 0x8
%define VCPU_OFF_KERNEL_STACK 0x10

%define USER_DS_64 0x18
%define USER_CS_64 0x20

%define KERNEL_CS_64 0x08

%define USER_THREAD_INIT_RFLAGS 0x202

%define SIG_SYSCALL_CTX_OFF_RSP 0x0
%define SIG_SYSCALL_CTX_OFF_RBP 0x8
%define SIG_SYSCALL_CTX_OFF_R11 0x10
%define SIG_SYSCALL_CTX_OFF_RCX 0x28
%define SIG_SYSCALL_CTX_OFF_RAX 0x30
%define SIG_SYSCALL_CTX_OFF_R15 0x38
%define SIG_SYSCALL_CTX_OFF_R14 0x40
%define SIG_SYSCALL_CTX_OFF_R13 0x48
%define SIG_SYSCALL_CTX_OFF_R12 0x50
%define SIG_SYSCALL_CTX_OFF_RBX 0x58

%define SIG_INTERRUPT_CTX_OFF_RIP 0x0
%define SIG_INTERRUPT_CTX_OFF_RFLAGS 0x8
%define SIG_INTERRUPT_CTX_OFF_RSP 0x10
%define SIG_INTERRUPT_CTX_OFF_RBP 0x18
%define SIG_INTERRUPT_CTX_OFF_GSBASE 0x20
%define SIG_INTERRUPT_CTX_OFF_R8 0x28
%define SIG_INTERRUPT_CTX_OFF_R9 0x30
%define SIG_INTERRUPT_CTX_OFF_R10 0x38
%define SIG_INTERRUPT_CTX_OFF_R11 0x40
%define SIG_INTERRUPT_CTX_OFF_R12 0x48
%define SIG_INTERRUPT_CTX_OFF_R13 0x50
%define SIG_INTERRUPT_CTX_OFF_R14 0x58
%define SIG_INTERRUPT_CTX_OFF_R15 0x60
%define SIG_INTERRUPT_CTX_OFF_RDI 0x68
%define SIG_INTERRUPT_CTX_OFF_RSI 0x70
%define SIG_INTERRUPT_CTX_OFF_RDX 0x78
%define SIG_INTERRUPT_CTX_OFF_RCX 0x80
%define SIG_INTERRUPT_CTX_OFF_RBX 0x88
%define SIG_INTERRUPT_CTX_OFF_RAX 0x90
%define SIG_INTERRUPT_CTX_OFF_FXDATA 0x98

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
extern ppSchedulerContext

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern SchedulerSchedule

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global CPUGetId
global CPUSaveContext
global CPURestoreContext
global CPUSaveContextAndSchedule
global CPUEnterUserSpace
global CPURestoreContextFromInterruptSignal
global CPURestoreContextFromSyscallSignal

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
global _xSaveoptSupported

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------
section .text
align 4

;-------------------------------------------------------------------------------
; Get the CPU Id
;
; Param:
;     None

CPUGetId:
  ; Get the core id
  mov rcx, 0xC0000103
  rdmsr
  ret

;-------------------------------------------------------------------------------
; Save the CPU context of a thread
;
; Param:
;     None
CPUSaveContext:
  ; Save return address
  push rax
  push rbx
  mov  rax, [rsp + 16]
  mov  rbx, [rsp + 8]
  mov  [rsp + 16], rbx
  mov  rbx, [rsp]
  mov  [rsp + 8], rbx
  add  rsp, 8

  ; Save the rest of the context
  push rcx
  push rdx
  push rsi
  push rdi

  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8

  ; Restore the return address
  mov r15, rax

  ; Save user GS that is swapped to kernel now
  mov rcx, 0xC0000102
  rdmsr
  shl rdx, 32
  or  rax, rdx
  push rax

  push rbp
  push rsp

  ; Load the thread vcpu
  mov rax, gs:0

  ; Save the old context and update the new one
  mov  rdi, [rax + VCPU_OFF_CTX]
  push rdi
  mov  [rax + VCPU_OFF_CTX], rsp

  ; Save the FxData
  mov rdi, [rax + VCPU_OFF_FXD]
  mov rax, 0xFFFFFFFFFFFFFFFF
  mov rdx, 0xFFFFFFFFFFFFFFFF
  mov rbx, [_xSaveoptSupported]
  cmp rbx, 0
  je __noXSaveopt
  xsaveopt64 [rdi]
  jmp __saveContextEnd
__noXSaveopt:
  xsave64 [rdi]

__saveContextEnd:
  ; Restore the return address
  jmp r15


;-------------------------------------------------------------------------------
; Save the CPU context of a thread and call the scheduler
;
; Param:
;     RDI - The VCPU of the current thread
CPUSaveContextAndSchedule:
  ; Save the current stack pointer, RAX, RDI, RSI are scratch registers
  mov rax, rsp
  mov rdx, ss
  mov rcx, cs

  ; Create interrupt stack
  push rdx
  push rax
  pushfq
  push rcx
  push __saveContextReturn
  push 0
  push 0

  ; Context
  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi

  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8

  ; Save user GS that is swapped to kernel now
  mov rcx, 0xC0000102
  rdmsr
  shl rdx, 32
  or  rax, rdx
  push rax

  push rbp
  push rsp

  ; Save the old context
  mov  rax, [rdi + VCPU_OFF_CTX]
  push rax

  ; Save the new context
  mov [rdi + VCPU_OFF_CTX], rsp

  ; Save the FxData
  mov rax, 0xFFFFFFFFFFFFFFFF
  mov rdx, 0xFFFFFFFFFFFFFFFF
  mov rdi, [rdi + VCPU_OFF_FXD]
  mov rbx, [_xSaveoptSupported]
  cmp rbx, 0
  je __noXSaveopt1
  xsaveopt64 [rdi]
  jmp __saveContextEnd1
__noXSaveopt1:
  xsave64 [rdi]

__saveContextEnd1:
  call SchedulerSchedule

__saveContextReturn:
  ret

;-------------------------------------------------------------------------------
; Restore the CPU context of a thread
;
; Param:
;     RDI - The pointer to the thread to restore
CPURestoreContext:
  ; The current thread is sent as parameter, load the VCPU
  mov rcx, [rdi]

  ; Restore the FxData
  mov rax, 0xFFFFFFFFFFFFFFFF
  mov rdx, 0xFFFFFFFFFFFFFFFF
  mov rsi, [rcx + VCPU_OFF_FXD]
  xrstor64 [rsi]

  ; Restore the stack pointer
  mov rsp, [rcx + VCPU_OFF_CTX]

  ; Restore the saved context link
  pop rax
  mov [rcx + VCPU_OFF_CTX], rax

  ; Restore registers
  add rsp, 8 ; Skip RSP in CPU state, it is the same as context
  pop rbp

  ; Restore user GS that is swapped to kernel now
  pop rax
  mov rdx, rax
  shr rdx, 32
  mov ecx, 0xC0000102
  wrmsr

  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15

  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax

  ; Swap GS back if needed
  push   rax
  mov    rax, [rsp + 0x20]
  cmp    rax, KERNEL_CS_64   ; check if the interrupt came from user space
  je     __noUserSwapGS
  swapgs
__noUserSwapGS:
  pop rax

  ; Discard the interrupt context
  add rsp, 16

  ; Return from interrupt
  iretq

;-------------------------------------------------------------------------------
; Enters the user space for the thread provided in parameters.
;
; Param:
;     RDI - The pointer to the thread user stack
;     RSI - The entry point of the thread
;     RDX - The arguments of the thread
CPUEnterUserSpace:
  ; Open the stack
  sub rdi, 8
  mov rsp, rdi

  ; SS
  mov  rax, USER_DS_64
  or   rax, 0x3
  push rax

  ; RSP
  push rdi

  ; RFLAGS
  push USER_THREAD_INIT_RFLAGS

  ; CS
  mov  rax, USER_CS_64
  or   rax, 0x3
  push rax

  ; RIP
  push rsi

  ; Get the arguments
  mov rdi, rdx

  ; Swap GS
  swapgs

  ; Return to user space
  iretq

;-------------------------------------------------------------------------------
; Returns to the regular execution flow after a signal handler has been
; executed. Use this function when returning from an interrupt.
;
; Param:
;     RDI - The user context to return to.
CPURestoreContextFromInterruptSignal:
  ; Restore the FxData
  mov rsi, rdi
  add rsi, SIG_INTERRUPT_CTX_OFF_FXDATA
  add rsi, 0x3F
  and rsi, 0xFFFFFFFFFFFFFFC0
  mov rax, 0xFFFFFFFFFFFFFFFF
  mov rdx, 0xFFFFFFFFFFFFFFFF
  xrstor64 [rsi]

  ; Restore user GS that is swapped to kernel now
  mov rax, [rdi + SIG_INTERRUPT_CTX_OFF_GSBASE]
  mov rdx, rax
  shr rdx, 32
  mov ecx, 0xC0000102
  wrmsr

  ; Prepare the return stack SS:RSP
  mov  rax, USER_DS_64
  or   rax, 0x3
  push rax
  mov rax, [rdi + SIG_INTERRUPT_CTX_OFF_RSP]
  push rax

  ; Prepare the return stack RFLAGS
  mov rax, [rdi + SIG_INTERRUPT_CTX_OFF_RFLAGS]
  push rax

  ; Prepare the return stack CS:RIP
  mov  rax, USER_CS_64
  or   rax, 0x3
  push rax
  mov rax, [rdi + SIG_INTERRUPT_CTX_OFF_RIP]
  push rax

  ; Restore the rest of the registers
  mov rbp, [rdi + SIG_INTERRUPT_CTX_OFF_RBP]
  mov r8,  [rdi + SIG_INTERRUPT_CTX_OFF_R8]
  mov r9,  [rdi + SIG_INTERRUPT_CTX_OFF_R9]
  mov r10, [rdi + SIG_INTERRUPT_CTX_OFF_R10]
  mov r11, [rdi + SIG_INTERRUPT_CTX_OFF_R11]
  mov r12, [rdi + SIG_INTERRUPT_CTX_OFF_R12]
  mov r13, [rdi + SIG_INTERRUPT_CTX_OFF_R13]
  mov r14, [rdi + SIG_INTERRUPT_CTX_OFF_R14]
  mov r15, [rdi + SIG_INTERRUPT_CTX_OFF_R15]

  mov rsi, [rdi + SIG_INTERRUPT_CTX_OFF_RSI]
  mov rdx, [rdi + SIG_INTERRUPT_CTX_OFF_RDX]
  mov rcx, [rdi + SIG_INTERRUPT_CTX_OFF_RCX]
  mov rbx, [rdi + SIG_INTERRUPT_CTX_OFF_RBX]
  mov rax, [rdi + SIG_INTERRUPT_CTX_OFF_RAX]
  mov rdi, [rdi + SIG_INTERRUPT_CTX_OFF_RDI]

  ; Swap GS
  swapgs

  ; Return to user space
  iretq

;-------------------------------------------------------------------------------
; Returns to the regular execution flow after a signal handler has been
; executed. Use this function when returning from a system call.
;
; Param:
;     RDI - The user context to return to.
CPURestoreContextFromSyscallSignal:
  ; Restore the user context
  mov rsp, [rdi + SIG_SYSCALL_CTX_OFF_RSP]
  mov rbp, [rdi + SIG_SYSCALL_CTX_OFF_RBP]
  mov r11, [rdi + SIG_SYSCALL_CTX_OFF_R11]
  mov rcx, [rdi + SIG_SYSCALL_CTX_OFF_RCX]
  mov rax, [rdi + SIG_SYSCALL_CTX_OFF_RAX]
  mov r15, [rdi + SIG_SYSCALL_CTX_OFF_R15]
  mov r14, [rdi + SIG_SYSCALL_CTX_OFF_R14]
  mov r13, [rdi + SIG_SYSCALL_CTX_OFF_R13]
  mov r12, [rdi + SIG_SYSCALL_CTX_OFF_R12]
  mov rbx, [rdi + SIG_SYSCALL_CTX_OFF_RBX]

  ; Swap GS
  swapgs

  ; Return to user space
  o64 sysret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .data
align 4
_xSaveoptSupported:
  dd 0

; EOF