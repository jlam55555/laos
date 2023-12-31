#include "common/libc.h"    // for printf
#include "common/opcodes.h" // for op_hlt
#include "drivers/acpi.h"   // for acpi_shutdown

// Required for assert.h.
void __assert_fail(const char *assertion, const char *file, unsigned int line,
                   const char *function) {
  printf("%s:%u:%s(): assert(%s) failed\r\n", file, line, function, assertion);

#ifdef RUNTEST
  acpi_shutdown();
#else
  for (;;) {
    op_hlt();
  }
#endif // RUNTEST
}
