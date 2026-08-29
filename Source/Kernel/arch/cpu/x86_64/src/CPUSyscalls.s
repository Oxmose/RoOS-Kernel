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
extern SystemCallDispatcher
extern CPUGetId

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
;     RSI - The kernel code segment selector
;     RDX - The kernel data segment selector
CPUSystemCallInit:
  ; Setup segments
  mov rax, rsi
  and rax, 0xFFFF
  and rdx, 0xFFFF
  or  rdx, 3
  shl rdx, 16
  or  rdx, rax
  xor rax, rax
  mov rcx, 0xC0000081
  wrmsr

  ; Setup the entry RIP
  mov rax, rdi
  mov rdx, rdi
  shr rdx, 32
  mov rcx, 0xC0000082
  wrmsr

  ; Setup the flags
  mov rax, 0
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
;     RCX - The third parameter for the system call.
;     R8  - The fourth parameter for the system call.
;     R9  - The fifth parameter for the system call.
CPUSyscallHandler:
  ; Create stack frame
  push rbp
  mov  rbp, rsp

  push rbx
  push rcx
  push r11

  ; Move back the correct RCX value
  mov rcx, r10

  ; Get the offset in the schedule contexts
  push rdx
  call CPUGetId
  mov rbx, 8
  mul rbx
  pop rdx

  ; Load the schedule context
  mov rbx, ppSchedulerContext
  mov rbx, [rbx]
  add rax, rbx
  mov rax, [rax]

  ; Load the thread vcpu
  mov rax, [rax]
  mov rax, [rax]

  ; Update to kernel stack
  mov  rbx, rsp
  mov  rsp, [rax + VCPU_OFF_KERNEL_STACK]
  push rbx

  ; Call the system call dispatcher to handle the request
  call SystemCallDispatcher

  ; Switch back to the user stack
  pop rsp

  ; Restore the registers
  pop r11
  pop rcx
  pop rbx
  pop rbp

  ; Return to user space
  o64 sysret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
; None

; EOF