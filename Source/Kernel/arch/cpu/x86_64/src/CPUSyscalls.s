;-------------------------------------------------------------------------------
;
; File: CPUSyscalls.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 29/08/2026
;
; Version: 1.0
;
; CPU System Call management functions
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
%define VCPU_OFF_KERNEL_STACK 0x10
%define VCPU_OFF_USER_STACK   0x18
%define VCPU_OFF_HANDLE       0x20
%define SYSCALL_RFLAGS_MASK   0x600

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern SystemCallDispatcher
extern CPUGetId
extern SignalManage

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global CPUSystemCallInit
global CPUSyscallHandler

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------
section .text
align 4

;-------------------------------------------------------------------------------
; Initialize the MSR registers for system calls
;
; Param:
;     RDI - The address of the system call handler
;     RSI - The user code segment selector
;     RDX - The kernel code segment selector
CPUSystemCallInit:
  ; Setup segment for user (exit)
  and rsi, 0xFFFF
  sub rsi, 16
  or  rsi, 3
  shl rsi, 16
  ; Setup segment for kernel (entry)
  and rdx, 0xFFFF

  ; Merge
  or  rdx, rsi
  xor rax, rax
  mov rcx, 0xC0000081
  wrmsr

  ; Setup the entry RIP
  mov rax, rdi
  mov rdx, rdi
  shr rdx, 32
  mov rcx, 0xC0000082
  wrmsr

  ; Setup the flags mask
  mov rax, SYSCALL_RFLAGS_MASK
  mov rdx, 0
  mov rcx, 0xC0000084
  wrmsr

  ret

;-------------------------------------------------------------------------------
; Handles a system call request from the user space.
;
; Param:
;     RDI - The system call ID to handle.
;     RSI - The first parameter for the system call.
;     RDX - The second parameter for the system call.
;     R10 - The third parameter for the system call.
;     R8  - The fourth parameter for the system call.
;     R9  - The fifth parameter for the system call.
CPUSyscallHandler:
  ; Switch from user to kernel stack
  swapgs
  mov rax, gs:0
  mov [rax + VCPU_OFF_USER_STACK], rsp
  mov rsp, [rax + VCPU_OFF_KERNEL_STACK]

  ; Ensure stack is aligned
  and rsp, 0xFFFFFFFFFFFFFFF0

  ; Create stack frame
  push rbx
  push r12
  push r13
  push r14
  push r15
  push rdi
  push rsi
  push rcx
  push r11
  push rbp
  push rax

  ; Move back the correct RCX value
  mov rcx, r10

  ; Relase the interrupts
  sti

  ; Call the system call dispatcher to handle the request
  call SystemCallDispatcher

  ; Ensure no interrupts are pending
  cli

  ; Save the system call return value in case a signal needs to be handled
  mov [rsp], rax

  ; Manage signals
  mov rdi, gs:0
  mov rdi, [rdi + VCPU_OFF_HANDLE]
  mov rsi, 1
  call SignalManage

  ; Restore the registers
  pop rax
  pop rbp
  pop r11
  pop rcx
  pop rsi
  pop rdi
  pop r15
  pop r14
  pop r13
  pop r12
  pop rbx

  ; Switch back to user stack
  mov rdx, gs:0
  mov rsp, [rdx + VCPU_OFF_USER_STACK]
  swapgs

  ; Return to user space
  o64 sysret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
; None

; EOF