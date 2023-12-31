#ifndef DRIVERS_ACPI_H
#define DRIVERS_ACPI_H

#include "common/opcodes.h" // for op_outw

/**
 * QEMU-specific shutdown.
 *
 * See https://wiki.osdev.org/Shutdown#Emulator-specific_methods.
 */
inline void acpi_shutdown() { op_outw(0x2000, 0x604); }

#endif // DRIVERS_ACPI_H
