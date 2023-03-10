;
;       NanoShell Operating System
;       (C)2021-2022 iProgramInCpp
;
;     Fast MemoryCopy/Set Operations
;

; TODO: This is a mess, please go and clean this up later kthx :^)

global fast_memcpy
global fast_memset
global not_fast_memcpy
global memset_ints
global memset_shorts
global memcpy_ints
global align4_memcpy
global align8_memcpy
global align16_memcpy
global memcpy_16_byte_aligned
global memcpy_128_byte_aligned

section .text.othermemcpies align=4096
; Not Fast
not_fast_memcpy:
	push ebp
	mov  ebp, esp
	
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	rep movsb
	
	pop edi
	pop esi
	
	mov esp, ebp
	pop ebp
	ret
	
fast_memset:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
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
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret


fast_memcpy:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	push ebp
	;prefetchnta [esi]
	.some_loop:
		;prefetchnta [esi+32]
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
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
	
memset_ints:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	cmp ecx, 0
	je  .done
	
	.some_loop:
		mov [edi], esi
		add edi, 4
		dec ecx
		jnz .some_loop
	
.done:
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
; it may be worth slowing down there (bucko) if you are filling
; out an array of shorts instead of 32bit ints for whatever reason
memset_shorts:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	cmp ecx, 0
	je  .done
	
	.some_loop:
		mov [edi], si
		add edi, 2
		dec ecx
		jnz .some_loop
	
.done:
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
	
section .text.memcpies align=4096
; WARNING: Needs count and source/dest to be 16-byte aligned. Else will cause a GPF.
memcpy_16_byte_aligned:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	shr ecx, 4 ; divide by 16 (i.e. multiply the number of 16-byte blocks you want to copy by 16 when you call this) 
	
	.loop_cpy:
		;prefetchnta [esi + 16]
		
		movdqa xmm0, [esi + 0]
		movntdq [edi + 0]  , xmm0
		
		add esi, 16
		add edi, 16
		dec ecx
		jnz .loop_cpy
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
	
memcpy_ints:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	push ebp
	;prefetchnta [esi]
	.some_loop:
		;prefetchnta [esi+4]
		mov eax, [esi]
		mov [edi   ], eax
		add esi, 4
		add edi, 4
		dec ecx
		jnz .some_loop
		
	; done!
	pop ebp
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret

section .text.alignmemcpies align=4096
align4_memcpy:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	; An alternative to try. TODO test this out on real hardware, see how it performs.
	; Do this likewise on other platforms as well.
	;   shr ecx, 2
	;   rep movsd
	
	push ebp
	;prefetchnta [esi]
	.some_loop:
		;prefetchnta [esi+4]
		mov eax, [esi]
		mov [edi   ], eax
		add esi, 4
		add edi, 4
		sub ecx, 4
		jnz .some_loop
		
	; done!
	pop ebp
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
align8_memcpy:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov esi, [ebp + 0Ch]
	mov edi, [ebp + 08h]
	mov ecx, [ebp + 10h]
	
	push ebp
	;prefetchnta [esi]
	.some_loop:
		;prefetchnta [esi+8]
		mov eax, [esi]
		mov ebx, [esi+4]
		mov [edi  ], eax
		mov [edi+4], ebx
		add esi, 8
		add edi, 8
		sub ecx, 8
		jnz .some_loop
		
	; done!
	pop ebp
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
align16_memcpy:
	push ebp
	mov  ebp, esp
	
	push ebx
	push esi
	push edi
	
	mov edi, [ebp + 08h]
	mov esi, [ebp + 0Ch]
	mov ecx, [ebp + 10h]
	
	push ebp
	;prefetchnta [esi]
	.some_loop:
		;prefetchnta [esi+16]
		mov eax, [esi]
		mov ebx, [esi+4]
		mov edx, [esi+8]
		mov ebp, [esi+12]
		mov [edi   ], eax
		mov [edi+4 ], ebx
		mov [edi+8 ], edx
		mov [edi+12], ebp
		add esi, 16
		add edi, 16
		sub ecx, 16
		jnz .some_loop
		
	; done!
	pop ebp
	
	pop edi
	pop esi
	pop ebx
	
	mov esp, ebp
	pop ebp
	ret
