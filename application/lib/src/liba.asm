bits 32

section .text

global DoSyscall
DoSyscall:
	mov esi, [esp+0x04]
	mov eax, [esp+0x08]
	mov ebx, [esp+0x0C]
	mov ecx, [esp+0x10]
	mov edx, [esp+0x14]
	
	int 0x80
	
	ret
	