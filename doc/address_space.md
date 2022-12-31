## NanoShell Address Space Specification

Valid as of NanoShell Version 1.00

#### The higher half

* `0x80000000 - 0xC0000000` - The kernel heap. In practice, only up to `0x90000000` is used.
* `0xC0000000 - 0xC0800000` - The first 8 MiB of physical memory. This happens to include the kernel.
* `0xD0000000 - 0xFFFFFFFF` - MMIO area. In practice, right now, only the frame buffer is mapped in this region, at `0xE0000000`

#### The lower half

* `0x00C00000 - ...` - The program data of a typical NanoShell executable.
* `0x40000000 - 0x80000000` - The user heap. This is the area that `malloc` and `free` typically use.


