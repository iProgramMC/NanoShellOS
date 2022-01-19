;
; 		NanoShell Operating System
; 		  (C) 2021 iProgramInCpp
; 
;        EarlyBird Startup module
;
bits 32

; Setup kernel page directory and subsequent pages.  Maps the entire memory map from 0x00000000 to BASE_ADDRESS.
%define BASE_ADDRESS 0xC0000000
%define VIRT_TO_PHYS(k) ((k) - BASE_ADDRESS)
	
; variables to load from other c/asm files
extern g_kernelPageDirectory
extern g_pageTableArray
extern e_placement
extern e_frameBitsetSize
extern e_frameBitsetVirt
extern e_temporary1
extern e_temporary2

; functions to load from other c/asm files
extern KiStartupSystem

; things that aren't static and exclusive to this object file 
global KeEntry

section .multiboottext

KeEntry:
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
	
	; Map the one and only pagetable (TODO) to both virtual addresses 0x0 and 0xC0000000
	mov dword [VIRT_TO_PHYS(g_kernelPageDirectory) +   0*4], VIRT_TO_PHYS(g_pageTableArray+0000) + 0x03
	mov dword [VIRT_TO_PHYS(g_kernelPageDirectory) +   1*4], VIRT_TO_PHYS(g_pageTableArray+4096) + 0x03
	mov dword [VIRT_TO_PHYS(g_kernelPageDirectory) + 768*4], VIRT_TO_PHYS(g_pageTableArray+0000) + 0x03
	mov dword [VIRT_TO_PHYS(g_kernelPageDirectory) + 769*4], VIRT_TO_PHYS(g_pageTableArray+4096) + 0x03
	
	; Set CR3 to the physical address of the g_kernelPageDirectory
	mov ecx, VIRT_TO_PHYS(g_kernelPageDirectory)
	mov cr3, ecx
	
	; Set PG and WP bit
	mov ecx, cr0
	or  ecx, 0x80010000
	mov cr0, ecx
	
	; Jump to higher half:
	mov ecx, (KeHigherHalfEntry)
	jmp ecx
	
section .text
KeHigherHalfEntry:
	; Unmap the identity mapping, we don't need it anymore
	mov dword [g_kernelPageDirectory], 0
	
	; Reload CR3 to force a TLB flush (we updated the PDT but TLB isn't aware of that)
	; NOTE: you can probably also use invlpg.  We won't use that
	mov ecx, cr3
	mov cr3, ecx
	
	; Set up the stack
	mov esp, g_stackSpace
	
	jmp SetupGDT
GDTPostSetup:
	
	; Restore the multiboot data we got earlier
	mov eax, [e_temporary1]
	mov ebx, [e_temporary2]
	
	push ebx
	push eax
	
	; Enter the high-level kernel now.
	; Please note that it's marked __attribute__((noreturn)), so 
	; returning from this crashes the OS.
	call KiStartupSystem
	
	cli
.mmmm:
	hlt
	jmp .mmmm


GDTStart:
GDTNull:
	dd 0x0
	dd 0x0
GDTCode:
	dw 0xffff
	dw 0x00
	db 0x00
	db 0x9a
	db 0xcf
	db 0x00
GDTData:
	dw 0xffff
	dw 0x00
	db 0x00
	db 0x92
	db 0xcf
	db 0x00
GDTEnd:
GDTDescription:
	dw GDTEnd - GDTStart - 1
	dd GDTStart
CODE_SEG equ GDTCode - GDTStart
DATA_SEG equ GDTData - GDTStart

SetupGDT:
	lgdt [GDTDescription]
	jmp CODE_SEG:.set_cs
.set_cs:
	mov eax, DATA_SEG
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov ss, eax
	jmp GDTPostSetup
	
section .bss
	resb 65536 ; 64KB for stack
g_stackSpace:



