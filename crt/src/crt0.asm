; NanoShell C-Runtime basics
; Copyright (C) 2022 iProgramInCpp

BITS 32

SECTION .data

; The way we 'exit' on this is a bit unconventional. We do not have a syscall
; to exit, instead, returning from the entry point actually kills the process/thread.
; This is why we need to backup the EBP and ESP, so that we can return properly in
; case of an exit().
EbpBackup DD 1
EspBackup DD 1

SECTION .text

; C-main function
EXTERN _CEntry
; Internal library cleanup functions
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
	
	mov  eax, [ebp + 8]
	jmp _NsExitPoint ;Nothing to lose

_NsStart:
	mov  [EspBackup], esp
	mov  [EbpBackup], ebp
	
	push ebp
	mov  ebp, esp
	
	mov  eax, [ebp + 8]
	
	push eax
	call _CEntry
	; no return
	
_NsExitPoint:
	mov  esp, [EspBackup]
	mov  ebp, [EbpBackup]
	
	ret  ; return to kernel
	
; Well, this is basically the same thing, but a bit more diverse
; https://github.com/jezze/subc
global SetJump
global setjmp
global LongJump
global longjmp
SetJump:
setjmp:
	mov  edx, [esp + 4]
	mov  [edx],      esp
	add  dword [edx], 4
	mov  [edx + 4],  ebp
	mov  eax, [esp]
	mov  [edx + 8],  eax
	mov  [edx + 12], ebx
	mov  [edx + 16], esi
	mov  [edx + 20], edi
	xor  eax, eax
	retn
	
LongJump:
longjmp:
	mov  eax, [esp + 8]
	or   eax, eax
	; the spec says to return 1 if there's no 'val' passed into longjmp().
	jnz  .increment
	inc  eax
.increment:
	mov  edx, [esp + 4]
	mov  esp, [edx]
	mov  ebp, [edx + 4]
	mov  ebx, [edx + 12]
	mov  esi, [edx + 16]
	mov  edi, [edx + 20]
	mov  edx, [edx + 8]
	jmp  edx
