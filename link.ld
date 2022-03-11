/* Tell the linker that we want an ELF32 output file */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

/* Linker script for the OS */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

ENTRY (KeEntry)

/* Here is where all of the sections of the kernel are defined. */
SECTIONS
{
	/* Begin loading at 0x100000, as that's where GRUB will place our data. */
	. = 1M;
	
	_kernel_start = .;

	.multibootdata :
	{
		*(.multibootdata)
	}

	.multiboottext :
	{
		*(.multiboottext)
	}
	
	. += 0xC0000000;
	
	.text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
	{
		*(.text)
	}
	
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	
	.data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
	{
		*(.data)
	}
	
	.bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
	}
	
	. = 7M;
	
	.section : 
	{
	
	}

	/* Add a symbol that indicates the end address of the kernel. */
	_kernel_end = .;
}