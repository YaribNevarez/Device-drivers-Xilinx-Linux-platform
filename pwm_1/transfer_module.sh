#!/bin/bash
# Autor: Yarib Nevárez - yarib_007@hotmail.com

if [ "$1" != "" ]; then
    echo "Transfering kernel module to ZYNQ device @ $1 ... "
    echo " ___ PWM_1 ___ "
    echo "/etc/controller-manager/pwm_1"
    scp pwm_1.ko root@$1:/etc/controller-manager/pwm_1
else
    echo "ERROR: Provide IP address of ZYNQ device."
fi
