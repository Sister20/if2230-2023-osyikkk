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
#include "lib-header/paging.h"

// TODO : inserter masih segmentation fault

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);   
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    framebuffer_write_string("> ");

    activate_keyboard_interrupt();
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();
    
    // Allocate first 4 MiB virtual memory
    allocate_single_user_page_frame((uint8_t*) 0);



    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
        .buf                   = cbuf,
        .name                  = "ikanaide",
        .ext                   = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;
    struct FAT32DriverRequest request_dir = {
        .buf                   = cbuf,
        .name                  = "nbuna1\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    write(request);  // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    write(request);  // Create folder "kano1"
    memcpy(request.name, "ikanaide", 8);
    memcpy(request.ext, "\0\0\0", 3);
    delete(request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    memcpy(request.ext, "owo", 3);
    request.buffer_size = 5*CLUSTER_SIZE;
    write(request);  // Create fragmented file "daijoubu"
    // delete(request);

    struct ClusterBuffer readcbuf;
    read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // If read properly, readcbuf should filled with 'a'

    request.buffer_size = 0;
    read(request);
    request.buffer_size = CLUSTER_SIZE;
    read(request);   // Failed read due not enough buffer size
    request.buffer_size = 5*CLUSTER_SIZE;
    read(request);   // Success read on file "daijoubu"
    memcpy(request.name, "test\0\0\0\0", 8);
    read(request);
    memcpy(request.name, "\0\0\0\0\0\0\0\0", 8);
    read(request);
    memcpy(request.name, "daijoubu", 8);
    // delete(request);

    write(request_dir);
    read_directory(request);
    read_directory(request_dir);
    memcpy(request_dir.name, "nbunan\0\0", 8);
    read_directory(request_dir);
    memcpy(request_dir.name, "\0\0\0\0\0\0\0\0", 8);
    read_directory(request_dir);

    __asm__("int $0x4");

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t*) 0);
    
    while (TRUE) {
        keyboard_state_activate();
    }
}