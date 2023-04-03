ENTRY(loader)                /* the name of the entry label */

SECTIONS {
    . = 0xC0100000;          /* use relocation address (memory references) at 0xC010 0000 */

    /* Optional variable that can be used in kernel, starting address of kernel */
    _linker_kernel_virtual_addr_start  = .; 
    _linker_kernel_physical_addr_start = . - 0xC0000000;

    .__mbHeader ALIGN (0x1000) : AT (ADDR (.__mbHeader) - 0xC0000000)
    {
        *(.__mbHeader)       /* put GRUB multiboot header at front */
    }

    .setup.text ALIGN (0x1000) : AT (ADDR (.setup.text) - 0xC0000000)
    {
        *(.setup.text)       /* initial setup code */
    }

    .text ALIGN (0x1000) : AT (ADDR (.text) - 0xC0000000)
    {
        *(.text)             /* all text sections from all files */
    }

    .rodata ALIGN (0x1000) : AT (ADDR (.rodata) - 0xC0000000)
    {
        *(.rodata*)          /* all read-only data sections from all files */
    }

    .data ALIGN (0x1000) : AT (ADDR (.data) - 0xC0000000)
    {
        *(.data)             /* all data sections from all files */
    }

    .bss ALIGN (0x1000) : AT (ADDR (.bss) - 0xC0000000)
    {
        *(COMMON)            /* all COMMON sections from all files */
        *(.bss)              /* all bss sections from all files */
        kernel_loader.o(.bss)
        _linker_kernel_stack_top = .;
    }
    /* Optional variable that can be used in kernel, show end address of kernel */
    _linker_kernel_virtual_addr_end  = .;
    _linker_kernel_physical_addr_end = . - 0xC0000000;
}
