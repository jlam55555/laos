/**
 * VT100 (ANSI) escape sequences. Sample reference:
 * https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
 *
 * TODO(jlam55555): Add support for these in the console. Currently these are
 * not supported in the console, and only display correctly when displayed by an
 * outside shell.
 */

#ifndef COMMON_VT100_H
#define COMMON_VT100_H

#include "common/util.h"

#define VT100_SEQ_SEP ";"
#define VT100_SEQ_START "\x1B["
#define VT100_SEQ_END "m"

/**
 * Generate VT100 escape sequences with the given modes. More levels can be
 * added as necessary, but 8 levels should be plenty for most needs.
 */
#define VT100_ESC(mode) VT100_SEQ_START mode VT100_SEQ_END
#define VT100_ESC2(...)                                                        \
  VT100_SEQ_START JOIN2(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END
#define VT100_ESC3(...)                                                        \
  VT100_SEQ_START JOIN3(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END
#define VT100_ESC4(...)                                                        \
  VT100_SEQ_START JOIN4(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END
#define VT100_ESC5(...)                                                        \
  VT100_SEQ_START JOIN5(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END
#define VT100_ESC6(...)                                                        \
  VT100_SEQ_START JOIN6(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END
#define VT100_ESC7(...)                                                        \
  VT100_SEQ_START JOIN7(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END
#define VT100_ESC8(...)                                                        \
  VT100_SEQ_START JOIN8(VT100_SEQ_SEP, __VA_ARGS__) VT100_SEQ_END

#define VT100_CODE_FG_BLACK "30"
#define VT100_CODE_FG_RED "31"
#define VT100_CODE_FG_GREEN "32"
#define VT100_CODE_FG_YELLOW "33"
#define VT100_CODE_FG_BLUE "34"
#define VT100_CODE_FG_MAGENTA "35"
#define VT100_CODE_FG_CYAN "36"
#define VT100_CODE_FG_WHITE "37"
#define VT100_CODE_FG_DEFAULT "39"

#define VT100_CODE_BG_BLACK "40"
#define VT100_CODE_BG_RED "41"
#define VT100_CODE_BG_GREEN "42"
#define VT100_CODE_BG_YELLOW "43"
#define VT100_CODE_BG_BLUE "44"
#define VT100_CODE_BG_MAGENTA "45"
#define VT100_CODE_BG_CYAN "46"
#define VT100_CODE_BG_WHITE "47"
#define VT100_CODE_BG_DEFAULT "49"

#define VT100_CODE_BOLD "1"
#define VT100_CODE_DIM "2"
#define VT100_CODE_ITALIC "3"
#define VT100_CODE_UNDERLINE "4"
#define VT100_CODE_BLINKING "5"
#define VT100_CODE_INVERSE "7"
#define VT100_CODE_HIDDEN "8"
#define VT100_CODE_STRIKETHROUGH "9"

#define VT100_CODE_RESET_BOLD "22"
#define VT100_CODE_RESET_DIM "22"
#define VT100_CODE_RESET_ITALIC "23"
#define VT100_CODE_RESET_UNDERLINE "24"
#define VT100_CODE_RESET_BLINKING "25"
#define VT100_CODE_RESET_INVERSE "27"
#define VT100_CODE_RESET_HIDDEN "28"
#define VT100_CODE_RESET_STRIKETHROUGH "29"

// Reset all modes.
#define VT100_CODE_RESET "0"

// Shorthands for FG colors.
#define _VT100_FG(color) VT100_ESC(VT100_CODE_FG_##color)
#define VT100_BLACK _VT100_FG(BLACK)
#define VT100_RED _VT100_FG(RED)
#define VT100_GREEN _VT100_FG(GREEN)
#define VT100_YELLOW _VT100_FG(YELLOW)
#define VT100_BLUE _VT100_FG(BLUE)
#define VT100_MAGENTA _VT100_FG(MAGENTA)
#define VT100_CYAN _VT100_FG(CYAN)
#define VT100_WHITE _VT100_FG(WHITE)
#define VT100_DEFAULT _VT100_FG(DEFAULT)

// Shorthands for text styles
#define _VT100_STYLE(style) VT100_ESC(VT100_CODE_##style)
#define VT100_BOLD _VT100_STYLE(BOLD)
#define VT100_DIM _VT100_STYLE(DIM)
#define VT100_ITALIC _VT100_STYLE(ITALIC)
#define VT100_UNDERLINE _VT100_STYLE(UNDERLINE)
#define VT100_BLINKING _VT100_STYLE(BLINKING)
#define VT100_INVERSE _VT100_STYLE(INVERSE)
#define VT100_HIDDEN _VT100_STYLE(HIDDEN)
#define VT100_STRIKETHROUGH _VT100_STYLE(STRIKETHROUGH)

#define VT100_RESET VT100_ESC(VT100_CODE_RESET)

#endif // COMMON_VT100_H
