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
	call SbIrqHandler
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
	call UartOnInterrupt
	add  esp, 4
	popa
	iretd
IrqSerialCom2A:
	pusha
	push 1
	call UartOnInterrupt
	add  esp, 4
	popa
	iretd
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
	; Allow interrupts to come in again. Nested interrupts are supported by the CPU
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
	mov  eax, cr2
	push eax
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
	mov  eax, cr2
	push eax
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