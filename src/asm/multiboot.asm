;
; 		NanoShell Operating System
;      (C) 2021-2022 iProgramInCpp
; 
;         Multiboot Header module
;
; main multiboot header

%define vbe

section .multibootdata
	;multiboot V1 spec
	align 4
	dd 0x1BADB002
	%ifdef vbe
		dd 0x07
		dd - (0x1BADB002 + 0x07)
	%else
		dd 0x03
		dd - (0x1BADB002 + 0x03)
	%endif
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
%ifdef vbe
	dd 0
	dd 1024
	dd 768
	dd 32
%else
	dd 0
	dd 0
	dd 0
	dd 0
%endif