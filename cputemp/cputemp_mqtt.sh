#!/bin/bash

HOST="broker"
NODE="node"
USER="mqtt_user"
PWD="mqtt_pwd"

while true; do
  CPU_TEMP0=`cat /sys/class/thermal/thermal_zone0/temp`

  CPU_TEMP0_INT=$(($CPU_TEMP0 / 1000))
  CPU_TEMP0_REM=$(($CPU_TEMP0 % 1000))
  CPU_TEMP0=$CPU_TEMP0_INT.$CPU_TEMP0_REM

  mosquitto_pub -h $HOST -u $USER -P $PWD -t $NODE/cpu_temp -m $CPU_TEMP0

  sleep 60
done
