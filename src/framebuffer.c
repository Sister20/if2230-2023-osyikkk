#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    out(0x3D4, 0x0A);
	out(0x3D5, (in(0x3D5) & 0xC0) | r);
 
	out(0x3D4, 0x0B);
	out(0x3D5, (in(0x3D5) & 0xE0) | c);
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    volatile uint16_t * where;
    where = (volatile uint16_t *)0xB8000 + (row * 80 + col) ;
    *where = c | (attrib << 8); 
}

void framebuffer_clear(void) {
    // TODO : Implement
    uint16_t space = 0x20 | (0x07 << 8); // " " in ASCII with white text on black background 
    uint16_t i;
    volatile uint16_t * where;
    where = (volatile uint16_t *)0xB8000;
    for (i = 0; i < 80 * 25; i++) {
        *where = space;
        where++;
    }   
}
