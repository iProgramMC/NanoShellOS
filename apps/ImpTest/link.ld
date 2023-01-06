/* Tell the linker that we want an ELF32 output file */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

/* Linker script for an application for the OS. */

/* Entry point for application */
ENTRY (_NsStart)

SECTIONS 
{
	. = 12M; /*Load at 12M, until we get paging working*/
	
	/*text section*/
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}
	
	/*import section*/
	.nanoshell_imports BLOCK(4K) : ALIGN(4K)
	{
		*(.nanoshell_imports)
	}
	
	/*rodata section - read-only variables*/
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
	
	/*data section - variables with initialized data*/
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
	
	/*bss sections - */
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
}