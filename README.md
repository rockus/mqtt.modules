initial creation
----------------
`apt install libconfig-dev libmosquitto-dev git automake make gcc`

`git clone https://github.com/WiringPi/WiringPi`
`cd WiringPi`
`./build`
`cd ../`

`git clone https://github.com/rockus/mqtt.modul`
`cd mqtt.modules`
`aclocal && automake --add-missing && autoconf && ./configure && make`

raspi_pulsecount_mqtt
========
This tool senses S0 counter pulses and sends these pulses to an [emonCMS] (http://emoncms.org/) host. Additionally,
energy used since the last pulse is sent.
This is a command line tool. 
It should be run from `/etc/rc.local` (as it needs to monitor the GPIO pin activity) as such:
`/usr/local/sbin/raspi_pulsecount_mqtt -c /etc/raspimqtt.conf >/dev/null &`

* **BananaPi**: not yet tried
* **Raspi**: works

* Connect S0+ to GPIO 4 (P1:07).
* Connect S0- to GND (P1:09).
* No external components required, GPIO pull-ups are enabled.

* Prerequisites: libconfig9, libconfig-dev, [wiringPi] (https://github.com/WiringPi/WiringPi/)

bme280_mqtt
========
This tool interfaces to a BME280 temperature/humidity/pressure sensor connected to I2C bus 2.
This is a command line tool.
It should be run as a cronjob from `/etc/crontab` or `/etc/cron.hourly/`.

* **BananaPi**: works
* **Raspi**: if `make` fails at the end of the steps above with something like `undefined reference to symbol 'pow@@GLIBC_2.29'`, then `cd bme280` and manually compile with `gcc -o bme280_mqtt bme280_mqtt.c gatherData.c -lconfig -lmosquitto -lwiringPi -lm`
