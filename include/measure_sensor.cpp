#include "measure_sensor.h"

#define aht10_i2c_adress 0x38
#define aht10_init_adress 0b011100001
#define ah10_trigger_measure 0b10101100

unsigned char eSensorCalibrateCmd[3] = {0xE1, 0x08, 0x00};
unsigned char eSensorNormalCmd[3]    = {0xA8, 0x00, 0x00};
unsigned char eSensorMeasureCmd[3]   = {0xAC, 0x33, 0x00};
unsigned char eSensorResetCmd        = 0xBA;

boolean    GetRHumidityCmd        = true;
boolean    GetTempCmd             = false;

