;
; 		NanoShell Operating System
; 		  (C) 2021 iProgramInCpp
; 
;        EarlyBird Startup module
;
bits 32

; functions to load from other c/asm files
extern KiStartupSystem

extern g_KernelPageDirectory
extern e_temporary1
extern e_temporary2

section .text
global KeHigherHalfEntry
KeHigherHalfEntry:
	; Unmap the identity mapping, we don't need it anymore
	mov dword [g_KernelPageDirectory+0], 0
	mov dword [g_KernelPageDirectory+4], 0
	
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
	
	; Before entering the high level kernel, setup SSE.
	; This way, we can benefit from the performance improvements of SSE.
	call KiSetupStuff
	
	; Enter the high-level kernel now.
	; Please note that it's marked __attribute__((noreturn)), so 
	; returning from this crashes the OS.
	call KiStartupSystem
	
	cli
.stop:
	hlt
	jmp .stop
	
KiSetupStuff: ; enables FPU, SSE and Write Protect honoring in ring 0

	clts ;clear task switched flag
	
%ifndef DONT_ENABLE_FPU
	; load cr0
	mov  eax, cr0
	; check for the x87 FPU unit
	test eax, (1 << 2)
	; no FPU? :pleading_face:
	jnz  .noFpuOrFinish
	
	; enable:
	; - the MP bit (monitor co-processor), and
	; - the WP bit (honor read-only restrictions in ring 0)
	or   eax, (1 << 1) | (1 << 16)
	; push the changes
	mov  cr0, eax
	
	; load cr4
	mov  eax, cr4
	
	; enable:
	; - OSFXSR (enable SSE instructions and fast FPU save and restore), and
	; - OSXMMEXCEPT (enable unmasked SSE exceptions)
	or   eax, (1 << 9) | (1 << 10)
	
	; push our changes
	mov  cr4, eax
	
	; initialize the FPU
	fninit
%endif

.noFpuOrFinish:
	ret
	

extern KeHandleSsFailureC
KeHandleSsFailure:
	cli
	call KeHandleSsFailureC
.mht:
	hlt
	jmp .mht

extern g_EmergencyStack

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
GDTUserCode:;planned?
	dw 0xffff
	dw 0x00
	db 0x00
	db 0x9a
	db 0xcf
	db 0x00
GDTUserData:;planned?
	dw 0xffff
	dw 0x00
	db 0x00
	db 0x92
	db 0xcf
	db 0x00
GDT_TSS_StackSegFault:
	dd 0x00000000            ;Link
	dd g_EmergencyStack      ;ESP0
	dd 0x00000010            ; SS0
	dd g_EmergencyStack      ;ESP1
	dd 0x00000010            ; SS1
	dd g_EmergencyStack      ;ESP2
	dd 0x00000010            ; SS2
	dd g_KernelPageDirectory ; CR3
	dd KeHandleSsFailure     ; EIP
	dd 0x00000000            ; EFLAGS.  To be filled in by system later
	dd 0x12345678            ; EAX
	dd 0x12345678            ; EBX
	dd 0x12345678            ; ECX
	dd 0x12345678            ; EDX
	dd g_EmergencyStack      ; ESP
	dd 0x00000000            ; EBP
	dd 0x00000000            ; ESI
	dd 0x00000000            ; EDI
	dd 0x00000010            ; ES
	dd 0x00000008            ; CS
	dd 0x00000010            ; SS
	dd 0x00000010            ; DS
	dd 0x00000010            ; FS
	dd 0x00000010            ; GS
	dd 0x00000000            ; LDTR
	dd 0x00000000            ; IOPB
	dd g_EmergencyStack      ; SSP
	
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
	resb 131072 ; 128KB for stack
g_stackSpace:



