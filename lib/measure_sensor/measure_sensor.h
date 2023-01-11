#ifndef MEASURE_SENSOR_H
#define MEASURE_SENSOR_H
#include <stdio.h>
#include <Arduino.h>


void AHT10_Begin(unsigned char _AHT10_address);

unsigned long readSensor(boolean GetDataCmd,unsigned char AHT10_address);

float getTemperature(void);

float getHumidity(void);




#endif