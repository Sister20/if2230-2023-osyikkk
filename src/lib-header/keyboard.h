#ifndef _USER_ISR_H
#define _USER_ISR_H

#include "stdtype.h"
#include "interrupt.h"
#include "portio.h"

#define EXT_SCANCODE_UP        0x48
#define EXT_SCANCODE_DOWN      0x50
#define EXT_SCANCODE_LEFT      0x4B
#define EXT_SCANCODE_RIGHT     0x4D

#define KEYBOARD_DATA_PORT     0x60
#define EXTENDED_SCANCODE_BYTE 0xE0

#define KEYBOARD_BUFFER_SIZE   256

#define SC_MAX 57
#define SCANCODE_KEYUP_THRESHOLD 0x80
#define SCANCODE_LSHIFT 42
#define SCANCODE_CAPS 58
#define SCANCODE_LEFTARROW 75
#define SCANCODE_ESC 27

/**
 * keyboard_scancode_1_to_ascii_map[256], Convert scancode values that correspond to ASCII printables
 * How to use this array: ascii_char = k[scancode]
 * 
 * By default, QEMU using scancode set 1 (from empirical testing)
 */
extern const char keyboard_scancode_1_to_ascii_map[256];

/**
 * KeyboardDriverState - Contain all driver states
 * 
 * @param read_extended_mode Optional, can be used for signaling next read is extended scancode (ex. arrow keys)
 * @param keyboard_input_on  Indicate whether keyboard ISR is activated or not
 * @param buffer_index       Used for keyboard_buffer index
 * @param keyboard_buffer    Storing keyboard input values in ASCII
 */
struct KeyboardDriverState {
    bool    read_extended_mode;
    bool    keyboard_input_on;
    bool    shift_pressed;
    bool    ctrl_pressed;
    bool    alt_pressed;
    bool    caps_cond;
    uint8_t buffer_index;
    char    keyboard_buffer[KEYBOARD_BUFFER_SIZE];
} __attribute((packed));





/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void);

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void);

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf);

// Check whether keyboard ISR is active or not - @return Equal with keyboard_input_on value
bool is_keyboard_blocking(void);


/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 * 
 * Will only print printable character into framebuffer.
 * Stop processing when enter key (line feed) is pressed.
 * 
 * Note that, with keyboard interrupt & ISR, keyboard reading is non-blocking.
 * This can be made into blocking input with `while (is_keyboard_blocking());` 
 * after calling `keyboard_state_activate();`
 */
void keyboard_isr(void);

/**
 * @brief Clear screen
 * 
 * 
 */
void clear_screen();

/**
 * @brief Print string to screen
 * 
 * @param char* s
 * @param int n 
 */
void append(char s[], char n);

/**
 * @brief Compare two string
 * 
 * @param s1 
 * @param s2 
 * @return int 
 */
int strcmp(char s1[], char s2[]);

/**
 * @brief Execute command from input
 * 
 * @param char* input 
 */
void execute_cmd(char *input);

#endif