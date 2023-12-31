/**
 * Diagnostics to print out system information.
 */
#ifndef DIAG_SYS_H
#define DIAG_SYS_H

#include "limine.h"

void print_limine_mmap(struct limine_memmap_response *);
void run_printf_tests(void);
void print_gdtr_info(void);
void print_idtr_info(void);

#endif
