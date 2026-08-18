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
%define VCPU_OFF_INT 0x00
%define VCPU_OFF_ERR 0x08
%define VCPU_OFF_RIP 0x10
%define VCPU_OFF_CS  0x18
%define VCPU_OFF_FLG 0x20
%define VCPU_OFF_RSP 0x28
%define VCPU_OFF_RBP 0x30
%define VCPU_OFF_RDI 0x38
%define VCPU_OFF_RSI 0x40
%define VCPU_OFF_RDX 0x48
%define VCPU_OFF_RCX 0x50
%define VCPU_OFF_RBX 0x58
%define VCPU_OFF_RAX 0x60
%define VCPU_OFF_R8  0x68
%define VCPU_OFF_R9  0x70
%define VCPU_OFF_R10 0x78
%define VCPU_OFF_R11 0x80
%define VCPU_OFF_R12 0x88
%define VCPU_OFF_R13 0x90
%define VCPU_OFF_R14 0x98
%define VCPU_OFF_R15 0xA0
%define VCPU_OFF_SS  0xA8
%define VCPU_OFF_GS  0xB0
%define VCPU_OFF_FS  0xB8
%define VCPU_OFF_ES  0xC0
%define VCPU_OFF_DS  0xC8
%define VCPU_OFF_FXD 0xD0

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
; Get the CPU Id
;
; Param:
;     None

CPUGetId:
  push rcx
  push rdx
  ; Get the core id
  mov rcx, 0xC0000103
  rdmsr
  pop rdx
  pop rcx
  ret

;-------------------------------------------------------------------------------
; Save the CPU context of a thread
;
; Param:
;     None
CPUSaveContext:
  ; Save a bit of context
  push rax
  push rbx
  push rdx
  push rcx

  ; Get the current CPU
  xor rax, rax
  call CPUGetId
  pop rcx

  ; Get the offset in the schedule contexts
  mov rbx, 8
  mul rbx

  ; Load the schedule context
  mov rbx, ppSchedulerContext
  mov rbx, [rbx]
  add rax, rbx
  mov rax, [rax]

  ; Load the thread
  mov rax, [rax]

  ; Load the thread vCPU
  mov rax, [rax]

  ; Restore RDX used with mul
  pop rdx

  ; Save the interrupt context
  mov rbx, [rsp + 24]               ; Int id
  mov [rax + VCPU_OFF_INT], rbx
  mov rbx, [rsp + 32]               ; Int code
  mov [rax + VCPU_OFF_ERR], rbx
  mov rbx, [rsp + 40]               ; RIP
  mov [rax + VCPU_OFF_RIP], rbx
  mov rbx, [rsp + 48]               ; CS
  mov [rax + VCPU_OFF_CS], rbx
  mov rbx, [rsp + 56]               ; RFLAGS
  mov [rax + VCPU_OFF_FLG], rbx
  mov rbx, [rsp + 64]               ; RSP
  mov [rax + VCPU_OFF_RSP], rbx
  mov rbx, [rsp + 72]               ; SS
  mov [rax + VCPU_OFF_SS], rbx

  mov [rax + VCPU_OFF_RBP], rbp
  mov [rax + VCPU_OFF_RDI], rdi
  mov [rax + VCPU_OFF_RSI], rsi
  mov [rax + VCPU_OFF_RDX], rdx
  mov [rax + VCPU_OFF_RCX], rcx
  pop rbx                         ; restore prelude rbx
  mov [rax + VCPU_OFF_RBX], rbx
  pop rbx                         ; restore prelude rax
  mov [rax + VCPU_OFF_RAX], rbx

  mov [rax + VCPU_OFF_R8],  r8
  mov [rax + VCPU_OFF_R9],  r9
  mov [rax + VCPU_OFF_R10], r10
  mov [rax + VCPU_OFF_R11], r11
  mov [rax + VCPU_OFF_R12], r12
  mov [rax + VCPU_OFF_R13], r13
  mov [rax + VCPU_OFF_R14], r14
  mov [rax + VCPU_OFF_R15], r15

  mov [rax + VCPU_OFF_GS], gs
  mov [rax + VCPU_OFF_FS], fs
  mov [rax + VCPU_OFF_ES], es
  mov [rax + VCPU_OFF_DS], ds

  ; Save the FxData
  mov rbx, rax
  add rbx, VCPU_OFF_FXD
  add rbx, 0xF                   ; ALIGN Region
  and rbx, 0xFFFFFFFFFFFFFFF0    ; ALIGN Region
  fxsave [rbx]

  ret

