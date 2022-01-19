;
; 		NanoShell Operating System
;      (C) 2021-2022 iProgramInCpp
; 
;        Misc/Input-Output module
;
bits 32
section .text

global ReadPort
global WritePort
global ReadPortW
global WritePortW
global WriteFont8px
global WriteFont16px

extern e_placement
extern e_frameBitsetVirt

; PORT I/O
ReadPort:	
	mov edx, [esp + 4]
	in al, dx
	ret
WritePort:
	mov edx, [esp + 4]
	mov al, [esp + 8]
	out dx, al
	ret
ReadPortW:
	mov edx, [esp + 4]
	in ax, dx
	ret
WritePortW:
	mov edx, [esp + 4]
	mov ax, [esp + 8]
	out dx, ax
	ret
	
	
extern fast_memcpy
extern not_fast_memcpy
; Not Fast
not_fast_memcpy:
	mov esi, [esp + 8]
	mov edi, [esp + 4]
	mov ecx, [esp + 12]
	rep movsb
	ret
	
extern fast_memset
fast_memset:
	mov esi, [esp + 8]
	mov edi, [esp + 4]
	mov ecx, [esp + 12]
	
	and esi, 0xFF
	mov eax, esi
	xor esi, esi
	
	or  esi, eax
	shl eax, 8
	or  esi, eax
	shl eax, 8
	or  esi, eax
	shl eax, 8
	or  esi, eax
	
	.some_loop:
		mov [edi], esi
		add edi, 4
		sub ecx, 4
		jnz .some_loop
	
	ret


fast_memcpy:
	mov esi, [esp + 8]
	mov edi, [esp + 4]
	mov ecx, [esp + 12]
	
	push ebp
	prefetchnta [esi]
	.some_loop:
		prefetchnta [esi+32]
		mov eax, [esi]
		mov ebx, [esi+4]
		mov edx, [esi+8]
		mov ebp, [esi+12]
		mov [edi   ], eax
		mov [edi+4 ], ebx
		mov [edi+8 ], edx
		mov [edi+12], ebp
		mov eax, [esi+16]
		mov ebx, [esi+16+4]
		mov edx, [esi+16+8]
		mov ebp, [esi+16+12]
		mov [edi+16   ], eax
		mov [edi+16+4 ], ebx
		mov [edi+16+8 ], edx
		mov [edi+16+12], ebp
		add esi, 32
		add edi, 32
		sub ecx, 32
		jnz .some_loop
		
	; done!
	pop ebp
	ret
	

