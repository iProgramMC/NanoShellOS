;
; 		NanoShell Operating System
; 		  (C) 2021 iProgramInCpp
; 
;    Assembly global variables module
;
section .data
	
global g_kernelPageDirectory
global e_placement
global e_frameBitsetSize
global e_frameBitsetVirt
global g_pageTableArray
global e_temporary1
global e_temporary2

; WORK: Change this if necessary.  Paging is not setup at this stage
;       so this address is purely PHYSICAL.
e_placement dd 0x400000

section .bss

	align 4096
g_kernelPageDirectory:
	resd 1024 ; reserve 1024 Page Directory Entries.
	
	; When the PG bit is set in CR0, any and all reads and writes from the CPU 
	; the addresses get turned into linear addresses via the TLB and MMU.
	;
	; The structure of a virtual address (on 32-Bit) is as follows:
	; [31        -       22] [21     -      12] [11            -           0]
	; [Page directory index] [Page table index] [Address inside the 4KB page]
	;
	; The page directory array's address is stored in CR3, along with some flags.
	
g_pageTableArray:
	resd 8192
	
e_temporary1 resd 1
e_temporary2 resd 1
e_frameBitsetVirt resd 1
e_frameBitsetSize resd 1