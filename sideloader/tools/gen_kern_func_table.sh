#!/bin/bash

FUNC_LIST="memblock_reserve$"
FUNC_LIST+="|memblock_find_in_range$"
FUNC_LIST+="|memblock_free$"
SYSMAP=$1

echo "#ifndef __KERN_FUNC_TABLE_H__"
echo "#define __KERN_FUNC_TABLE_H__"
echo ""
egrep $FUNC_LIST $SYSMAP|awk '{printf("%s%s\t0x%s\n","#define KERN_FUNC_", toupper($3),$1)}'|sort
echo ""
echo "#endif /* __KERN_FUNC_TABLE_H__ */"

