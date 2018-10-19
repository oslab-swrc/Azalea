#!/bin/bash
hexdump -e '1/1 "0x" "%04_ax) "' -e '4/4 "%08X " "\n"' $1