;-------------------------------------------------------------------------------
; Restore the CPU context of a thread
;
; Param:
;     RDI - The pointer to the thread to restore
CPURestoreContext:
  ; The current thread is sent as parameter, load the VCPU
  mov rax, [rdi]

  ; Restore the FxData
  mov rbx, rax
  add rbx, VCPU_OFF_FXD
  add rbx, 0xF                   ; ALIGN Region
  and rbx, 0xFFFFFFFFFFFFFFF0    ; ALIGN Region
  fxrstor [rbx]

  ; Restore registers
  mov es, [rax + VCPU_OFF_ES]
  mov ds, [rax + VCPU_OFF_DS]
  mov fs, [rax + VCPU_OFF_FS]
  mov gs, [rax + VCPU_OFF_GS]

  mov r8,  [rax + VCPU_OFF_R8]
  mov r9,  [rax + VCPU_OFF_R9]
  mov r10, [rax + VCPU_OFF_R10]
  mov r11, [rax + VCPU_OFF_R11]
  mov r12, [rax + VCPU_OFF_R12]
  mov r13, [rax + VCPU_OFF_R13]
  mov r14, [rax + VCPU_OFF_R14]
  mov r15, [rax + VCPU_OFF_R15]

  mov rbp, [rax + VCPU_OFF_RBP]
  mov rdi, [rax + VCPU_OFF_RDI]
  mov rsi, [rax + VCPU_OFF_RSI]
  mov rdx, [rax + VCPU_OFF_RDX]
  mov rcx, [rax + VCPU_OFF_RCX]

  ; Use FXData as temporary stack for the return
  mov rsp, [rax + VCPU_OFF_RSP]
  sub rsp, 8

  ; Restore the interrupt context
  mov  rbx, [rax + VCPU_OFF_SS]  ; SS
  push rbx
  mov  rbx, [rax + VCPU_OFF_RSP] ; RSP
  push rbx
  mov  rbx, [rax + VCPU_OFF_FLG] ; RFLAGS
  push rbx
  mov  rbx, [rax + VCPU_OFF_CS]  ; CS
  push rbx
  mov  rbx, [rax + VCPU_OFF_RIP] ; RIP
  push rbx

  ; Restore RBX and RAX
  mov rbx, [rax + VCPU_OFF_RBX]
  mov rax, [rax + VCPU_OFF_RAX]

  ; Return from interrupt
  iretq

;-------------------------------------------------------------------------------
; Save the CPU context of a thread and call the scheduler
;
; Param:
;     RDI - The VCPU of the current thread
CPUSaveContextAndSchedule:
  ; Save RAX
  mov [rdi + VCPU_OFF_RBP], rbp
  mov [rdi + VCPU_OFF_RDI], rdi
  mov [rdi + VCPU_OFF_RSI], rsi
  mov [rdi + VCPU_OFF_RDX], rdx
  mov [rdi + VCPU_OFF_RCX], rcx
  mov [rdi + VCPU_OFF_RBX], rbx
  mov [rdi + VCPU_OFF_RAX], rax

  mov [rdi + VCPU_OFF_R8],  r8
  mov [rdi + VCPU_OFF_R9],  r9
  mov [rdi + VCPU_OFF_R10], r10
  mov [rdi + VCPU_OFF_R11], r11
  mov [rdi + VCPU_OFF_R12], r12
  mov [rdi + VCPU_OFF_R13], r13
  mov [rdi + VCPU_OFF_R14], r14
  mov [rdi + VCPU_OFF_R15], r15

  mov [rdi + VCPU_OFF_GS], gs
  mov [rdi + VCPU_OFF_FS], fs
  mov [rdi + VCPU_OFF_ES], es
  mov [rdi + VCPU_OFF_DS], ds

  ; Save the interrupt context
  mov rax, [rsp]                   ; RIP
  mov [rdi + VCPU_OFF_RIP], rax
  mov rax, cs                       ; CS
  mov [rdi + VCPU_OFF_CS], rax
  pushfq
  pop rax                           ; RFLAGS
  mov [rdi + VCPU_OFF_FLG], rax
  mov rax, rsp                      ; RSP
  add rax, 8
  mov [rdi + VCPU_OFF_RSP], rax
  mov rax, ss                       ; SS
  mov [rdi + VCPU_OFF_SS], rax

  ; Save the FxData
  mov rbx, rax
  add rbx, VCPU_OFF_FXD
  add rbx, 0xF                   ; ALIGN Region
  and rbx, 0xFFFFFFFFFFFFFFF0    ; ALIGN Region
  fxsave [rbx]

  call SchedulerSchedule
  ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
; None

; EOF