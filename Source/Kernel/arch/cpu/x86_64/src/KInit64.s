;-------------------------------------------------------------------------------
;
; File: kInit64.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 28/06/2026
;
; Version: 1.0
;
; Kernel entry point and CPU initialization. This module setup high-memory
; kernel by mapping the first 4MB of memory 1:1 and 4MB in high memory.
; Paging is enabled.
; BSS is initialized (zeroed)
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
; None

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
extern _KERNEL_STACKS_BASE
extern _kernelPDP
extern _kernelPGDir

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern X64KernelEntry

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global __kInitx86_64

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
global _bootedCPUCount

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
align 4
__kInitx86_64:
  ; Write the core ID into PID
  xor rax, rax
  xor rdx, rdx
  mov rcx, 0xC0000103
  wrmsr


__stackInit:
  ; Init stack
  mov rax, _KERNEL_STACKS_BASE
  mov rbx, KERNEL_STACK_SIZE - 16
  add rax, rbx
  mov rbx, 0
  mov [rax], rbx
  mov rsp, rax
  mov rbp, 0

  ; Update the booted CPU count
  mov eax, 1
  mov rbx, _bootedCPUCount
  mov [rbx], eax

; __kInitCleanPGDir:
;   ; Clean the low entries in the kernel page directory
;   mov rbx, 0
;   mov rax, _kernelPDP
;   mov [rax], rbx
;   mov rax, _kernelPGDir
;   mov [rax], rbx

  ; Jump to kernel entry
  call X64KernelEntry

__kInitx64End:
  mov rax, 0xFFFFFFFF800B8F00
  mov rbx, _kinitEndOfLine
  mov cl,  0xF0

__kInitx64EndPrint:
  mov dl, [rbx]
  cmp dl, 0
  jbe __kInitx64EndPrintEnd
  mov [rax], dl
  add rax, 1
  mov [rax], cl
  add rbx, 1
  add rax, 1
  jmp __kInitx64EndPrint

__kInitx64EndPrintEnd:
  ; Disable interrupt and loop forever
  cli
  hlt
  jmp __kInitx64EndPrintEnd

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .data

; Number of booted CPUs
_bootedCPUCount:
  dd 0x00000000

_kinitEndOfLine:
  db "END OF LINE"
  db 0

; EOF