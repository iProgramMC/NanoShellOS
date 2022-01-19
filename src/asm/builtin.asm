;
; 		NanoShell Operating System
; 		  (C) 2021 iProgramInCpp
; 
;        Built-in features module
;
%define enable 1

%ifdef enable

section .data
global g_testingElfStart, g_testingElfEnd, g_testingElf

g_testingElfStart:
g_testingElf:
	incbin 'application/helloworld/main.nse'
g_testingElfEnd:

global g_initrdStart, g_initrd, g_initrdEnd

g_initrdStart:
g_initrd:
incbin 'nanoshell_initrd'
g_initrdEnd:

%endif