#!/bin/bash
# Autor: Yarib Nevárez - yarib_007@hotmail.com

if [ "$1" != "" ]; then
    echo "Transfering devicetree to ZYNQ device @ $1 ... "
    scp devicetree.dtb root@$1:/media/BOOT/
else
    echo "ERROR: Provide IP address of ZYNQ device."
fi
