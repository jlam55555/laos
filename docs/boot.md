# Bootloader

The Limine bootloader is used because it is a well-documented, modern bootloader that supports the x86_64 platform.

A custom bootloader may be written in the future to better understand the boot process, but this is secondary to learning about the kernel.

### Questions
- **Higher-half kernel**: Why?
- **HHDM (Higher Half Direct Map)**: ???
- **Bootloader-reclaimable memory**: ???
- **Bootloader-set memory mappings**: Why these?
- **KASLR**: ???

### Vocabulary
- **GDT (Global Descriptor Table)**: 

### Resources
- [Inside the Linux boot process](https://developer.ibm.com/articles/l-linuxboot/)
- [Limine Protocol](https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md)
- [Limine Bare Bones tutorial](https://wiki.osdev.org/Limine_Bare_Bones)
- [Reasoning behind bootloader magic bytes](https://stackoverflow.com/a/1125062/)
