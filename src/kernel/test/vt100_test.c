#include "common/vt100.h"

#include "test/test.h"

DEFINE_TEST(vt100, colors) {
  TEST_ASSERT(!strcmp(VT100_ESC(VT100_CODE_FG_RED),
                      VT100_SEQ_START VT100_CODE_FG_RED VT100_SEQ_END));
  TEST_ASSERT(!strcmp(VT100_ESC2(VT100_CODE_FG_RED, VT100_CODE_FG_GREEN),
                      VT100_SEQ_START VT100_CODE_FG_RED VT100_SEQ_SEP
                          VT100_CODE_FG_GREEN VT100_SEQ_END));
}
