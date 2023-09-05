#ifndef DRIVERS_ACPI_H
#define DRIVERS_ACPI_H

#include "common/util.h"

// QEMU-specific shutdown.
inline void acpi_shutdown() { outw(0x2000, 0x604); }

#endif // DRIVERS_ACPI_H
