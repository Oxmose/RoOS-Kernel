;-------------------------------------------------------------------------------
;
; File: InitAp.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 07/06/2024
;
; Version: 1.0
;
; Kernel entry point for secondary cores. The CPU are switched to protected mode
; then long mode and the existing kernel page table is setup.
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; INCLUDES
;-------------------------------------------------------------------------------
%include "config.inc"

;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 16]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------
%define CODE32 0x08
%define DATA32 0x10
%define CODE64 0x28
%define DATA64 0x30

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
extern _KERNEL_STACKS_BASE
extern _kernelPGDir
extern _bootedCPUCount
extern _fcw

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern CPUAPInit

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .low_ap_startup_code
;-------------------------------------------------------------------------------
; Kernel entry AP point
[bits 16]
__kinitApCores:
  cli

  ; Set GDT
  lgdt [_gdt16Ptr]

  ; Set PE bit in cr0
  mov eax, cr0
  or  eax, 0x1
  mov cr0, eax

  ; Jump to protected mode
  jmp dword CODE32:__kinitApPM

[bits 32]
__kinitApPM:
  ; Set Segments
  mov ax, DATA32
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; Init FPU
  fninit
  fldcw [_fcw]

  ; Enable SSE
  mov eax, cr0
  and al, ~0x04
  or  al, 0x22
  mov cr0, eax
  mov eax, cr4
  or  ax, 0x600
  mov cr4, eax

  ; Enable PAE and PGE
  mov eax, cr4
  or  eax, 0xA0
  mov cr4, eax

  ; Switch to compatibility mode, enable NXE and SYSCALL/SYSRET
  mov ecx, 0xC0000080
  rdmsr
  or  eax, 0x00000901
  wrmsr

  ; Set CR3
  mov eax, (_kernelPGDir - KERNEL_MEM_OFFSET)
  mov cr3, eax

  ; Enable paging and caches
  mov eax, cr0
  and eax, 0x0FFFFFFF
  or  eax, 0x80010000
  mov cr0, eax

  ; Far jump to 64 bit mode
  jmp CODE64:__kinitApPM64bEntry

[bits 64]
__kinitApPM64bEntry:
  ; Set Segments
  mov ax, DATA64
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; Get our CPU id based on the booted CPU count and update booted CPU count
  xor rcx, rcx
  mov ecx, [_bootedCPUCount]
  mov rax, rcx
  add rax, 1
  mov [_bootedCPUCount], rax

  ; Set the stack base on the CPU id
  mov rbx, KERNEL_STACK_SIZE
  mul rbx
  mov rbx, _KERNEL_STACKS_BASE
  add rax, rbx
  sub rax, 16
  mov rsp, rax
  mov rbp, rsp

  ; Write the core ID into PID
  xor rdx, rdx
  mov rax, rcx
  mov rcx, 0xC0000103
  wrmsr

  ; rax contains the CPU ID, set as first parameter
  mov  rdi, rax
  mov  rax, CPUAPInit
  call rax

; We should never return
__kinitAp64PMEnd:
  cli
  hlt
  jmp __kinitAp64PMEnd

align 32
; Temporary GDT for AP
_gdt16:
  .null:
    dd 0x00000000
    dd 0x00000000
  .code_32:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9B
    db 0xCF
    db 0x00
  .data_32:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x93
    db 0xCF
    db 0x00
  .code_16:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0x0F
    db 0x00
  .data_16:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0x0F
    db 0x00
  .code_64:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9B
    db 0xAF
    db 0x00
  .data_64:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x93
    db 0xCF
    db 0x00

_gdt16Ptr:                                 ; GDT pointer for 16bit access
  dw _gdt16Ptr - _gdt16 - 1              ; GDT limit
  dd _gdt16                              ; GDT base address

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .data
