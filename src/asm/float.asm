;
; 		NanoShell Operating System
; 		  (C) 2022 iProgramInCpp
; 
;     Floating Point Math ASM module
;
bits 32

global round
round:
	fld 	dword 	[esp + 4]
	
	frndint
	
	fistp 	dword 	[esp + 4]
	mov 	eax, 	[esp + 4]
	
	ret
global floor
floor:
	fld 	dword 	[esp + 4]
		
	fisttp 	dword 	[esp + 4]
	mov 	eax,  	[esp + 4]
	
	ret
	
;TODO
global floorf
floorf:
	;fld 	dword 	[esp + 4]
	;fisttp 	dword 	[esp + 4]
	;fild 	dword 	[esp + 4]
	;fstp 	dword 	[esp + 4]
	;mov 	eax, 	[esp + 4]
	ret
global roundf
roundf:
	push ebp
	mov  ebp, esp
	sub  esp, 4
	
	fld 	dword 	[ebp + 8]
	frndint
	fstp	dword	[ebp - 4]
	fld 	dword	[ebp - 4]
	
	leave
	ret
	