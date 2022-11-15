# http://www.gnu.org/software/make/manual/make.html
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# http://mij.oltrelinux.com/devel/autoconf-automake/
# http://inti.sourceforge.net/tutorial/libinti/autotoolsproject.html

#######

AM_LDFLAGS = -lconfig -L /usr/local/lib -lwiringPi -lm -lmosquitto

sbin_PROGRAMS = bme280_mqtt

bme280_mqtt_SOURCES = bme280_mqtt.c gatherData.c
