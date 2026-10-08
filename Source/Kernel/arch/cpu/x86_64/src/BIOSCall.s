;-------------------------------------------------------------------------------
;
; File: CPUConteBIOSCallxt.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 29/06/2026
;
; Version: 1.0
;
; BIOS Call functions for the x86_64 architecture.
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
%define CODERM 0x0000
%define DATARM 0x0000
%define CODE64 0x0008
%define DATA64 0x0010
%define CODE32 0x0018
%define DATA32 0x0020
%define CODE16 0x0028
%define DATA16 0x0030

%define BIOS_CALL_DATA_SIZE 0x1000

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
; None

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global CPUBIOSCall

;-------------------------------------------------------------------------------
; EXPORTED DATA
;-------------------------------------------------------------------------------
; None

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------
section .bios_call_code
align 4

;-------------------------------------------------------------------------------
; Bios Call function
;
; Param:
;     Input: RDI: The address of the register array used for the call
;            RSI: The interrupt number to raise
;            RDX: The buffer to receive produced data
;            RCX: The size for the buffer to receive produced data
;            R8: The initial data location pointer
CPUBIOSCall:
  ; Save interrupt state and disable interrupts
  pushfq
  cli

  ; Set 16 Bits stack
  mov r9, rsp
  mov rsp, _biosCallStackTop
  sub rsp, 0x8

  ; Save old stack
  push r9
  push rbp
  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  mov  r9, ds
  push r9
  mov  r9, es
  push r9
  mov  r9, ss
  push r9
  mov  r9, fs
  push r9
  mov  r9, gs
  push r9

  ; Save the arguments
  mov [_biosCallArray], rdi
  mov [_dataBuffer], rdx

  cmp rcx, BIOS_CALL_DATA_SIZE
  jbe _BiosCallSizeOk
  mov rcx, BIOS_CALL_DATA_SIZE

_BiosCallSizeOk:
  mov [_dataBufferSize], rcx

  ; Save the current IDT and GDT
  sgdt [_savedGDTPointer]
  sidt [_savedIDTPointer]

  ; Patch the code to set the interrupt number
  mov [_CPUBiosCallIssue], si

  ; Save the registers for the call
  mov rsi, rdi
  mov rdi, _biosCallRegisters
  mov rcx, 8
  rep movsb
  mov rax, [rdi]

  ; Load the 16bit Real Mode GDT
  lgdt [_gdt16Ptr]
  lea rax, [_CPUBiosCall32BitsGdtSetup]
  push CODE32
  push rax
  retfq

_CPUBiosCall32BitsGdtSetup:
[bits 32]
  mov eax, DATA32
  mov ds, eax
  mov es, eax
  mov ss, eax
  mov fs, eax
  mov gs, eax

  ; Disable paging
  mov eax, cr0
  and eax, 0x7FFEFFFF
  mov cr0, eax

  ; Jump to 16 bits
  jmp word CODE16:_CPUBiosCall16BitsGdtSetup

_CPUBiosCall16BitsGdtSetup:
[bits 16]
  mov ax, DATA16
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  ; Load IVT
  lidt [_idt16Ptr]

  ; Disable protected mode
  mov eax, cr0
  and al,  ~0x01
  mov cr0, eax

  ; Jump to real mode
  jmp word CODERM:_CPUBiosCallRealMode

_CPUBiosCallRealMode:
  mov ax, DATARM
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  ; Set destination register for data
  lea ax, [_savedDataBuffer]
  mov di, ax

  ; Get the registers
  mov ax, [_biosCallRegisters]
  mov bx, [_biosCallRegisters + 2]
  mov cx, [_biosCallRegisters + 4]
  mov dx, [_biosCallRegisters + 6]

  db 0xCD  ; Interrupt OPCODE
_CPUBiosCallIssue:
  db 0x00  ; Placeholder for the interrupt number
  nop
  nop
  nop
  nop

  ; Save the registers
  mov [_biosCallRegisters], ax
  mov [_biosCallRegisters + 2], bx
  mov [_biosCallRegisters + 4], cx
  mov [_biosCallRegisters + 6], dx

  ; Enable protected mode
  mov  eax, cr0
  or   eax, 0x00000001
  mov  cr0, eax

  ; Jump to protected mode
  jmp dword CODE32:_CPUBiosCallProtecteMode

_CPUBiosCallProtecteMode:
[bits 32]
  mov eax, DATA32
  mov ds, eax
  mov es, eax
  mov ss, eax
  mov fs, eax
  mov gs, eax

  ; Enable paging
  mov eax, cr0
  or  eax, 0x80010000
  mov cr0, eax

  ; Jump to long mode
  jmp dword CODE64:_CPUBiosCallLongMode

_CPUBiosCallLongMode:
[bits 64]
  mov rax, DATA64
  mov ds, ax
  mov es, ax
  mov ss, ax
  mov fs, ax
  mov gs, ax

  ; Restore the initial Stack, IDT and GDT
  lgdt [_savedGDTPointer]
  lidt [_savedIDTPointer]

  ; Restore the registers and the stack

  pop r9
  mov gs, r9
  pop r9
  mov fs, r9
  pop r9
  mov ss, r9
  pop r9
  mov es, r9
  pop r9
  mov ds, r9
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax
  pop rbp
  pop rsp

  ; Tranfer the registers values
  mov rdi, [_biosCallArray]
  mov ax, [_biosCallRegisters]
  mov [rdi], ax
  mov ax, [_biosCallRegisters + 2]
  mov [rdi + 2], ax
  mov ax, [_biosCallRegisters + 4]
  mov [rdi + 4], ax
  mov ax, [_biosCallRegisters + 6]
  mov [rdi + 6], ax

  ; Transfer the data produced by the call
  mov rdi, [_dataBuffer]
  mov rsi, _savedDataBuffer
  mov rcx, [_dataBufferSize]
  rep movsb

  ; Set the initial data location pointer
  lea rax, [_savedDataBuffer]
  mov [r8], rax

  ; Restore the interrupt state
  popfq
  ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
section .bios_call_data
align 16
; Temporary GDT for Bios Calls
_gdt16:
    .null:
        dd 0x00000000
        dd 0x00000000
    .code_64:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A
        db 0xAF
        db 0x00
    .data_64:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x93
        db 0xCF
        db 0x00
    .code32:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9B
        db 0xCF
        db 0x00
    .data32:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x93
        db 0xCF
        db 0x00
    .code16:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9B
        db 0x0F
        db 0x00
    .data16:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x93
        db 0x0F
        db 0x00
_gdt16Ptr:                                 ; GDT pointer for 16bit access
    dw _gdt16Ptr - _gdt16 - 1              ; GDT limit
    dd _gdt16                              ; GDT base address
    dd 0x00000000

align 16
; BIOS IVT pointer
_idt16Ptr:
    dw 0x03FF                              ; IVT limit
    dd 0x00000000                          ; IVT base address

align 16
_biosCallStack:
  times 0x100 db 0x00
_biosCallStackTop:

_dataBuffer:
  dd 0x00000000
  dd 0x00000000

_dataBufferSize:
  dd 0x00000000
  dd 0x00000000

_biosCallArray:
  dd 0x00000000
  dd 0x00000000

_biosCallRegisters:
  dd 0x00000000
  dd 0x00000000

_savedDataBuffer:
  times BIOS_CALL_DATA_SIZE db 0x00

_savedIDTPointer:
  dd 0x00000000
  dd 0x00000000
  dd 0x00000000

_savedGDTPointer:
  dd 0x00000000
  dd 0x00000000
  dd 0x00000000

; EOF