#ifndef BME280_H
#define BME280_H

#include <stdint.h>	// for data types below

struct data
{
	uint16_t hum_raw;
	uint32_t temp_raw;
	uint32_t press_raw;
	float Humidity;
	float Temperature;
	float Pressure;
	float PressureReduced;
};

#endif
