; NanoShell Operating System (C) 2022 iProgramInCpp
; Raycaster application
; Some math functions I couldn't implement in C directly

; Stolen from musl: https://git.musl-libc.org/cgit/musl/tree/src/math/i386/atan2f.s?id=v1.1.24
global atan2f
global atan2l

atan2f:
	fld  dword [esp + 4]
	fld  dword [esp + 8]
	fpatan
	fst  dword [esp + 4]
	mov  eax, [esp + 4]
	add  eax, eax
	cmp  eax, 0x01000000
	jae .1f
	; subnormal x, return x with underflow
	fld  ST0
	fmul ST1
	fstp dword [esp + 4]
.1f:
	ret
	
atan2l:
	fld qword [esp + 4]
	fld qword [esp + 16]
	fpatan
	ret