; Functions for DRIVERS/VGA.H
; https://wiki.osdev.org/VGA_Fonts#Get_from_VGA_RAM_directly
WriteFont8px:
	mov esi, [esp + 4]
	mov edi, 0xC00A0000
	;in: edi=4k buffer
	;out: buffer filled with font
	;clear even/odd mode
	mov	dx, 03ceh
	mov	ax, 5
	out	dx, ax
	;map VGA memory to 0A0000h
	mov	ax, 0406h
	out	dx, ax
	;set bitplane 2
	mov	dx, 03c4h
	mov	ax, 0402h
	out	dx, ax
	;clear even/odd mode (the other way, don't ask why)
	mov	ax, 0604h
	out	dx, ax
	;copy charmap
	mov	ecx, 256
	;copy 8 bytes to bitmap
.b:	
	movsd
	movsd
	;skip another 24 bytes
	add	edi, 24
	loop .b
	;restore VGA state to normal operation
	mov	ax, 0302h
	out	dx, ax
	mov	ax, 0204h
	out	dx, ax
	mov	dx, 03ceh
	mov	ax, 1005h
	out	dx, ax
	mov	ax, 0E06h
	out	dx, ax
	ret
	
WriteFont16px:
	mov esi, [esp + 4]
	mov edi, 0xC00A0000
	;in: edi=4k buffer
	;out: buffer filled with font
	;clear even/odd mode
	mov	dx, 03ceh
	mov	ax, 5
	out	dx, ax
	;map VGA memory to 0A0000h
	mov	ax, 0406h
	out	dx, ax
	;set bitplane 2
	mov	dx, 03c4h
	mov	ax, 0402h
	out	dx, ax
	;clear even/odd mode (the other way, don't ask why)
	mov	ax, 0604h
	out	dx, ax
	;copy charmap
	mov	ecx, 256
	;copy 16 bytes to bitmap
.b:	movsd
	movsd
    movsd
	movsd
	;skip another 16 bytes
	add	edi, 16
	loop .b
	;restore VGA state to normal operation
	mov	ax, 0302h
	out	dx, ax
	mov	ax, 0204h
	out	dx, ax
	mov	dx, 03ceh
	mov	ax, 1005h
	out	dx, ax
	mov	ax, 0E06h
	out	dx, ax
	ret

global KeIdtLoad
; requires a phys address.
KeIdtLoad:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret


global MmStartupStuff
MmStartupStuff:
	mov ecx, dword [e_placement]
	add ecx, 0xC0000000
	mov dword [e_frameBitsetVirt], ecx
	ret
	
global g_cpuidLastLeaf
global g_cpuidNameEBX
global g_cpuidNameECX
global g_cpuidNameEDX
global g_cpuidNameNUL
global g_cpuidFeatureBits
global g_cpuidBrandingInfo
	
global KeCPUID
KeCPUID:
	MOV EAX, 0 ; First leaf of CPUID
	
	CPUID
	
	MOV [g_cpuidLastLeaf], EAX
	MOV [g_cpuidNameEBX],  EBX
	MOV [g_cpuidNameEDX],  EDX
	MOV [g_cpuidNameECX],  ECX
	MOV DWORD [g_cpuidNameNUL], 0x0
	
	MOV EAX, 1 ; Second leaf of CPUID
	CPUID
	MOV [g_cpuidFeatureBits], EAX
	
	; get processor brand string
	
	MOV EAX, 80000002h
	CPUID
	MOV [g_cpuidBrandingInfo+ 0], EAX
	MOV [g_cpuidBrandingInfo+ 4], EBX
	MOV [g_cpuidBrandingInfo+ 8], ECX
	MOV [g_cpuidBrandingInfo+12], EDX
	
	MOV EAX, 80000003h
	CPUID
	MOV [g_cpuidBrandingInfo+16], EAX
	MOV [g_cpuidBrandingInfo+20], EBX
	MOV [g_cpuidBrandingInfo+24], ECX
	MOV [g_cpuidBrandingInfo+28], EDX
	
	MOV EAX, 80000004h
	CPUID
	MOV [g_cpuidBrandingInfo+32], EAX
	MOV [g_cpuidBrandingInfo+36], EBX
	MOV [g_cpuidBrandingInfo+40], ECX
	MOV [g_cpuidBrandingInfo+44], EDX
	
	XOR EAX, EAX
	MOV [g_cpuidBrandingInfo+48], AL
	
	RET


extern WindowCall
extern LogMsg
extern UserCallStuffNotSupportedC
; Window manager call stuff
global UserCallStuff
global UserCallStuffEnd
UserCallStuff:
	MOV EBX, [0xC0007CFC]
	SHL EBX, 2
	
	MOV EAX, [WindowCall+EBX]
	JMP EAX
	
	RET
UserCallStuffEnd:

global UserCallStuffNotSupported
global UserCallStuffNotSupportedEnd
UserCallStuffNotSupported:
	MOV EAX, UserCallStuffNotSupportedC
	JMP [EAX]
UserCallStuffNotSupportedEnd:
	
section .bss

; eax=0, eax's value:
g_cpuidLastLeaf resd 1

; eax=0, the processor name (GenuineIntel, AuthenticAMD etc):
g_cpuidNameEBX resd 1
g_cpuidNameEDX resd 1
g_cpuidNameECX resd 1
g_cpuidNameNUL resd 1

g_cpuidBrandingInfo resd 49

; eax=1, eax's value:
g_cpuidFeatureBits resd 1




	