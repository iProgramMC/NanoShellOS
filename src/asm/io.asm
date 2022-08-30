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
global ReadPortL
global WritePortL
global WriteFont8px
global WriteFont16px

extern e_placement
extern e_frameBitsetVirt

; PORT I/O
ReadPort:	
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	in   al,  dx
	
	mov  esp, ebp
	pop  ebp
	ret
WritePort:
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	mov  al,  [ebp + 12]
	out  dx,  al
	
	mov  esp, ebp
	pop  ebp
	ret
ReadPortW:
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	in   ax,  dx
	
	mov  esp, ebp
	pop  ebp
	ret
WritePortW:
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	mov  ax,  [ebp + 12]
	out  dx,  ax
	
	mov  esp, ebp
	pop  ebp
	ret
ReadPortL:
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	in   eax, dx
	
	mov  esp, ebp
	pop  ebp
	ret
WritePortL:
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	mov  eax, [ebp + 12]
	out  dx,  eax
	
	mov  esp, ebp
	pop  ebp
	ret
	
	
; Functions for DRIVERS/VGA.H
; https://wiki.osdev.org/VGA_Fonts#Get_from_VGA_RAM_directly
WriteFont8px:
	push ebp
	mov  ebp, esp
	
	mov esi, [ebp + 8]
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
	
	mov esp, ebp
	pop ebp
	ret
	
WriteFont16px:
	push ebp
	mov  ebp, esp
	
	mov esi, [ebp + 8]
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
	
	mov esp, ebp
	pop ebp
	ret

global KeIdtLoad
; requires a phys address.
KeIdtLoad:
	push ebp
	mov  ebp, esp
	
	mov  edx, [ebp + 8]
	lidt [edx]
	
	mov  esp, ebp
	pop  ebp
	
	sti
	ret

extern _kernel_end
global MmStartupStuff
MmStartupStuff:
; WORK: Change this if necessary.  Paging is not setup at this stage
;       so this address is purely PHYSICAL.

; TODO: Compilation seems to include stuff like CODE_SEG, DATA_SEG, VMWARE_MAGIC
;       etc after our bss, which may not work.
; Use this weird math to move the placement, in order to be page aligned, as the
; system loves it that way.

	mov ecx, (_kernel_end - 0xC0000000 + 0x1000)
	
	; couldn't do the and itself during compilation :(
	and ecx, 0xFFFFF000
	mov dword [e_placement], ecx
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
	PUSH EBX
	
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
	
	POP EBX
	
	RET


extern WindowCall
extern LogMsg
extern UserCallStuffNotSupportedC
; Window manager call stuff
global UserCallStuff
global UserCallStuffEnd
UserCallStuff:
	MOV ECX, [0xC0007CFC]
	SHL ECX, 2
	
	MOV EAX, [WindowCall+ECX]
	JMP EAX
	
	RET
UserCallStuffEnd:

global UserCallStuffNotSupported
global UserCallStuffNotSupportedEnd
UserCallStuffNotSupported:
	MOV EAX, UserCallStuffNotSupportedC
	JMP [EAX]
UserCallStuffNotSupportedEnd:

; To facilitate 64-bit math (thanks https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/i386/udivdi3.S ) :
; TODO: Re-factor this later :^)
global __udivdi3 ; Unsigned division of two 64-bit integers
__udivdi3:
	push    ebx
	mov     ebx, [esp+14h]
	bsr     ecx, ebx
	jz      short loc_8000087
	mov     eax, [esp+10h]
	shr     eax, cl
	shr     eax, 1
	not     ecx
	shl     ebx, cl
	or      ebx, eax
	mov     edx, [esp+0Ch]
	mov     eax, [esp+8]
	cmp     edx, ebx
	jnb     short loc_8000052
	div     ebx
	push    edi
	not     ecx
	shr     eax, 1
	shr     eax, cl
	mov     edi, eax
	mul     dword [esp+14h]
	mov     ebx, [esp+0Ch]
	mov     ecx, [esp+10h]
	sub     ebx, eax
	sbb     ecx, edx
	mov     eax, [esp+18h]
	imul    eax, edi
	sub     ecx, eax
	sbb     edi, 0
	xor     edx, edx
	mov     eax, edi
	pop     edi
	pop     ebx
	retn
; ---------------------------------------------------------------------------

loc_8000052:                            ; CODE XREF: .text:08000022↑j
	sub     edx, ebx
	div     ebx
	push    edi
	not     ecx
	shr     eax, 1
	or      eax, 80000000h
	shr     eax, cl
	mov     edi, eax
	mul     dword [esp+14h]
	mov     ebx, [esp+0Ch]
	mov     ecx, [esp+10h]
	sub     ebx, eax
	sbb     ecx, edx
	mov     eax, [esp+18h]
	imul    eax, edi
	sub     ecx, eax
	sbb     edi, 0
	xor     edx, edx
	mov     eax, edi
	pop     edi
	pop     ebx
	retn
