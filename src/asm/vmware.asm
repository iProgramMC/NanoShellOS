BITS 32
extern VmwAbsCursorIrq
global VmwCommandInput
global VmwCommandOutput
global VmwASend
global VmwASendHb
global VmwAGetHb
global VmwAbsCursorIrqA

section .data
VmwCommandInput:  dd 4
VmwCommandOutput: dd 6

VMWARE_MAGIC  equ 0x564D5868
VMWARE_PORT   equ 0x00005658
VMWARE_PORTHB equ 0x00005659



section .text
VmwASend:
; boilerplate
	push ebp
	mov  ebp, esp
	
	push esi
	push edi
	push ebx
	
; the real code
	mov eax, VMWARE_MAGIC
	mov ebx, [VmwCommandInput + 0]
	mov ecx, [VmwCommandInput + 4]
	mov edx, VMWARE_PORT
	mov esi, [VmwCommandInput + 8]
	mov edi, [VmwCommandInput + 12]
	
	in  eax, dx
	
	mov [VmwCommandOutput +  0], eax
	mov [VmwCommandOutput +  4], ebx
	mov [VmwCommandOutput +  8], ecx
	mov [VmwCommandOutput + 12], edx
	mov [VmwCommandOutput + 16], esi
	mov [VmwCommandOutput + 20], edi
	
; more boilerplate
	pop  ebx
	pop  edi
	pop  esi
	
	mov  esp, ebp
	pop  ebp
	ret

VmwASendHb:
; boilerplate
	push ebp
	mov  ebp, esp
	
	push esi
	push edi
	push ebx
	
; the real code
	mov eax, VMWARE_MAGIC
	mov ebx, [VmwCommandInput + 0]
	mov ecx, [VmwCommandInput + 4]
	mov edx, VMWARE_PORTHB
	mov esi, [VmwCommandInput + 8]
	mov edi, [VmwCommandInput + 12]
	
	rep outsb
	
	mov [VmwCommandOutput +  0], eax
	mov [VmwCommandOutput +  4], ebx
	mov [VmwCommandOutput +  8], ecx
	mov [VmwCommandOutput + 12], edx
	mov [VmwCommandOutput + 16], esi
	mov [VmwCommandOutput + 20], edi
	
; more boilerplate
	pop  ebx
	pop  edi
	pop  esi
	
	mov  esp, ebp
	pop  ebp
	ret


VmwAGetHb:
; boilerplate
	push ebp
	mov  ebp, esp
	
	push esi
	push edi
	push ebx
	
; the real code
	mov eax, VMWARE_MAGIC
	mov ebx, [VmwCommandInput + 0]
	mov ecx, [VmwCommandInput + 4]
	mov edx, VMWARE_PORTHB
	mov esi, [VmwCommandInput + 8]
	mov edi, [VmwCommandInput + 12]
	
	rep insb
	
	mov [VmwCommandOutput +  0], eax
	mov [VmwCommandOutput +  4], ebx
	mov [VmwCommandOutput +  8], ecx
	mov [VmwCommandOutput + 12], edx
	mov [VmwCommandOutput + 16], esi
	mov [VmwCommandOutput + 20], edi
	
; more boilerplate
	pop  ebx
	pop  edi
	pop  esi
	
	mov  esp, ebp
	pop  ebp
	ret

VmwAbsCursorIrqA:
	pusha
	call VmwAbsCursorIrq
	popa
	iretd