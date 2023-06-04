/**
 * Virtual memory allocator. WIP
 */
#ifndef MEM_VIRT_H
#define MEM_VIRT_H

#include <stddef.h>

void *virt_mem_map(void *phys_ptr, size_t pg);
void *virt_mem_map_noalloc(void *phys_ptr, size_t pg);

#endif // MEM_VIRT_H