; ---------------------------------------------------------------------------

loc_8000087:                            ; CODE XREF: .text:08000008↑j
	mov     eax, [esp+0Ch]
	mov     ecx, [esp+10h]
	xor     edx, edx
	div     ecx
	mov     ebx, eax
	mov     eax, [esp+8]
	div     ecx
	mov     edx, ebx
	pop     ebx
	retn

global __divdi3
__divdi3:
                push    esi
                mov     edx, [esp+14h]
                mov     eax, [esp+10h]
                mov     ecx, edx
                sar     ecx, 1Fh
                xor     eax, ecx
                xor     edx, ecx
                sub     eax, ecx
                sbb     edx, ecx
                mov     [esp+14h], edx
                mov     [esp+10h], eax
                mov     esi, ecx
                mov     edx, [esp+0Ch]
                mov     eax, [esp+8]
                mov     ecx, edx
                sar     ecx, 1Fh
                xor     eax, ecx
                xor     edx, ecx
                sub     eax, ecx
                sbb     edx, ecx
                mov     [esp+0Ch], edx
                mov     [esp+8], eax
                xor     esi, ecx
                push    ebx
                mov     ebx, [esp+18h]
                bsr     ecx, ebx
                jz      loc_80000DC
                mov     eax, [esp+14h]
                shr     eax, cl
                shr     eax, 1
                not     ecx
                shl     ebx, cl
                or      ebx, eax
                mov     edx, [esp+10h]
                mov     eax, [esp+0Ch]
                cmp     edx, ebx
                jnb     short loc_800009E
                div     ebx
                push    edi
                not     ecx
                shr     eax, 1
                shr     eax, cl
                mov     edi, eax
                mul     dword [esp+18h]
                mov     ebx, [esp+10h]
                mov     ecx, [esp+14h]
                sub     ebx, eax
                sbb     ecx, edx
                mov     eax, [esp+1Ch]
                imul    eax, edi
                sub     ecx, eax
                sbb     edi, 0
                xor     edx, edx
                mov     eax, edi
                add     eax, esi
                adc     edx, esi
                xor     eax, esi
                xor     edx, esi
                pop     edi
                pop     ebx
                pop     esi
                retn
; ---------------------------------------------------------------------------

loc_800009E:                            ; CODE XREF: .text:08000065↑j
                sub     edx, ebx
                div     ebx
                push    edi
                not     ecx
                shr     eax, 1
                or      eax, 80000000h
                shr     eax, cl
                mov     edi, eax
                mul     dword [esp+18h]
                mov     ebx, [esp+10h]
                mov     ecx, [esp+14h]
                sub     ebx, eax
                sbb     ecx, edx
                mov     eax, [esp+1Ch]
                imul    eax, edi
                sub     ecx, eax
                sbb     edi, 0
                xor     edx, edx
                mov     eax, edi
                add     eax, esi
                adc     edx, esi
                xor     eax, esi
                xor     edx, esi
                pop     edi
                pop     ebx
                pop     esi
                retn
; ---------------------------------------------------------------------------

loc_80000DC:                            ; CODE XREF: .text:08000047↑j
                mov     eax, [esp+10h]
                mov     ecx, [esp+14h]
                xor     edx, edx
                div     ecx
                mov     ebx, eax
                mov     eax, [esp+0Ch]
                div     ecx
                mov     edx, ebx
                add     eax, esi
                adc     edx, esi
                xor     eax, esi
                xor     edx, esi
                pop     ebx
                pop     esi
                retn
;

; https://github.com/jezze/subc
global SetJump
global LongJump
SetJump:
	mov  edx, [esp + 4]
	mov  [edx], esp
	add  dword [edx], 4
	mov  [edx + 4], ebp
	mov  eax, [esp]
	mov  [edx + 8], eax
	xor  eax, eax
	retn
	
LongJump:
	mov  eax, [esp + 8]
	or   eax, eax
	jnz  .crap
	inc  eax
.crap:
	mov  edx, [esp + 4]
	mov  esp, [edx]
	mov  ebp, [edx + 4]
	mov  edx, [edx + 8]
	jmp  edx

; uint32 KeGetEFlags()

global KeGetEFlags
KeGetEFlags:
	pushf                  ; Push the eflags register
	pop eax                ; Pop it into EAX
	ret

global KeGetEIP
KeGetEIP:
	mov eax, [esp]
	ret

global KeGetEBP
KeGetEBP:
	mov eax, ebp
	ret

section .bss

; eax=0, eax's value:
g_cpuidLastLeaf resd 1

; eax=0, the processor name (GenuineIntel, AuthenticAMD etc):
g_cpuidNameEBX resd 1
g_cpuidNameEDX resd 1
g_cpuidNameECX resd 1
g_cpuidNameNUL resd 1

g_cpuidBrandingInfo resd 13

; eax=1, eax's value:
g_cpuidFeatureBits resd 1
