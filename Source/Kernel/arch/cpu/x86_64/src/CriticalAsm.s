;-------------------------------------------------------------------------------
;
; File: CriticalAsm.s
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
; None

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
global SpinLock
global SpinUnlock
global AtomicIncrement32
global AtomicDecrement32
global AtomicOr32
global AtomicExchange32
global CPUMemoryFenceRelease
global CPUMemoryFenceAcquire

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
; Lock Spinlock
;
; Param:
;     Input: rdi: Address of the lock
SpinLock:
  cmp     dword [rdi], 0
  jne     __pauseSpinlockPause

__spinlockTryAcquire:
  mov         eax, 1
  xchg dword [rdi], eax
  test        eax, eax
  jz          __spinlockAcquired

__pauseSpinlockPause:
  pause
  cmp     dword [rdi], 0
  jne     __pauseSpinlockPause
  jmp     __spinlockTryAcquire

__spinlockAcquired:
  ret

;-------------------------------------------------------------------------------
; Unlock Spinlock
;
; Param:
;     Input: rdi: Address of the lock

SpinUnlock:
  mov  dword [rdi], 0
  ret

;-------------------------------------------------------------------------------
; 32 Bits atomic increment
;
; Param:
;     Input: rdi: T_U32Atomic value to increment
AtomicIncrement32:
  mov  eax, 1
  lock xadd dword [rdi], eax
  ret

;-------------------------------------------------------------------------------
; 32 Bits atomic decrement
;
; Param:
;     Input: rdi: T_U32Atomic value to decrement
AtomicDecrement32:
  mov  eax, -1
  lock xadd dword [rdi], eax
  ret


;-------------------------------------------------------------------------------
; 32 Bits atomic or
;
; Param:
;     Input: rdi: T_U32Atomic value to or
;     Input: rsi: value to OR with
; Return:
;     Output: rax: previous value
AtomicOr32:
  mov  eax, [rdi]
.retry:
  mov  edx, eax
  or   edx, esi
  lock cmpxchg [rdi], edx
  jnz  .retry
  ret

;-------------------------------------------------------------------------------
; 32 Bits atomic exchange
;
; Param:
;     Input: rdi: T_U32Atomic value to exchange
;     Input: rsi: new value to exchange with
; Return:
;     Output: rax: previous value
AtomicExchange32:
  mov  eax, esi
  xchg dword [rdi], eax
  ret

;-------------------------------------------------------------------------------
; CPU Memory acquire fence
;
; Param:
;     None
CPUMemoryFenceAcquire:
  mfence
  ret

;-------------------------------------------------------------------------------
; CPU Memory release fence
;
; Param:
;     None
CPUMemoryFenceRelease:
  mfence
  ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
; None

; EOF