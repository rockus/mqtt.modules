#!/bin/bash

HOST="broker"
NODE="node"
USER="mqtt_user"
PWD="mqtt_pwd"

while true; do
  NTP_DRIFT=`cat /var/lib/ntp/ntp.drift`

  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/ntp_drift/config" -m '{"name": "timeserver_ntp_drift", "device_class": "frequency", "unit_of_measurement": "ppm", "state_topic": "homeassistant/sensor/timeserver/ntp_drift/state"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/ntp_drift/state" -m $NTP_DRIFT

  sleep 300
done
