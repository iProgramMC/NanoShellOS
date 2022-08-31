;
; 		NanoShell Operating System
;      (C) 2021-2022 iProgramInCpp
; 
;        Interrupt handler module
;
BITS 32
SECTION .text


extern IrqTimer
extern IrqClock
extern IrqMouse
extern IrqKeyboard
extern IrqVirtualBox
extern OnSyscallReceived
extern IsrSoftware
extern UartOnInterrupt
extern SbIrqHandler

extern KeOnEnterInterrupt
extern KeOnExitInterrupt

global IrqTimerA
global IrqClockA
global IrqMouseA
global IrqSb16A
global IrqVirtualBoxA
global IrqKeyboardA
global IrqCascadeA
global IrqSerialCom1A
global IrqSerialCom2A
global OnSyscallReceivedA

section .text

IrqSb16A:
	pusha
	call KeOnEnterInterrupt
	call SbIrqHandler
	call KeOnExitInterrupt
	popa
	iretd

IrqVirtualBoxA:
	;pusha
	;call IrqVirtualBox
	;popa
	iretd

IrqSerialCom1A:
	pusha
	push 0
	call KeOnEnterInterrupt
	call UartOnInterrupt
	call KeOnExitInterrupt
	add  esp, 4
	popa
	iretd

IrqSerialCom2A:
	pusha
	push 1
	call KeOnEnterInterrupt
	call UartOnInterrupt
	call KeOnExitInterrupt
	add  esp, 4
	popa
	iretd

IrqTimerA:
	pusha
	call KeOnEnterInterrupt
	call IrqTimer
	call KeOnExitInterrupt
	popa
	iretd

IrqKeyboardA:
	pusha
	push esp
	call KeOnEnterInterrupt
	call IrqKeyboard
	call KeOnExitInterrupt
	add esp, 4
	popa
	iretd

IrqClockA:
	pusha
	call KeOnEnterInterrupt
	call IrqClock
	call KeOnExitInterrupt
	popa
	iretd

IrqMouseA:
	pusha
	call KeOnEnterInterrupt
	call IrqMouse
	call KeOnExitInterrupt
	popa
	iretd

IrqCascadeA:
	iretd

OnSyscallReceivedA:
	; Allow interrupts to come in again. Nested interrupts are supported by the CPU
	push 0
	pusha
	
	; call isrSoftware with our one and only parm:
	push esp
	call KeOnEnterInterrupt
	call OnSyscallReceived
	call KeOnExitInterrupt
	add esp, 4
	
	popa
	add esp, 4 ;remove the zero from the stack
	
	iretd
	
extern IsrExceptionCommon
%macro ExceptionNoErrorCode 1
global IsrStub%+%1
IsrStub%+%1:
	push 0
	pusha
	mov  eax, cr2
	push eax
	push esp
	push %1
	call KeOnEnterInterrupt
	call IsrExceptionCommon
.mht:
	hlt
	jmp .mht
	; This function shouldn't return
extern IsrExceptionCommon
%endmacro

%macro ExceptionErrorCode 1
global IsrStub%+%1
IsrStub%+%1:
	pusha
	mov  eax, cr2
	push eax
	push esp
	push %1
	call KeOnEnterInterrupt
	call IsrExceptionCommon
.mht:
	hlt
	jmp .mht
	; This function shouldn't return
%endmacro

; Special handler for interrupt 0xE (page fault)
global IsrStub14
extern MmOnPageFault
IsrStub14:
	; the error code has already been pushed
	pusha                         ; back up all registers
	mov eax, cr2                  ; push cr2 (the faulting address), to complete the 'registers' struct
	push eax
	push esp                      ; push esp - a pointer to the registers* struct
	call KeOnEnterInterrupt       ; tell us we entered some interrupt
	call MmOnPageFault            ; call the page fault handler
	call KeOnExitInterrupt        ; tell us we exited that interrupt
	add  esp, 8                   ; pop away esp and cr2
	popa                          ; restore the registers, then return from the page fault
	add  esp, 4                   ; pop away the error code
	iretd

ExceptionNoErrorCode 0
ExceptionNoErrorCode 1
ExceptionNoErrorCode 2
ExceptionNoErrorCode 3
ExceptionNoErrorCode 4
ExceptionNoErrorCode 5
ExceptionNoErrorCode 6
ExceptionNoErrorCode 7
ExceptionErrorCode   8
ExceptionNoErrorCode 9
ExceptionErrorCode   10
ExceptionErrorCode   11
ExceptionErrorCode   12
ExceptionErrorCode   13
ExceptionNoErrorCode 15
ExceptionNoErrorCode 16
ExceptionErrorCode   17
ExceptionNoErrorCode 18
ExceptionNoErrorCode 19
ExceptionNoErrorCode 20
ExceptionNoErrorCode 21
ExceptionNoErrorCode 22
ExceptionNoErrorCode 23
ExceptionNoErrorCode 24
ExceptionNoErrorCode 25
ExceptionNoErrorCode 26
ExceptionNoErrorCode 27
ExceptionNoErrorCode 28
ExceptionNoErrorCode 29
ExceptionErrorCode   30
ExceptionNoErrorCode 31
