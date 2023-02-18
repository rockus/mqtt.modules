#!/bin/bash

HOST="broker"
NODE="node"
USER="mqtt_user"
PWD="mqtt_pwd"

while true; do
  NTP_DRIFT=`cat /var/lib/ntp/ntp.drift`
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/ntp_drift/config" -m '{"name": "timeserver_ntp_drift", "device_class": "frequency", "unit_of_measurement": "ppm", "state_topic": "homeassistant/sensor/timeserver/ntp_drift/state"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/ntp_drift/state" -m $NTP_DRIFT

  GARMIN18=(`ntpq -p | grep G18P | awk '{print $9, $10}'`)
  GARMIN18_PPS_OFFSET=${GARMIN18[0]}
  GARMIN18_PPS_JITTER=${GARMIN18[1]}
  GARMIN18_PAYLOAD=\''{"jitter": "'$GARMIN18_PPS_JITTER'", "offset": "'$GARMIN18_PPS_OFFSET'"}'\'
#  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_pps_jitter/config" -m '{"name": "timeserver_g18_pps_jitter", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/G18_pps_jitter/state"}'
#  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/G18_pps_jitter/state" -m $GARMIN18_PPS_JITTER
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_pps_jitter/config" -m '{"name": "timeserver_g18_pps_jitter", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/G18_pps_jitter/state", "value_template": "{{value_json.jitter}}"'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_pps_offset/config" -m '{"name": "timeserver_g18_pps_offset", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/G18_pps_jitter/state", "value_template": "{{value_json.offset}}"'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/G18_pps_jitter/state" -m "{\"jitter\":$GARMIN18_PPS_JITTER,\"offset\":$GARMIN18_PPS_OFFSET}"

  M8N_NMEA_JITTER=`ntpq -p | grep M8N | awk '{print $10}'`
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/M8N_nmea_jitter/config" -m '{"name": "timeserver_m8n_nmea_jitter", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/M8N_nmea_jitter/state"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/M8N_nmea_jitter/state" -m $M8N_NMEA_JITTER

  sleep 30
done
