#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // First 32-bit
            .segment_low = 0x0000,
            .base_low = 0x0000,

            // Next 16-bit (Bit 32 to 47)
            .base_mid = 0x00,
            .type_bit = 0x0,
            .non_system = 0x0,
            
            .dpl = 0x0,
            .segment_present = 0x0,
            .segment_limit = 0x0,
            .avl = 0x0,
            .code_seg_64bit = 0x0,
            .def_op_size = 0x0,
            .granularity = 0x0,
            .base_high = 0x0
        },
        {
            // Kernel Code Segment

            // First 32-bit
            .segment_low = 0xFFFF,
            .base_low = 0x0000,

            // Next 16-bit (Bit 32 to 47)
            .base_mid = 0x00,
            .type_bit = 0xA,
            .non_system = 0x1,
            
            .dpl = 0x0,
            .segment_present = 0x1,
            .segment_limit = 0xF,
            .avl = 0x0,
            .code_seg_64bit = 0x1,
            .def_op_size = 0x1,
            .granularity = 0x1,
            .base_high = 0x00
        },
        {
            // Kernel Data Segment

            // First 32-bit
            .segment_low = 0xFFFF,
            .base_low = 0x0000,

            // Next 16-bit (Bit 32 to 47)
            .base_mid = 0x00,
            .type_bit = 0x2,
            .non_system = 0x1,

            .dpl = 0x0,
            .segment_present = 0x1,
            .segment_limit = 0xF,
            .avl = 0x0,
            .code_seg_64bit = 0x0,
            .def_op_size = 0x1,
            .granularity = 0x1,
            .base_high = 0x00
        }
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    //        Use sizeof operator
    .size = sizeof(global_descriptor_table) - 1,
    
    .address = &global_descriptor_table
};

// static struct GlobalDescriptorTable global_descriptor_table = {
//     .table = {
//         {/* TODO: Null Descriptor */},
//         {/* TODO: Kernel Code Descriptor */},
//         {/* TODO: Kernel Data Descriptor */},
//         {/* TODO: User   Code Descriptor */},
//         {/* TODO: User   Data Descriptor */},
//         {
//             .segment_high      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
//             .segment_low       = sizeof(struct TSSEntry),
//             .base_high         = 0,
//             .base_mid          = 0,
//             .base_low          = 0,
//             .non_system        = 0,    // S bit
//             .type_bit          = 0x9,
//             .privilege         = 0,    // DPL
//             .valid_bit         = 1,    // P bit
//             .opr_32_bit        = 1,    // D/B bit
//             .long_mode         = 0,    // L bit
//             .granularity       = 0,    // G bit
//         },
//         {0}
//     }
// };

// void gdt_install_tss(void) {
//     uint32_t base = (uint32_t) &_interrupt_tss_entry;
//     global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
//     global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
//     global_descriptor_table.table[5].base_low  = base & 0xFFFF;
// }
