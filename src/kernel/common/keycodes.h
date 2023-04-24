/**
 * Keyboard keycodes. This is meant to be a standard
 * interface between the keyboard driver and applications/
 * userspace.
 *
 * The keyboard driver receives "scan codes" from the PS/2
 * keyboard device and translates them into "key codes".
 * Scancodes are standardized for the PS/2 interface, but they
 * are not great for use for applications/userspace, because:
 * - They may be multiple bytes.
 * - There are multiple "scan code sets".
 * Thus we use the idea of a "key code", which represents a key
 * on the keyboard using a one-byte code whose interpretation
 * is shared between the kernel and userspace. For our purposes,
 * I use the "keyboard page usage ID's" defined in the USB HID
 * specification for keycodes
 * (https://usb.org/sites/default/files/hut1_3_0.pdf#page=89),
 * but overall there is no one standard for keycodes.
 *
 * Scancodes and keycodes are used to represent a physical key
 * on the keyboard, such as the "A" key or the "Print Screen" key.
 * There are two other related concepts that are useful for
 * application input:
 * - Key event: represents a key, along with whether the event was
 *   a keydown, keypress (key repeat), or keyup event.
 * - ASCII char: some keys correspond to ASCII characters, while
 *   others do not. For example, the "A" key corresponds to ASCII
 *   'a' or 'A' (depending on if the Shift key is pressed), and
 *   the "Enter" or "Return" key corresponds to a '\n' newline
 *   character. Note that the mapping from physical key to ASCII
 *   character is a function of the keyboard layout.
 *
 * Note that key combinations can be managed using key events
 * and a mapping of pressed keys.
 *
 * TODO(jlam55555): Turn this into an entry in docs/.
 */
#ifndef COMMON_KEYCODES_H
#define COMMON_KEYCODES_H

#include <stdint.h>

/**
 * Keycodes shared between the keyboard driver and applications/
 * userspace. Each keycode is a single byte representing a physical
 * key. These values are taken from the USB HID keyboard usage page.
 *
 * The abbreviations are a work in progress. Common abbrevations
 * (e.g., RET = RETURN, SHFT = SHIFT, etc.) are accepted; less-common
 * keys are spelled out in their entirety.
 *
 * Future work may be to fix up some of these acronyms and to introduce
 * aliases where appropriate.
 */
