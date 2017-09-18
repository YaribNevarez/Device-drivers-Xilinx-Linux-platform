#!/bin/bash
# Autor: Yarib Nevárez - yarib_007@hotmail.com

if [ "$1" != "" ]; then
    echo "Transfering kernel module to ZYNQ device @ $1 ... "
    echo " ___ PWM_0 ___ "
    echo "/etc/controller-manager/pwm_0"
    scp pwm_0.ko root@$1:/etc/controller-manager/pwm_0
else
    echo "ERROR: Provide IP address of ZYNQ device."
fi
