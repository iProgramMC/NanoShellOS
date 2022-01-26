/* Linker script for an application for the OS. */

/* Entry point for application */
ENTRY (main)

SECTIONS 
{
	. = 12M; /*Load at 12M, until we get paging working*/
	
	/*text section*/
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
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