enum keycode {
  // Usage 0 is reserved in the USBHID spec.
  KC_INVAL = 0x00, /* = 0x00 */
  KC_A = 0x04,
  KC_B,
  KC_C,
  KC_D,
  KC_E,
  KC_F,
  KC_G,
  KC_H,
  KC_I,
  KC_J,
  KC_K,
  KC_L,
  KC_M, /* = 0x10 */
  KC_N,
  KC_O,
  KC_P,
  KC_Q,
  KC_R,
  KC_S,
  KC_T,
  KC_U,
  KC_V,
  KC_W,
  KC_X,
  KC_Y,
  KC_Z,
  KC_1,
  KC_2,
  KC_3, /* = 0x20 */
  KC_4,
  KC_5,
  KC_6,
  KC_7,
  KC_8,
  KC_9,
  KC_0,
  KC_RET,
  KC_ESC,
  KC_BKSP,
  KC_TAB,
  KC_SPACE,
  KC_HYPHEN,
  KC_EQUALS,
  KC_LBRKT,
  KC_RBRKT, /* = 0x30 */
  KC_BKSLASH,
  KC_POUND, /* non-US keyboards */
  KC_SEMICOLON,
  KC_QUOTE,
  KC_BACKTICK,
  KC_COMMA,
  KC_PERIOD,
  KC_SLASH,
  KC_CAPS_LOCK,
  KC_F1,
  KC_F2,
  KC_F3,
  KC_F4,
  KC_F5,
  KC_F6,
  KC_F7, /* = 0x40 */
  KC_F8,
  KC_F9,
  KC_F10,
  KC_F11,
  KC_F12,
  KC_PRINT_SCREEN,
  KC_SCROLL_LOCK,
  KC_PAUSE,
  KC_INSERT,
  KC_HOME,
  KC_PAGE_UP,
  KC_DEL,
  KC_END,
  KC_PAGE_DOWN,
  KC_RIGHT,
  KC_LEFT, /* = 0x50 */
  KC_DOWN,
  KC_UP,
  KC_NUM_LOCK,
  KC_KP_DIVIDE,
  KC_KP_MULTIPLY,
  KC_KP_MINUS,
  KC_KP_ADD,
  KC_KP_ENTER,
  KC_KP_1,
  KC_KP_2,
  KC_KP_3,
  KC_KP_4,
  KC_KP_5,
  KC_KP_6,
  KC_KP_7,
  KC_KP_8, /* = 0x60 */
  KC_KP_9,
  KC_KP_0,
  KC_KP_PERIOD,
  KC_BKSLASH2, /* non-US keyboards */
  KC_APPLICATION,
  KC_POWER,
  KC_KP_EQUALS,
  KC_F13,
  KC_F14,
  KC_F15,
  KC_F16,
  KC_F17,
  KC_F18,
  KC_F19,
  KC_F20,
  KC_F21, /* = 0x70 */
  KC_F22,
  KC_F23,
  KC_F24,
  KC_EXECUTE,
  KC_HELP,
  KC_MENU,
  KC_SELECT,
  KC_STOP,
  KC_AGAIN,
  KC_UNDO,
  KC_CUT,
  KC_COPY,
  KC_PASTE,
  KC_FIND,
  KC_MUTE,
  KC_VOLUME_UP, /* = 0x80 */
  KC_VOLUME_DOWN,
  KC_LOCKING_CAPS_LOCK,   /* legacy */
  KC_LOCKING_NUM_LOCK,    /* legacy */
  KC_LOCKING_SCROLL_LOCK, /* legacy */
  KC_KP_COMMA,
  KC_KP_EQUALS2, /* AS/400 keyboards */
  KC_INTERNATIONAL1,
  KC_INTERNATIONAL2,
  KC_INTERNATIONAL3,
  KC_INTERNATIONAL4,
  KC_INTERNATIONAL5,
  KC_INTERNATIONAL6,
  KC_INTERNATIONAL7,
  KC_INTERNATIONAL8,
  KC_INTERNATIONAL9,
  KC_LANG1, /* = 0x90 */
  KC_LANG2,
  KC_LANG3,
  KC_LANG4,
  KC_LANG5,
  KC_LANG6,
  KC_LANG7,
  KC_LANG8,
  KC_LANG9,
  KC_ALTERNATE_ERASE,
  KC_SYSREQ,
  KC_CANCEL,
  KC_CLEAR,
  KC_PRIOR,
  KC_RETURN2,
  KC_SEPARATOR,
  KC_OUT, /* = 0xA0 */
  KC_OPER,
  KC_CLEAR_AGAIN,
  KC_CRSEL,
  KC_EXCEL,
  KC_KP_00 = 0xB0, /* = 0xB0 */
  KC_KP_000,
  KC_THOUSANDS_SEPARATOR,
  KC_DECIMAL_SEPARATOR,
  KC_CURRENCY_UNIT,
  KC_CURRENCY_SUBUNIT,
  KC_KP_LPAREN,
  KC_KP_RPAREN,
  KC_KP_LBRACE,
  KC_KP_RBRACE,
  KC_KP_TAB,
  KC_KP_BKSP,
  KC_KP_A,
  KC_KP_B,
  KC_KP_C,
  KC_KP_D,
  KC_KP_E, /* = 0xC0 */
  KC_KP_F,
  KC_KP_XOR,
  KC_KP_CARET,
  KC_KP_MODULUS,
  KC_KP_LANGBRKT,
  KC_KP_RANGBRKT,
  KC_KP_AMPERSAND,
  KC_KP_DOUBLE_AMPERSAND,
  KP_KP_BAR,
  KC_KP_DOUBLE_BAR,
  KC_KP_COLON,
  KC_KP_POUND,
  KC_KP_SPACE,
  KC_KP_AT,
  KC_KP_EXCLAMATION,
  KC_KP_MEMORY_STORE, /* = 0xD0 */
  KC_KP_MEMORY_RECALL,
  KC_KP_MEMORY_CLEAR,
  KC_KP_MEMORY_ADD,
  KC_KP_MEMORY_SUBTRACT,
  KC_KP_MEMORY_MULTIPLY,
  KC_KP_MEMORY_DIVIDE,
  KC_KP_PLUS_MINUS,
  KC_KP_CLEAR,
  KC_KP_CLEAR_ENTRY,
  KC_KP_BINARY,
  KC_KP_OCTAL,
  KC_KP_DECIMAL,
  KC_KP_HEXADECIMAL,
  KC_LCTRL = 0xE0, /* = 0xE0 */
  KC_LSHFT,
  KC_LALT,
  KC_LGUI, /* Windows key */
  KC_RCTRL,
  KC_RSHFT,
  KC_RALT,
  KC_RGUI,
  /* 0xE8-0xFF(FF) are reserved in the USBHID spec. */
};

