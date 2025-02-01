#include "keyboard.h"
#include "all.h"
#include "io.h"
#include "ioqueue.h"

#define KBD_BUF_PORT 0x60

#define esc         '\033'
#define backspace   '\b'
#define tab         '\t'
#define enter       '\r'
#define delete      '\177'

#define char_invisible 0
#define ctrl_l_char    char_invisible
#define ctrl_r_char    char_invisible
#define shift_l_char   char_invisible
#define shift_r_char   char_invisible
#define alt_l_char     char_invisible
#define alt_r_char     char_invisible
#define caps_lock_char char_invisible

#define shift_l_make 0x2a
#define shift_r_make 0x36
#define alt_l_make   0x38
#define alt_r_make   0xe038
#define alt_r_break  0xe0b8
#define ctrl_l_make  0x1d
#define ctrl_r_make  0xe01d
#define ctrl_r_break 0xe09d
#define caps_lock_make 0x3a

static int ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

struct ioqueue kbd_buf;

static char keymap[][2] = {
    {0, 0},
    {esc, esc},
    {1, '!'},
    {2, '@'},
    {3, '#'},
    {4, '$'},
    {5, '%'},
    {6, '^'},
    {7, '&'},
    {8, '*'},
    {9, '('},
    {0, ')'},
    {'-', '_'},
    {'=', '+'},
    {backspace, backspace},
    {tab, tab},
    {'q', 'Q'},
    {'w', 'W'},
    {'e', 'E'},
    {'r', 'R'},
    {'t', 'T'},
    {'y', 'Y'},
    {'u', 'U'},
    {'i', 'I'},
    {'o', 'O'},
    {'p', 'P'},
    {'[', '{'},
    {']', '}'},
    {enter, enter},
    {ctrl_l_char, ctrl_l_char},
    {'a', 'A'},
    {'s', 'S'},
    {'d', 'D'},
    {'f', 'F'},
    {'g', 'G'},
    {'h', 'H'},
    {'j', 'J'},
    {'k', 'K'},
    {'l', 'L'},
    {';', ':'},
    {'\'', '"'},
    {'`', '~'},
    {shift_l_char, shift_l_char},
    {'\\', '|'},
    {'z', 'Z'},
    {'x', 'X'},
    {'c', 'C'},
    {'v', 'V'},
    {'b', 'B'},
    {'n', 'N'},
    {'m', 'M'},
    {',', '<'},
    {'.', '>'},
    {'/', '?'},
    {shift_r_char, shift_r_char},
    {'*', '*'},
    {alt_l_char, alt_l_char},
    {' ', ' '},
    {caps_lock_char, caps_lock_char}
};

static void intr_keyboard_handler(void) {
    uint16_t code = inb(KBD_BUF_PORT);

    if (code == 0xe0) {
        // 扩展字符，等待第二个扫描码
        ext_scancode = 1;
        return;
    }

    if (ext_scancode) {
        code = ((0xe000) | code);
        ext_scancode = 0;
    }

    // 是否是断码
    int is_break_code = ((code & 0x0080) != 0);

    if (is_break_code) {
        // 获得其通码
        uint16_t make_code = (code &= 0xff7f);
        if (make_code == ctrl_l_make || make_code == ctrl_r_make) {
            ctrl_status = 0;
        } else if (make_code == shift_l_make || make_code == shift_r_make) {
            shift_status = 0;
        } else if (make_code == alt_l_make || make_code == alt_r_make) {
            alt_status = 0;
        }
        return;
    }

    // 到这里就说明是通码，但是我们只能处理0x01-0x3a之间的按键，再加上两个特殊通码
    if ((code <= 0x00 || code > 0x3a) && (code != alt_r_make && code != ctrl_r_make)) {
        put_str("Unkown key.\n");
        return;
    }

    int shift = 0;

    if (code < 0x0e || code == 0x29 || code == 0x1a || code == 0x1b || code == 0x2b || code == 0x27 || code == 0x28 || code == 0x33 || code == 0x34 || code == 0x35) {
        // 0-9, [等字符不关心caps
        shift = shift_status;
    } else {
        if (shift_status && caps_lock_status) {
            // shift和caps同时按下，相互抵消
            shift = 0;
        } else if (shift_status || caps_lock_status) {
            shift = 1;
        } else {
            shift = 0;
        }
    }

    uint8_t index = (code & 0x00ff);

    char cur_char = keymap[index][shift];

    if (cur_char && !is_queue_full(&kbd_buf)) {  // 非空字符且缓冲区未满
        put_char(cur_char);
        ioq_putchar(&kbd_buf, cur_char);
        return;
    }

    if (code == ctrl_l_make || code == ctrl_r_make) {
        ctrl_status = 1;
    } else if (code == shift_l_make || code == shift_r_make) {
        shift_status = 1;
    } else if (code == alt_l_make || code == alt_r_make) {
        alt_status = 1;
    } else if (code == caps_lock_make) {
        caps_lock_status = !caps_lock_status;
    }
}

void keyboard_init() {
    mylog("    keyboard_init\n", LOG_INIT);
    register_handler(0x21, intr_keyboard_handler);
    mylog("    keyboard_init done\n", LOG_OK);
}