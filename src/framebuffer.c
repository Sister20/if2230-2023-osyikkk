#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    out(CURSOR_PORT_CMD, 0x0A);
	out(CURSOR_PORT_DATA, (in(CURSOR_PORT_DATA) & 0xC0) | r);
 
	out(CURSOR_PORT_CMD, 0x0B);
	out(CURSOR_PORT_DATA, (in(CURSOR_PORT_DATA) & 0xE0) | c);
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement
    uint16_t attrib = (bg << 4) | (fg & 0x0F);
    volatile uint16_t * where;
    where = (volatile uint16_t *)MEMORY_FRAMEBUFFER + (row * 80 + col) ;
    *where = c | (attrib << 8); 
}

void framebuffer_clear(void) {
    // TODO : Implement
    uint16_t space = 0x20 | (0x07 << 8); // " " in ASCII with white text on black background 
    uint16_t i;
    volatile uint16_t * where;
    where = (volatile uint16_t *)MEMORY_FRAMEBUFFER;
    for (i = 0; i < 80 * 25; i++) {
        *where = space;
        where++;
    }   
}