// Modifier keys.
enum keyboard_modifiers {
  KM_CTRL = 0x01,
  KM_SHFT = 0x02,
  KM_ALT = 0x04,
  KM_CAPS_LOCK = 0x08,
  KM_NUM_LOCK = 0x10,
  KM_SCROLL_LOCK = 0x20,
};

/**
 * Mappings from keycodes to ASCII characters. This depends on the
 * keyboard layout. A return value of -1 indicates that there is
 * no corresponding ASCII character.
 */
extern const char kc_to_ascii_map_qwerty[2][256];
extern const char kc_to_ascii_map_colemak[2][256];

/**
 * Combination of a keycode with an event type.
 */
struct kbd_event {
  enum keycode kc;
  uint8_t km;
  char ascii;
  enum kbd_event_type {
    KBD_EVENT_KEYDOWN,
    KBD_EVENT_KEYUP,
    KBD_EVENT_KEYPRESS,
  } type;
};

/**
 * Generate an ASCII character for a keyboard event, or -1 for a non-ASCII
 * character. Takes the Shift, Caps Lock, and Num Lock modifier keys into
 * account, and ignores other modifiers keys.
 *
 * Notes:
 * - This takes as argument a keyboard event rather than a keycode,
 *   because some ASCII characters depend on multiple keys. (For example,
 *   the Shift key modifies the ASCII output).
 * - This also takes a keyboard mapping (keyboard layout) argument.
 * - This doesn't generate special textual representations for non-printable
 *   ASCII characters (0x00 to 0x1F, corresponding to ^@ to ^_), nor does it
 *   generate ANSI escape sequences for special keycodes (e.g., cursor arrow
 *   keys or the Insert key). The former capability is implemented in the
 *   terminal ECHO, and the second is implemented in the keyboard driver
 *   callback for the terminal (which may be moved to a more generic input
 *   subsystem).
 *
 * TODO(jlam55555): Handle Num Lock, and fix Caps Lock behavior (which
 *     shouldn't behave the same as Shift). This may require two more
 *     keymaps for each keyboard layout.
 * TODO(jlam55555): Make a keyboard layout into a struct.
 */
inline char kc_to_ascii(struct kbd_event *evt,
                        const char kc_to_ascii_map[2][256]) {
  int shift = !!(evt->km & evt->km & KM_CAPS_LOCK) ^ !!(evt->km & KM_SHFT);
  return evt->ascii = kc_to_ascii_map[shift][evt->kc];
}

#endif // COMMON_KEYCODES_H
