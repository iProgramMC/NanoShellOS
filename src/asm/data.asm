;
; 		NanoShell Operating System
; 		  (C) 2021 iProgramInCpp
; 
;    Assembly global variables module
;

section .data

global e_placement
global e_frameBitsetSize
global e_frameBitsetVirt
global g_pageTableArray
global e_temporary1
global e_temporary2

e_placement dd 0x000000

section .bss

	align 4096
	
g_pageTableArray:
	resd 8192

global g_EmergencyStack
g_EmergencyStack:
	resd 2048 ;8192 bytes should be safe
	
e_temporary1 resd 1
e_temporary2 resd 1
e_frameBitsetVirt resd 1
e_frameBitsetSize resd 1