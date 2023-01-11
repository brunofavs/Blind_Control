#include "measure_sensor.h"
#include <Wire.h>
#include <math.h>

#define aht10_i2c_adress 0x38
#define aht10_init_adress 0b011100001
#define ah10_trigger_measure 0b10101100

unsigned char eSensorCalibrateCmd[3] = {0xE1, 0x08, 0x00};
unsigned char eSensorMeasureCmd[3]   = {0xAC, 0x33, 0x00};
// Not used
// unsigned char eSensorNormalCmd[3]    = {0xA8, 0x00, 0x00};
// unsigned char eSensorResetCmd        = 0xBA;

boolean    GetRHumidityCmd        = true;
boolean    GetTempCmd             = false;

void AHT10_Begin(unsigned char _AHT10_address)
{
    // This function just executes a calibration procedure, its not strictly necessary for the programm to work
    unsigned char AHT10_address = _AHT10_address;
    
    
    Wire.beginTransmission(AHT10_address);
    Wire.write(eSensorCalibrateCmd, 3);
    Wire.endTransmission();

    delay(500);
}

unsigned long readSensor(boolean GetDataCmd,unsigned char AHT10_address){
    unsigned long result, temp[6];

    // Puts AHT10 ready to send data
    Wire.beginTransmission(AHT10_address);
    Wire.write(eSensorMeasureCmd, 3);
    Wire.endTransmission();
    delay(100);

    // AHT10 message always has 6 bytes, first is status, next 2.5 are humidity, last 2.5 are temperature
    Wire.requestFrom(AHT10_address, 6);

    for(unsigned char i = 0; Wire.available() > 0; i++)
    {
        temp[i] = Wire.read();
    }   

    if(GetDataCmd) // If true, reads humidity
    {
      // Its 8+8+4 bits for each measure, 0x0F its 1111

        result = ((temp[1] << 16) | (temp[2] << 8) | temp[3]) >> 4;
    }
    else // If false reads temperature
    {
        result = ((temp[3] & 0x0F) << 16) | (temp[4] << 8) | temp[5];
    }

    return result;
}

float getTemperature(void)
{
    float value = readSensor(GetTempCmd,aht10_i2c_adress);
    return ((200 * value) / 1048576) - 50;
}

float getHumidity(void)
{
    float value = readSensor(GetRHumidityCmd,aht10_i2c_adress);
    if (value == 0) {
        return 0;      
    }
    return value * 100 / 1048576;
}
