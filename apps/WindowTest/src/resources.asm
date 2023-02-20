; NanoShell compiled resource file
; Ideally, this would be processed by a program in the NanoShell build system first.

section .nanoshell
; This contains information about the application.

dd 1            ; SubSystem
				; 0 - Console application
                ; 1 - Windowed application
				; *Other values are reserved.
dd 0x01010000   ; Version Number (MM.mm.bbbb)

; These are all pointers to strings
dd AppName      ; Name
dd AppAuth      ; Author
dd AppCopr      ; Copyright infomation
dd ProName      ; Project Name

AppName: db "Chart demo application", 0
AppAuth: db "iProgramInCpp", 0
AppCopr: db "Copyright (C) 2019-2023 iProgramInCpp. Licensed under the GNU GPLv3.", 0
ProName: db "The NanoShell(TM) Operating System", 0

section .nanoshell_resources
; This section contains the resources used by NanoShell.

dd 2   ; # of resources

dd 100 ; id
dd 1   ; type
dd 22  ; size
db "Hello, there! It's ok", 0

dd 101 ; id
dd 1   ; type
dd 11  ; size
dd "Resources!", 0


