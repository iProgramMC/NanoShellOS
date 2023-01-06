BITS 32
SECTION .data

EBPBackup DD 1
ESPBackup DD 1

SECTION .text

EXTERN NsMain
GLOBAL _NsStart
_NsStart:
	mov  [EBPBackup], ebp
	mov  [ESPBackup], esp
	push ebp
	mov  ebp, esp
	call NsMain
	mov  esp, [ESPBackup]
	mov  ebp, [EBPBackup]
	ret

; ****************************************************************************
;   NanoShell Imports
; ****************************************************************************
;  This is kind of re-inventing the dllimport wheel a bit. Since we have been
;  treating the  NanoShell kernel as a  kind  of library,  by using a LUT and
;  calling directly into parts of the code, we might  as well make that clean
;  and let the kernel resolve the functions itself.
;
;  How does this work? It's very simple:
;  The  NanoShell  imports section will  look through  the .nanoshell_imports
;  section. For each MAKE_IMPORT entry NanoShell either replaces  the special
;  NANOSHELL_MAGIC value with an actual proper call, exits if the  first byte
;  is 0xFF, or throws up and rejects our executable.
; ****************************************************************************
SECTION .nanoshell_imports

NANOSHELL_MAGIC equ 0x6853614E ; 'NaSh'

%macro MAKE_IMPORT 1
GLOBAL %1
%defstr ___%[%1] %1
%1:
	; Define the jmp and the address. This will be replaced by NanoShell's ELF loader.
	; This is 7 bytes for the stub, encoded as "B8 4E 61 53 68 FF E0 <FunctionName> 00".
	; We do not want to perform a long jmp with CS change since I don't really want
	; to assume CS' contents. (they're usually 0x08)
	mov eax, NANOSHELL_MAGIC
	jmp eax
	; The function's name as a null-terminated string.
	db ___%[%1],0
%endmacro

%macro MAKE_IMPORT_END 0
	db 0xFF
%endmacro

MAKE_IMPORT LogString
MAKE_IMPORT GetVersionNumber
MAKE_IMPORT FictionalImport
MAKE_IMPORT_END
