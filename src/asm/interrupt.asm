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
extern OnSyscallReceived
extern IsrSoftware

global IrqTimerA
global IrqClockA
global IrqMouseA
global IrqKeyboardA
global IrqCascadeA
global OnSyscallReceivedA

section .text

IrqTimerA:
	pusha
	call IrqTimer
	popa
	iretd
IrqKeyboardA:
	pusha
	push esp
	call IrqKeyboard
	add esp, 4
	popa
	iretd
IrqClockA:
	pusha
	call IrqClock
	popa
	iretd
IrqMouseA:
	pusha
	call IrqMouse
	popa
	iretd
IrqCascadeA:
	iretd
OnSyscallReceivedA:
	cli
	push 0
	pusha
	
	; call isrSoftware with our one and only parm:
	push esp
	call OnSyscallReceived
	add esp, 4
	
	popa
	add esp, 4 ;remove the zero from the stack
	sti
	iretd
	
extern IsrExceptionCommon
%macro ExceptionNoErrorCode 1
global IsrStub%+%1
IsrStub%+%1:
	push 0
	pusha
	push esp
	push %1
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
	push esp
	push %1
	call IsrExceptionCommon
.mht:
	hlt
	jmp .mht
	; This function shouldn't return
%endmacro
	
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
ExceptionErrorCode   14
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