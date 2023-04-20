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

/**
 * Keycodes shared between the keyboard driver and applications/
 * userspace. Each keycode is a single byte representing a physical
 * key. These values are taken from the USB HID keyboard usage page.
 *
 * This could also be defined using an enum, but macros are used
 * as these are simple constants to be replaced at compile-time.
 */
#define KC_A 0x04
#define KC_B 0x05
#define KC_C 0x06
// TODO(jlam55555): Finish this.

/**
 * Mappings from keycodes to ASCII characters. This depends on the
 * keyboard layout. A return value of -1 indicates that there is
 * no corresponding ASCII character.
 *
 * TODO(jlam55555): Fill these out.
 */
extern const char kc_to_ascii_map_qwerty[256];
extern const char kc_to_ascii_map_colemak[256];

inline char kc_to_ascii(uint8_t kc, const char kc_to_ascii_map[256]) {
  return kc_to_ascii_map[kc];
}

#endif // COMMON_KEYCODES_H
