; NanoShell C-Runtime entry
; Copyright (C) 2022 iProgramInCpp

BITS 32

SECTION .data

EbpBackup DD 1
EspBackup DD 1

SECTION .text

; C-main function
EXTERN _CEntry
; Internal library cleanup functions
EXTERN _I_CloseOpenWindows
EXTERN _I_FreeEverything
EXTERN _I_CloseOpenFiles
EXTERN _I_Setup

; Force exit function `void exit(int code)`
GLOBAL exit
; Internal entry point
GLOBAL _NsStart

exit:
	push ebp
	mov  ebp, esp
	
	; Call internal cleanup functions
	call _I_CloseOpenFiles
	call _I_FreeEverything
	call _I_CloseOpenWindows
	
	mov  eax, [ebp + 8]
	jmp _NsExitPoint ;Nothing to lose

_NsStart:
	mov  [EspBackup], esp
	mov  [EbpBackup], ebp
	
	push ebp
	mov  ebp, esp
	
	call _I_Setup
	
	mov  eax, [ebp + 8]
	
	push eax
	call _CEntry
	; no return
	
_NsExitPoint:
	mov  esp, [EspBackup]
	mov  ebp, [EbpBackup]
	
	ret  ; return to kernel
	
	
	