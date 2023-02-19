;
; 		NanoShell Operating System
;      (C) 2021-2023 iProgramInCpp
; 
;         Initial program loader
;

; The goal of the initial program loader is to set up a basic page table and stack for us.

; Setup kernel page directory and subsequent pages.  Maps the entire memory map from 0x00000000 to BASE_ADDRESS.
%define BASE_ADDRESS 0xC0000000
%define VIRT_TO_PHYS(k) ((k) - BASE_ADDRESS)
	
; variables to load from other c/asm files
extern g_KernelPageDirectory
extern g_pageTableArray
extern e_placement
extern e_frameBitsetSize
extern e_frameBitsetVirt
extern e_temporary1
extern e_temporary2

section .ipldata
	;multiboot V1 spec
	align 4
	dd 0x1BADB002
	dd 0x07                  ; flags
	dd - (0x1BADB002 + 0x07) ; checksum
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0
	dd 1024  ; default resolution
	dd 768
	dd 32

section .ipltext
global KeIPLEntry
extern KeHigherHalfEntry
KeIPLEntry:
	cli					; Block all interrupts.  We don't have them setup yet
	
	; We don't actually need a stack at this stage
	xor ebp, ebp
	
	mov [VIRT_TO_PHYS(e_temporary1)], eax	; Make a backup of the multiboot parameters.
	mov [VIRT_TO_PHYS(e_temporary2)], ebx
	
	; First address to map is 0x00000000
	xor esi, esi

	; Map 8192 pages.
	mov ecx, 8192
	
	; Get physical address of the boot page table.
	mov edi, VIRT_TO_PHYS (g_pageTableArray)
	
.label1:
	mov edx, esi
	or  edx, 0x03 ; Set present and R/W bits
	mov [edi], edx
	
	add esi, 0x1000 ; The size of a page is 4096 bytes
	add edi, 4
	;inc dword [VIRT_TO_PHYS(e_pageTableNum)]
	loop .label1
	
	; NOTE: If the kernel goes haywire just change PagesToMap
.label3:
	
	; Map the two pagetables required to both virtual addresses 0x0 and 0xC0000000
	mov dword [VIRT_TO_PHYS(g_KernelPageDirectory) +   0*4], VIRT_TO_PHYS(g_pageTableArray+0000) + 0x03
	mov dword [VIRT_TO_PHYS(g_KernelPageDirectory) +   1*4], VIRT_TO_PHYS(g_pageTableArray+4096) + 0x03
	mov dword [VIRT_TO_PHYS(g_KernelPageDirectory) + 768*4], VIRT_TO_PHYS(g_pageTableArray+0000) + 0x03
	mov dword [VIRT_TO_PHYS(g_KernelPageDirectory) + 769*4], VIRT_TO_PHYS(g_pageTableArray+4096) + 0x03
	
	; Set CR3 to the physical address of the g_KernelPageDirectory
	mov ecx, VIRT_TO_PHYS(g_KernelPageDirectory)
	mov cr3, ecx
	
	; Set PG and WP bit
	mov ecx, cr0
	or  ecx, 0x80010000
	mov cr0, ecx
	
	; Jump to higher half:
	mov ecx, (KeHigherHalfEntry)
	jmp ecx
