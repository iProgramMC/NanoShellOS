;
; 		NanoShell Operating System
;      (C) 2021-2022 iProgramInCpp
; 
;       Task Scheduling ASM Module
;
BITS 32

;section .data
;global e_TaskStack
;e_TaskStack res 1024

section .text

extern KeSwitchTask
global IrqTaskA
global IrqTaskB
IrqTaskA:
	cli
	push esp
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
	
	mov eax, cr3
	push eax
	
	; acknowledge the interrupt
	mov al, 0x20
	mov dx, 0x20
	out dx, al
	mov dx, 0xA0
	out dx, al
	
	push esp
	; call the re-schedule function
	call KeSwitchTask
	add esp, 4 ; get rid of what we had on the stack
	
	pop eax
	mov cr3, eax
	
	pop eax
	pop ebx
	pop ecx
	pop edx
	pop esi
	pop edi
	pop ebp
	pop esp
	
	; There is actually no need to call "sti", iretd 
	; reloads the EFLAGS, which automatically does that.
	iretd
IrqTaskB:
	cli
	push esp
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
	
	mov eax, cr3
	push eax
	
	; since this is a _software_ interrupt do not acknowledge it
	
	push esp
	; call the re-schedule function
	call KeSwitchTask
	add esp, 4 ; get rid of what we had on the stack
	
	pop eax
	mov cr3, eax
	
	pop eax
	pop ebx
	pop ecx
	pop edx
	pop esi
	pop edi
	pop ebp
	pop esp
	
	; There is actually no need to call "sti", iretd 
	; reloads the EFLAGS, which automatically does that.
	iretd
	
extern g_saveStateToRestore1
global KeStartedNewTask
global KeStartedNewKernelTask
KeStartedNewTask:
KeStartedNewKernelTask:
	mov esp, [g_saveStateToRestore1]
	
	pop eax
	mov cr3, eax
	
	pop eax
	pop ebx
	pop ecx
	pop edx
	pop esi
	pop edi
	pop ebp
	pop esp
	
	iretd

extern KeTaskStartupC
global KeTaskStartup
KeTaskStartup:
	; Sets up the task, calls its function, and then
	; kills it.  The task's pointer is in eax.
	
	push eax
	call KeTaskStartupC
	
	; freeze if we got to this point!?
.halt:
	hlt
	jmp .halt