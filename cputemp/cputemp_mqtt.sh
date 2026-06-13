#!/bin/bash

HOST="broker"
NODE="node"
TOPIC="homeassistant/sensor"
USER="mqtt_user"
PWD="mqtt_pwd"

while true; do
  CPU_TEMP0=`cat /sys/class/thermal/thermal_zone0/temp`

  CPU_TEMP0_INT=$(($CPU_TEMP0 / 1000))
  CPU_TEMP0_REM=$(($CPU_TEMP0 % 1000))
  CPU_TEMP0=$CPU_TEMP0_INT.$CPU_TEMP0_REM

  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t $TOPIC/$NODE/cpu_temp/config -m '{"name": "'$NODE'_cpu_temp", "device_class": "temperature", "state_topic": "'$TOPIC'/'$NODE'/cpu_temp/state"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t $TOPIC/$NODE/cpu_temp/state -m $CPU_TEMP0

  sleep 60
done
