#!/bin/bash
# Autor: Yarib Nevárez - yarib_007@hotmail.com

if [ "$1" != "" ]; then
    echo "Transfering kernel module to ZYNQ device @ $1 ... "
    echo " ___ ADC ___ "
    echo "/etc/controller-manager/pwm_0"
    scp adc.ko root@$1:/etc/controller-manager/adc
else
    echo "ERROR: Provide IP address of ZYNQ device."
fi
