#include "../lib-header/interrupt.h"
#include "../lib-header/idt.h"
#include "../lib-header/fat32.h"
#include "../lib-header/stdmem.h"

struct TSSEntry _interrupt_tss_entry = {
    .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
}; 

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA); 
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    if (cpu.eax == 0) {
        // Read file
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);
    } else if (cpu.eax == 1) {
        // Read directory
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read_directory(request);    
    } else if (cpu.eax == 2) {
        // Write file
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = write(request);
    } else if (cpu.eax == 3) {
        // Delete file
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = delete(request);
    } else if (cpu.eax == 4) {
        // Keyboard 
        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking());
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char *) cpu.ebx, buf, cpu.ecx);
        // execute_cmd(buf);
        reset_keyboard_buffer();
    } else if (cpu.eax == 5) {
        // Write string to framebuffer
        framebuffer_write_string((char*) cpu.ebx, (uint8_t) cpu.edx);
    } else if (cpu.eax == 6){
        // Set cursor position
        struct Cursor c = framebuffer_get_cursor();
        framebuffer_set_cursor(c.row + cpu.ebx, cpu.ecx);
    } else if (cpu.eax == 7){
        // Clear screen
        clear_screen();
    } else if (cpu.eax == 8){
        // Set parent cluster buffer for move command
        struct FAT32DirectoryTable src_table = *(struct FAT32DirectoryTable*) cpu.ebx;
        struct FAT32DirectoryTable dest_table = *(struct FAT32DirectoryTable*) cpu.ecx;
        uint32_t src_idx = (uint32_t) cpu.edx;


        for (uint32_t i = 1; i < CLUSTER_SIZE/(sizeof(struct FAT32DirectoryEntry)); i++) {
            if (dest_table.table[i].user_attribute != UATTR_NOT_EMPTY) {
                memcpy(dest_table.table[i].name, src_table.table[src_idx].name, 8);
                memcpy(dest_table.table[i].ext, src_table.table[src_idx].ext, 8);
                dest_table.table[i].attribute = src_table.table[src_idx].attribute;
                dest_table.table[i].user_attribute = src_table.table[src_idx].user_attribute;
                dest_table.table[i].undelete = src_table.table[src_idx].undelete;
                dest_table.table[i].create_time = src_table.table[src_idx].create_time;
                dest_table.table[i].create_date = src_table.table[src_idx].create_date;
                dest_table.table[i].access_date = src_table.table[src_idx].access_date;
                dest_table.table[i].modified_time = src_table.table[src_idx].modified_time;
                dest_table.table[i].modified_date = src_table.table[src_idx].modified_date;
                dest_table.table[i].filesize = src_table.table[src_idx].filesize;
                dest_table.table[i].cluster_high = src_table.table[src_idx].cluster_high;
                dest_table.table[i].cluster_low = src_table.table[src_idx].cluster_low;

                memcpy(src_table.table[src_idx].name, "\0\0\0\0\0\0\0\0", 8);
                memcpy(src_table.table[src_idx].ext, "\0\0\0", 3);
                src_table.table[src_idx].attribute = 0;
                src_table.table[src_idx].user_attribute = 0;
                src_table.table[src_idx].undelete = 0;
                src_table.table[src_idx].create_time = 0;
                src_table.table[src_idx].create_date = 0;
                src_table.table[src_idx].access_date = 0;
                src_table.table[src_idx].modified_time = 0;
                src_table.table[src_idx].modified_date = 0;
                src_table.table[src_idx].filesize = 0;
                src_table.table[src_idx].cluster_high = 0;
                src_table.table[src_idx].cluster_low = 0;
                break;
            }
        }
        
        uint32_t src_cluster_number = src_table.table[0].cluster_high << 16 | src_table.table[0].cluster_low;
        uint32_t dest_cluster_number = dest_table.table[0].cluster_high << 16 | dest_table.table[0].cluster_low;

        write_clusters(&(src_table), src_cluster_number, 1);
        write_clusters(&(dest_table), dest_cluster_number, 1);

    }
}

void main_interrupt_handler (
  __attribute__((unused)) struct CPURegister cpu,
  uint32_t int_number,
  __attribute__((unused)) struct InterruptStack info
) {
  switch (int_number) {
    case (PIC1_OFFSET + IRQ_KEYBOARD) :
      keyboard_isr();
      break;
    case PAGE_FAULT:
      __asm__("hlt");
      break;
    case 0x30:
      syscall(cpu, info);
      break;
  };
}



void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}
