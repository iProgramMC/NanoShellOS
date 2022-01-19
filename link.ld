/* Linker script for the OS */
ENTRY (KeEntry)

/* Here is where all of the sections of the kernel are defined. */
SECTIONS
{
	/* Begin loading at 0x100000, as that's where GRUB will place our data. */
	. = 1M;
	
	_kernel_start = .;
	.multibootdata : {
		*(.multibootdata)
	}
	.multiboottext : {
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
 
	/* The compiler may produce other sections, by default it will put them in
	   a segment with the same name. Simply add stuff here as needed. */
	   
	
	/* Add a symbol that indicates the end address of the kernel. */
	_kernel_end = .;
}