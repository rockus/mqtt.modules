#!/bin/bash

HOST="broker"
NODE="node"
USER="mqtt_user"
PWD="mqtt_pwd"

PIN_SENSOR_1=22
PIN_SENSOR_2=27

while true; do
  SENSOR_1_JSON=`/usr/local/sbin/am2302.py $PIN_SENSOR_1`
  mosquitto_pub -h $HOST -u $USER -P $PWD -t $NODE/unten -m "$SENSOR_1_JSON"
  SENSOR_2_JSON=`/usr/local/sbin/am2302.py $PIN_SENSOR_2`
  mosquitto_pub -h $HOST -u $USER -P $PWD -t $NODE/oben -m "$SENSOR_2_JSON"

  sleep 30
done
