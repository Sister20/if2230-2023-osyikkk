#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/interrupt.h"
#include "lib-header/keyboard.h"
#include "lib-header/disk.h"
#include "lib-header/fat32.h"

void kernel_setup(void) {
    // uint32_t a;
    // uint32_t volatile b = 0x0000BABE;
    // __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    enter_protected_mode(&_gdt_gdtr);   
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    // framebuffer_write(3, 8, 'H', 0, 0xF);
    // framebuffer_write(3, 9, 'a', 0, 0xF);
    // framebuffer_write(3, 10, 'i', 0, 0xF);
    // framebuffer_write(3, 11, '!', 0, 0xF);
    framebuffer_set_cursor(0, 0);
    framebuffer_write_string("> ");
    // while (TRUE) b += 1;
    __asm__("int $0x4");
    while (TRUE) {
        keyboard_state_activate();
    }

}