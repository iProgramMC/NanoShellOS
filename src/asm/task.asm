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

helloText:
	db "Lapic switch", 0

extern KeSwitchTask
extern SLogMsg
extern g_pLapic
global IrqTaskPitA
global IrqTaskLapicA
global IrqTaskSoftA
global IrqSpuriousA
extern ApicEoi
IrqSpuriousA:
	push eax
	push edx
	mov dword [g_pLapic + 0xB0], 0x00
	
	; acknowledge the interrupt the PIC way
	mov al, 0x20
	mov dx, 0x20
	out dx, al
	mov dx, 0xA0
	out dx, al
	
	pop edx
	pop eax
	iretd
IrqTaskLapicA:
	cli
	; Preserve basic registers
	push esp
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
	
	; Preserve page table
	mov eax, cr3
	push eax
	
	; Preserve segment registers
	mov eax, ds
	push eax
	mov eax, es
	push eax
	mov eax, fs
	push eax
	mov eax, gs
	push eax
	mov eax, ss
	push eax
	
	push esp
	
	; Disable the direction flag, mandatory for C 32-bit SysV ABI if
	; code relies on repeat instructions.
	; The direction flag will be restored to the task, since it's part
	; of the EFLAGS, and it's already saved.
	cld
	
	; acknowledge the interrupt
	mov dword [g_pLapic + 0xB0], 0x00
	
	push helloText
	call SLogMsg
	
	; call the re-schedule function
	call KeSwitchTask
	add esp, 4 ; get rid of what we had on the stack
	
	; Restore the seg registers
	pop eax
	mov ss, eax
	pop eax
	mov gs, eax
	pop eax
	mov fs, eax
	pop eax
	mov es, eax
	pop eax
	mov ds, eax
	
	; Restore page table
	pop eax
	mov cr3, eax
	
	; Restore working registers
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
IrqTaskPitA:
	cli
	; Preserve basic registers
	push esp
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
	
	; Preserve page table
	mov eax, cr3
	push eax
	
	; Preserve segment registers
	mov eax, ds
	push eax
	mov eax, es
	push eax
	mov eax, fs
	push eax
	mov eax, gs
	push eax
	mov eax, ss
	push eax
	
	; acknowledge the interrupt
	mov al, 0x20
	mov dx, 0x20
	out dx, al
	mov dx, 0xA0
	out dx, al
	
	push esp
	
	; Disable the direction flag, mandatory for C 32-bit SysV ABI if
	; code relies on repeat instructions.
	; The direction flag will be restored to the task, since it's part
	; of the EFLAGS, and it's already saved.
	cld
	
	; call the re-schedule function
	call KeSwitchTask
	add esp, 4 ; get rid of what we had on the stack
	
	; Restore the seg registers
	pop eax
	mov ss, eax
	pop eax
	mov gs, eax
	pop eax
	mov fs, eax
	pop eax
	mov es, eax
	pop eax
	mov ds, eax
	
	; Restore page table
	pop eax
	mov cr3, eax
	
	; Restore working registers
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
IrqTaskSoftA:
	cli
	; Preserve basic registers
	push esp
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
	
	; Preserve page table
	mov eax, cr3
	push eax
	
	; Preserve segment registers
	mov eax, ds
	push eax
	mov eax, es
	push eax
	mov eax, fs
	push eax
	mov eax, gs
	push eax
	mov eax, ss
	push eax
	
	push esp
	
	; Disable the direction flag, mandatory for C 32-bit SysV ABI if
	; code relies on repeat instructions.
	; The direction flag will be restored to the task, since it's part
	; of the EFLAGS, and it's already saved.
	cld
	
	; call the re-schedule function
	call KeSwitchTask
	add esp, 4 ; get rid of what we had on the stack
	
	; Restore the seg registers
	pop eax
	mov ss, eax
	pop eax
	mov gs, eax
	pop eax
	mov fs, eax
	pop eax
	mov es, eax
	pop eax
	mov ds, eax
	
	; Restore page table
	pop eax
	mov cr3, eax
	
	; Restore working registers
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
	; Get the save state pointer to restore
	mov esp, [g_saveStateToRestore1]
	
	; Restore the seg registers
	pop eax
	mov ss, eax
	pop eax
	mov gs, eax
	pop eax
	mov fs, eax
	pop eax
	mov es, eax
	pop eax
	mov ds, eax
	
	; Restore page table
	pop eax
	mov cr3, eax
	
	; Restore working registers
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