#!/bin/bash

HOST="broker"
NODE="node"
USER="mqtt_user"
PWD="mqtt_pwd"

# declare variables as numbers
declare -i GARMIN18_nSATS GARMIN18_uSATS GARMIN18_MODE
declare -i M8N_nSATS M8N uSATS M8N_MODE

while true; do
  NTP_DRIFT=`cat /var/lib/ntp/ntp.drift`
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/ntp_drift/config" -m '{"name": "timeserver_ntp_drift", "device_class": "frequency", "unit_of_measurement": "ppm", "state_topic": "homeassistant/sensor/timeserver/ntp_drift/state"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/ntp_drift/state" -m $NTP_DRIFT

  GARMIN18=(`ntpq -p | grep G18P | awk '{print $9, $10}'`)
  GARMIN18_PPS_OFFSET=${GARMIN18[0]}
  GARMIN18_PPS_JITTER=${GARMIN18[1]}
  GARMIN18_PAYLOAD=\''{"jitter": "'$GARMIN18_PPS_JITTER'", "offset": "'$GARMIN18_PPS_OFFSET'"}'\'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_pps_jitter/config" -m '{"name": "timeserver_g18_pps_jitter", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/G18_pps_jitter/state", "value_template": "{{value_json.jitter}}"'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_pps_offset/config" -m '{"name": "timeserver_g18_pps_offset", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/G18_pps_jitter/state", "value_template": "{{value_json.offset}}"'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/G18_pps_jitter/state" -m "{\"jitter\":$GARMIN18_PPS_JITTER,\"offset\":$GARMIN18_PPS_OFFSET}"

  head -20 /dev/serial0 >/tmp/serial0.msg
  GARMIN18_nSATS=`grep -m 1 GPGSV /tmp/serial0.msg | awk -F ',' '{print $4}'`
  GARMIN18_uSATS=`grep -m 1 GPGGA /tmp/serial0.msg | awk -F ',' '{print $8}'`
  GARMIN18_MODE=`grep -m 1 GPGSA /tmp/serial0.msg | awk -F ',' '{print $3}'`
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_stats_sats_seen/config" -m '{"name": "timeserver_g18_sats_seen", "state_topic": "homeassistant/sensor/timeserver/G18_stats/state", "value_template": "{{value_json.sats_seen}}"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_stats_sats_used/config" -m '{"name": "timeserver_g18_sats_used", "state_topic": "homeassistant/sensor/timeserver/G18_stats/state", "value_template": "{{value_json.sats_used}}"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/G18_stats_mode/config" -m '{"name": "timeserver_g18_mode", "state_topic": "homeassistant/sensor/timeserver/G18_stats/state", "value_template": "{{value_json.mode}}"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/G18_stats/state" -m "{\"sats_seen\":$GARMIN18_nSATS,\"sats_used\":$GARMIN18_uSATS,\"mode\":$GARMIN18_MODE}"


  gpspipe -n 7 -w >/tmp/spidev0.0.msg
  M8N_nSATS=`grep SKY /tmp/spidev0.0.msg | grep -m 1 satellites | awk -F ',' '{print $10}' | awk -F ':' '{print $2}'`
  M8N_uSATS=`grep SKY /tmp/spidev0.0.msg | grep -m 1 satellites | awk -F ',' '{print $11}' | awk -F ':' '{print $2}'`
  M8N_MODE=`grep -m 1 TPV /tmp/spidev0.0.msg | awk -F ',' '{print $4}' | awk -F ':' '{print $2}'`
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/M8N_stats_sats_seen/config" -m '{"name": "timeserver_m8n_sats_seen", "state_topic": "homeassistant/sensor/timeserver/M8N_stats/state", "value_template": "{{value_json.sats_seen}}"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/M8N_stats_sats_used/config" -m '{"name": "timeserver_m8n_sats_used", "state_topic": "homeassistant/sensor/timeserver/M8N_stats/state", "value_template": "{{value_json.sats_used}}"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/M8N_stats_mode/config" -m '{"name": "timeserver_m8n_mode", "state_topic": "homeassistant/sensor/timeserver/M8N_stats/state", "value_template": "{{value_json.mode}}"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/M8N_stats/state" -m "{\"sats_seen\":$M8N_nSATS,\"sats_used\":$M8N_uSATS,\"mode\":$M8N_MODE}"


  M8N_NMEA_JITTER=`ntpq -p | grep M8N | awk '{print $10}'`
  mosquitto_pub -h $HOST -u $USER -P $PWD -r -t "homeassistant/sensor/timeserver/M8N_nmea_jitter/config" -m '{"name": "timeserver_m8n_nmea_jitter", "device_class": "frequency", "unit_of_measurement": "ms", "state_topic": "homeassistant/sensor/timeserver/M8N_nmea_jitter/state"}'
  mosquitto_pub -h $HOST -u $USER -P $PWD -t "homeassistant/sensor/timeserver/M8N_nmea_jitter/state" -m $M8N_NMEA_JITTER

  sleep 30
done
