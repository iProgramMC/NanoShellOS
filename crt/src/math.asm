; NanoShell C-Runtime math
; Copyright (C) 2022 iProgramInCpp

; To facilitate 64-bit math (thanks https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/i386/udivdi3.S ) :
; TODO: Re-factor this later :^)
global __divdi3  ; Signed division of two 64-bit ints
global __moddi3  ; Signed modulo of two 64-bit integers
global __umoddi3 ; Unsigned modulo of two 64-bit integers
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
loc_8000052:
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
loc_8000087:
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
	
loc_800009E:
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

loc_80000DC:
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

__umoddi3:
	push    ebx
	mov     ebx, [esp+14h]
	bsr     ecx, ebx
	jz      loc_8000099
	mov     eax, [esp+10h]
	shr     eax, cl
	shr     eax, 1
	not     ecx
	shl     ebx, cl
	or      ebx, eax
	mov     edx, [esp+0Ch]
	mov     eax, [esp+8]
	cmp     edx, ebx
	jnb     short loc_800005D
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
	jnb     short loc_8000056
	add     ebx, [esp+14h]
	adc     ecx, [esp+18h]
loc_8000056:
	mov     eax, ebx
	mov     edx, ecx
	pop     edi
	pop     ebx
	retn
loc_800005D:
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
	jnb     short loc_8000092
	add     ebx, [esp+14h]
	adc     ecx, [esp+18h]
loc_8000092:
	mov     eax, ebx
	mov     edx, ecx
	pop     edi
	pop     ebx
	retn
loc_8000099:
	mov     eax, [esp+0Ch]
	mov     ecx, [esp+10h]
	xor     edx, edx
	div     ecx
	mov     ebx, eax
	mov     eax, [esp+8]
	div     ecx
	mov     eax, edx
	pop     ebx
	xor     edx, edx
	retn
	
	
__moddi3:
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
	mov     esi, ecx
	push    ebx
	mov     ebx, [esp+18h]
	bsr     ecx, ebx
	jz      __moddi3_9
	mov     eax, [esp+14h]
	shr     eax, cl
	shr     eax, 1
	not     ecx
	shl     ebx, cl
	or      ebx, eax
	mov     edx, [esp+10h]
	mov     eax, [esp+0Ch]
	cmp     edx, ebx
	jnb     short __moddi3_2
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
	jnb     short __moddi3_1
	add     ebx, [esp+18h]
	adc     ecx, [esp+1Ch]
__moddi3_1:
	mov     eax, ebx
	mov     edx, ecx
	add     eax, esi
	adc     edx, esi
	xor     eax, esi
	xor     edx, esi
	pop     edi
	pop     ebx
	pop     esi
	retn
__moddi3_2:
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
	jnb     short __moddi3_3
	add     ebx, [esp+18h]
	adc     ecx, [esp+1Ch]
__moddi3_3:
	mov     eax, ebx
	mov     edx, ecx
	add     eax, esi
	adc     edx, esi
	xor     eax, esi
	xor     edx, esi
	pop     edi
	pop     ebx
	pop     esi
	retn
__moddi3_9:
	mov     eax, [esp+10h]
	mov     ecx, [esp+14h]
	xor     edx, edx
	div     ecx
	mov     ebx, eax
	mov     eax, [esp+0Ch]
	div     ecx
	mov     eax, edx
	pop     ebx
	xor     edx, edx
	add     eax, esi
	adc     edx, esi
	xor     eax, esi
	xor     edx, esi
	pop     esi
	retn

global atan2l
atan2l:
	fld qword [esp + 4]
	fld qword [esp + 16]
	fpatan
	ret