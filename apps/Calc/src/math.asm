; NanoShell Operating System (C) 2022 iProgramInCpp
; Calculator Application
; Math functions required for compilation
BITS 32
section .text

; To facilitate 64-bit math (thanks https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/i386/udivdi3.S ) :
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
				