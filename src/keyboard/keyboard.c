#include "../lib-header/keyboard.h"

#include "../lib-header/framebuffer.h"
#include "../lib-header/portio.h"
#include "../lib-header/stdmem.h"

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
// LSHIFT SCANCODE_LSHIFT
const char scancode_capital_letters[] = {
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
  // Get cursor index
  struct Cursor cursor = framebuffer_get_cursor();
  int row = cursor.row, col = cursor.col;

  if (!keyboard_state.keyboard_input_on) {
    // Set buffer index to zero if not receiving input
    keyboard_state.buffer_index = 0;
  } else {
    // Get scancode
    uint8_t scancode = in(KEYBOARD_DATA_PORT);
    if (scancode == SCANCODE_LSHIFT) {
      // Left shift scancode key down
      keyboard_state.shift_pressed = 1;
    }
    if (scancode == SCANCODE_LSHIFT + SCANCODE_KEYUP_THRESHOLD) {
      // Left shift scancode key up
      keyboard_state.shift_pressed = 0;
    }

    if (scancode == SCANCODE_CAPS) {
      // Caps lock scancode
      keyboard_state.caps_cond = !keyboard_state.caps_cond;
    }

    char mapped_char;
    if (!(keyboard_state.shift_pressed ^ keyboard_state.caps_cond)) {
      // Handle lowercase letters
      mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
    } else {
      // Handle uppercase letters
      mapped_char = scancode_capital_letters[scancode];
    }

    // Handle if scancode is key down and not capslock
    if (scancode < SCANCODE_KEYUP_THRESHOLD && scancode != SCANCODE_LSHIFT && scancode != SCANCODE_CAPS) {
      if (scancode != tempCode) {
        if (mapped_char == '\b' && keyboard_state.buffer_index > 0) {
          if (col == 0) {
            framebuffer_set_cursor(row - 1, MAX_COLS - 1);
            framebuffer_write(row - 1, MAX_COLS - 1, '\0', WHITE, BLACK);
          } else {
            framebuffer_set_cursor(row, col - 1);
            framebuffer_write(row, col - 1, '\0', WHITE, BLACK);
          }
          keyboard_state.buffer_index--;
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
        } else if (mapped_char == '\n') {
          // execute_cmd(keyboard_state.keyboard_buffer);
          // memset(keyboard_state.keyboard_buffer, 0, 256);
          // keyboard_state.buffer_index = 0;
          keyboard_state_deactivate();
        } else if (mapped_char != '\b' && scancode != SCANCODE_LEFTARROW) {
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index] =
              mapped_char;
          keyboard_state.buffer_index++;

          int offset = row * MAX_COLS + col;
          if (row == (MAX_ROWS - 1) && col == (MAX_COLS - 1)) {
            offset = framebuffer_scroll_ln(offset);
          }

          framebuffer_write(offset / MAX_COLS, offset % MAX_COLS, mapped_char, WHITE, BLACK);
          framebuffer_set_cursor(offset / MAX_COLS, offset % MAX_COLS + 1);
        } else if (scancode == SCANCODE_LEFTARROW && col != 2) {
          framebuffer_set_cursor(row, col - 1);
        }
        tempCode = scancode;
        if (mapped_char == SCANCODE_ESC) {
          clear_screen();
        }
        // framebuffer_set_cursor(row, col);
      }
    } else {
      tempCode = scancode;
      // countCode = 0;
    }
  }
  // framebuffer_set_cursor(row, col);
  pic_ack(IRQ_KEYBOARD);
}

void reset_keyboard_buffer(void) {
  memset(keyboard_state.keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
  keyboard_state.buffer_index = 0;
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
  framebuffer_write_string("> ", WHITE);
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

// void execute_cmd(char *input) {
//   // execute command from input 
//   if (strcmp(input, "clear") == 0) { 
//     // clear screen
//     clear_screen();
//     framebuffer_set_cursor(0, 0);
//     framebuffer_write_string("> ", WHITE);
//   } else {
//     if (strcmp(input, "help") == 0) {
//       // list of available commands
//       framebuffer_write_string("\nAvailable commands:\n", WHITE);
//       framebuffer_write_string("clear - Clear the screen\n", WHITE);
//       framebuffer_write_string("help - List of available commands", WHITE);
//     } else if (strcmp(&input[2], "rm") == 0) {
//       framebuffer_write_string("\nDeleting file: ", WHITE);
//       framebuffer_write_string(input, WHITE);
//     } else {
//       framebuffer_write_string("\nCommand not found: ", WHITE);
//       framebuffer_write_string(input, WHITE);
//     }
//     struct Cursor c = framebuffer_get_cursor();
//     framebuffer_set_cursor(c.row + 1, 0);
//     framebuffer_write_string("> ", WHITE);
//   } 
// }
