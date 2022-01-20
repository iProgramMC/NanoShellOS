;
; 		NanoShell Operating System
; 		  (C) 2022 iProgramInCpp
; 
;     Floating Point Math ASM module
;
bits 32

global round
round:
	push ebp
	mov  ebp, esp
	
	fld 	dword 	[ebp + 8]
	
	frndint
	
	fistp 	dword 	[ebp + 8]
	mov 	eax, 	[ebp + 8]
	
	mov  esp, ebp
	pop  ebp
	ret
global floor
floor:
	push ebp
	mov  ebp, esp
	
	fld 	dword 	[ebp + 8]
		
	fisttp 	dword 	[ebp + 8]
	mov 	eax,  	[ebp + 8]
	
	mov  esp, ebp
	pop  ebp
	ret
	
;TODO
global floorf
floorf:
	push ebp
	mov  ebp, esp
	
	;fld 	dword 	[ebp + 8]
	;fisttp 	dword 	[ebp + 8]
	;fild 	dword 	[ebp + 8]
	;fstp 	dword 	[ebp + 8]
	;mov 	eax, 	[ebp + 8]
	mov  esp, ebp
	pop  ebp
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
	