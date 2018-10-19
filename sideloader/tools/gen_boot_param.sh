#!/bin/bash

ADDR_LIST+="PML4_ADDR$"
ADDR_LIST+="|MPFP_ADDR$"
ADDR_LIST+="|APIC_ADDR$"
ADDR_LIST="|ADDR_VCON$"
ADDR_LIST+="|IPCS_ADDR$"
ADDR_LIST+="|TOTAL_COUNT$"
ADDR_LIST+="|KERNEL32_COUNT$"
ADDR_LIST+="|KERNEL64_COUNT$"
ADDR_LIST+="|APP_COUNT$"

echo "#ifndef __BOOT_PARAM_H__"
echo "#define __BOOT_PARAM_H__"
echo ""
nm $1 |egrep $ADDR_LIST | awk '{printf("#define %18s_OFFSET\t0x%s\n", $3, $1)}'
echo ""
echo "#endif"
