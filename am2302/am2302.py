#!/usr/bin/python3

# https://pimylifeup.com/raspberry-pi-humidity-sensor-dht22/

import sys
import Adafruit_DHT
import json

DHT_SENSOR = Adafruit_DHT.AM2302

if __name__ == "__main__":
    argc = len(sys.argv)
    if argc != 2:
        print(f"Arguments count: {argc}")
        print("Not exactly one argument given. First argument needs to be AM2302 pin, wiringpi numbering.")
        exit(1)

    DHT_PIN = sys.argv[1]
    humidity, temperature = Adafruit_DHT.read_retry(DHT_SENSOR, DHT_PIN)

    if humidity is not None and temperature is not None:
        print("{{\"temperature\": {0:0.2f}, \"humidity\": {1:0.2f}}}".format(temperature, humidity))
        exit (0)
    else:
        print("Failed to retrieve data from humidity sensor")
        exit (1)
