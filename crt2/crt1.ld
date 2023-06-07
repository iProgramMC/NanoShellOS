/**
 * This linker script is purely to let the linker know
 * that we're trying to get an elf32-i386 object out of
 * crt1.
 */
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)