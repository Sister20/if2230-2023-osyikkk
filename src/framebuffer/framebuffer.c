#include "../lib-header/framebuffer.h"
#include "../lib-header/stdtype.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/portio.h"


void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t pos = r * MAX_COLS + c;    
    out(CURSOR_PORT_CMD, VGA_CURSOR_HIGH);
    out(CURSOR_PORT_DATA, (uint8_t)((pos >> 8) & 0xFF));
    out(CURSOR_PORT_CMD, VGA_CURSOR_LOW);
    out(CURSOR_PORT_DATA, (uint8_t)(pos & 0xFF));
}

struct Cursor framebuffer_get_cursor() {
    out(CURSOR_PORT_CMD, VGA_CURSOR_HIGH);
    int offset = in(CURSOR_PORT_DATA) << 8;
    out(CURSOR_PORT_CMD, VGA_CURSOR_LOW);
    offset += in(CURSOR_PORT_DATA);
    struct Cursor c = 
    {
        .row = offset / MAX_COLS, 
        .col = offset % MAX_COLS
    };
    return c;
};

int framebuffer_get_col(struct Cursor c) {
    return c.col;
}

int framebuffer_get_row(struct Cursor c) {
    return c.row;
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    uint16_t attrib = (bg << 4) | (fg & WHITE);
    volatile uint16_t * where;
    where = (volatile uint16_t *)MEMORY_FRAMEBUFFER + (row * MAX_COLS + col) ;
    *where = c | (attrib << 8); 
}

void framebuffer_clear(void) {
    uint16_t space = 0x20 | (0x07 << 8);
    uint16_t i;
    volatile uint16_t * where;
    where = (volatile uint16_t *)MEMORY_FRAMEBUFFER;
    for (i = 0; i < MAX_COLS * MAX_ROWS; i++) {
        *where = space;
        where++;
    }   
}

void framebuffer_write_string(char * str) {
    struct Cursor c = framebuffer_get_cursor();
    int offset = c.row * MAX_COLS + c.col;
    int i = 0;
    while (str[i] != '\0') {
        if (offset >= MAX_COLS * (MAX_ROWS)) {
             offset = framebuffer_scroll_ln(offset);
        }
        if (str[i] == '\n') {
            offset = (offset / MAX_COLS + 1) * MAX_COLS;
        } else {
            framebuffer_write(offset / MAX_COLS, offset % MAX_COLS, str[i], WHITE, BLACK);
            offset += 1;
        }
        i++;
    }

    framebuffer_set_cursor(offset / MAX_COLS, offset % MAX_COLS);
}

int framebuffer_scroll_ln(int offset) {
    memcpy(
        (void *)MEMORY_FRAMEBUFFER, 
        (void *)(MEMORY_FRAMEBUFFER + MAX_COLS * 2),
        MAX_COLS * 2 * (MAX_ROWS));

    for (int i = 0; i < MAX_COLS; i++) {
        framebuffer_write(MAX_ROWS - 1, i, ' ', WHITE, BLACK);
    }

    return (offset) - (1 * (MAX_COLS));
}