#!/bin/bash
# Autor: Yarib Nevárez - yarib_007@hotmail.com

if [ "$1" != "" ]; then
    echo "Transfering kernel module to ZYNQ device @ $1 ... "
    echo " ___ CONTROLLER ___ "
    echo "/etc/controller-manager/controller"
    scp controller.ko root@$1:/etc/controller-manager/controller
else
    echo "ERROR: Provide IP address of ZYNQ device."
fi
