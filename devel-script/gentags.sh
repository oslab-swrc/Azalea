#!/bin/sh
rm -f TAGS*
find kernel64 -name "*.[ch]" -exec etags -a {} \;
find kernel64 -name "*.asm" -exec etags -a {} \;
find kernel64 -name "*.s" -exec etags -a {} \;

find kernel32 -name "*.[ch]" -exec etags -a {} \;
find kernel32 -name "*.asm" -exec etags -a {} \;
find kernel32 -name "*.s" -exec etags -a {} \;

find bootloader -name "*.[ch]" -exec etags -a {} \;
find bootloader -name "*.asm" -exec etags -a {} \;
find bootloader -name "*.s" -exec etags -a {} \;
