#include "../lib-header/keyboard.h"

#include "../lib-header/framebuffer.h"
#include "../lib-header/portio.h"
#include "../lib-header/stdmem.h"

// TODO : Add Feature 

#define SC_MAX 57
const char keyboard_scancode_1_to_ascii_map[256] = {
    0,   0x1B, '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,

    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,
};
// CTRL 29
// LSHIFT 42
const char scancode_to_char[] = {
    0,   0x1B, '!', '@', '#',  '$', '%', '^',  '&', '*', '(', ')',
    '_', '+',  0,   0,   'Q',  'W', 'E', 'R',  'T', 'Y', 'U', 'I',
    'O', 'P',  '[', ']', '?',  '?', 'A', 'S',  'D', 'F', 'G', 'H',
    'J', 'K',  'L', ':', '\"', '~', 0,   '\\', 'Z', 'X', 'C', 'V',
    'B', 'N',  'M', '<', '>',  '?', 0,   0,    0,   0};

static struct KeyboardDriverState keyboard_state = {.caps_cond = 0};
// static int writing = 1;
static int tempCode = 0;
// static int countCode = 0;

void keyboard_state_activate(void) { keyboard_state.keyboard_input_on = 1; }

void keyboard_state_deactivate(void) { keyboard_state.keyboard_input_on = 0; }

void keyboard_isr(void) {
  struct Cursor cursor = framebuffer_get_cursor();
  int row = cursor.row, col = cursor.col;
  if (!keyboard_state.keyboard_input_on) {
    keyboard_state.buffer_index = 0;
  } else {
    uint8_t scancode = in(KEYBOARD_DATA_PORT);
    if (scancode == 42) {
      keyboard_state.shift_pressed = 1;
    }
    if (scancode == 42 + 0x80) {
      keyboard_state.shift_pressed = 0;
    }

    if (scancode == 58) {
      keyboard_state.caps_cond = !keyboard_state.caps_cond;
    }

    char mapped_char;
    if (!(keyboard_state.shift_pressed ^ keyboard_state.caps_cond)) {
      mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
    } else {
      // if ()
      mapped_char = scancode_to_char[scancode];
    }
    // TODO : Implement scancode processing
    // if(scancode < 0x80) {
    //   if(writing == 1) {
    //     framebuffer_write(row, col, mapped_char, 0xF, 0);
    //     col++;
    //     writing = 0;
    //   }
    // } else {
    //   if(writing == 0) {
    //     writing = 1;
    //   }
    // }

    if (scancode < 0x80 && scancode != 42 && scancode != 58) {
      // countCode++;
      // if(scancode != tempCode || countCode > 10) {
      if (scancode != tempCode) {
        if (mapped_char == '\b' && keyboard_state.buffer_index > 0) {
          if (col == 0) {
            framebuffer_set_cursor(row - 1, MAX_COLS - 1);
            framebuffer_write(row - 1, MAX_COLS - 1, ' ', WHITE, BLACK);
          } else {
            framebuffer_set_cursor(row, col - 1);
            framebuffer_write(row, col - 1, ' ', WHITE, BLACK);
          }
          keyboard_state.buffer_index--;
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = ' ';
        } else if (mapped_char == '\n') {
          execute_cmd(keyboard_state.keyboard_buffer);
          memset(keyboard_state.keyboard_buffer, 0, 256);
          keyboard_state.buffer_index = 0;
        } else if (mapped_char != '\b' && scancode != 75) {
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index] =
              mapped_char;
          keyboard_state.buffer_index++;
          framebuffer_write(row, col, mapped_char, 0xF, 0);
          framebuffer_set_cursor(row, col + 1);
        } else if (scancode == 75 && col != 2) {
          framebuffer_set_cursor(row, col - 1);
          keyboard_state.buffer_index--;
        }
        // if(scancode != tempCode) {
        //   countCode = 0;
        // }
        tempCode = scancode;
        if (mapped_char == 27) {
          clear_screen();
        }
        // framebuffer_set_cursor(row, col);
      }
    } else {
      tempCode = scancode;
      // countCode = 0;
    }
    // if (scancode > 0x80) {
    //   if (scancode <= (0x39 + 0x80)){
    //     keyboard_state.shift_pressed = 0;
    //   } else if (scancode == (0x1D + 0x80)) {
    //     keyboard_state.ctrl_pressed = 0;
    //   } else if (scancode == (0x38 + 0x80)) {
    //     keyboard_state.alt_pressed = 0;
    //   }
    // framebuffer_write(row, col,
    // keyboard_state.keyboard_buffer[keyboard_state.buffer_index-1], 0xF, 0);
    // keyboard_state.buffer_index = 0;
    // // col++;
    // }
    //   else if (mapped_char == '\b') {
    //     if (col == 0) {
    //       row--;
    //       col = 15;
    //     } else {
    //       keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = ' ';
    //       keyboard_state.buffer_index--;
    //       // framebuffer_write(row, col, ' ', 0xF, 0);
    //       col--;
    //     }
    //   } else if (mapped_char != 0 && mapped_char != '\n') {
    //     keyboard_state.keyboard_buffer[keyboard_state.buffer_index] =
    //     mapped_char; keyboard_state.buffer_index++;
    //     // col++;
    //   } else if (mapped_char == '\n') {
    //     keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\n';
    //     row++;
    //     col = 0;
    //   }
    //   if (mapped_char == 'c'){
    //     framebuffer_clear();
    //     row = 0; col = 0;
    //   }
    // }
    // col++;
  }
  // framebuffer_set_cursor(row, col);
  pic_ack(IRQ_KEYBOARD);
}

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended
// size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf) {
  memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
  // buf = keyboard_state.keyboard_buffer;
}

// Check whether keyboard ISR is active or not - @return Equal with
// keyboard_input_on value
bool is_keyboard_blocking(void) { return keyboard_state.keyboard_input_on; }

void clear_screen() {
  framebuffer_clear();
  framebuffer_set_cursor(0, 0);
  framebuffer_write_string("> ");
  memset(keyboard_state.keyboard_buffer, 0, 256);
}

int strcmp(char *s1, char *s2) {
  int i = 0;
  while (s1[i] == s2[i]) {
    if (s1[i] == '\0') {
      return 0;
    }
    i++;
  }
  return s1[i] - s2[i];
}

void execute_cmd(char *input) {
  if (strcmp(input, "clear") == 0) {
    clear_screen();
    framebuffer_set_cursor(0, 0);
    framebuffer_write_string("> ");
  } else {
    if (strcmp(input, "help") == 0) {
      framebuffer_write_string("\nAvaible commands:\n");
      framebuffer_write_string("clear - Clear the screen\n");
      framebuffer_write_string("help - List of available commands\n");
    } else if (strcmp(&input[3], "del") == 0) {
      framebuffer_write_string("\nCommand not found: ");
      framebuffer_write_string(input);
    } else {
      framebuffer_write_string("\nCommand not found: ");
      framebuffer_write_string(input);
    }
    struct Cursor c = framebuffer_get_cursor();
    framebuffer_set_cursor(c.row + 1, 0);
    framebuffer_write_string("> ");
  } 
}