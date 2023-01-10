#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "measure_sensor.h"

#define aht10_i2c_adress 0x38
#define aht10_init_adress 0b011100001
#define ah10_trigger_measure 0b10101100

int blinds_up_pin_input = 34;
int blinds_down_pin_input = 27;

int blinds_up_pin_output = 17;
int blinds_down_pin_output = 16;

void Read_I2C(unsigned char I2Caddress, unsigned char memmory_adress);

void relayModuleControl();

void setup() {

  //* Default pins for IC2 on ESP32 are 21 SDA 22 SCL
  // Wire.begin(); // Without specifying SDA,SCL (in this order) it assumes default pins
  // We can also specify just a adress if we wanted to have this uC as a slave
  
  Wire.begin();
  AHT10_Begin(aht10_i2c_adress);

  Serial.begin(9600);
  pinMode(blinds_up_pin_input,INPUT);
  pinMode(blinds_down_pin_input,INPUT);


  pinMode(blinds_up_pin_output,OUTPUT);
  pinMode(blinds_down_pin_output,OUTPUT);

  digitalWrite(blinds_up_pin_output,HIGH);
  digitalWrite(blinds_down_pin_output,HIGH);

}


void loop() {

  float temperature = getTemperature();
  float humidity = getHumidity();

  Serial.printf("Temperature is \t\t%f\n",temperature);
  Serial.printf("Relative humidity is \t%f%% \n",humidity);
  delay(1000);

}




void relayModuleControl(){

  int up_status = digitalRead(blinds_up_pin_input);
  int down_status = digitalRead(blinds_down_pin_input);


  if(up_status == HIGH){
    digitalWrite(blinds_up_pin_output,LOW);
    digitalWrite(blinds_down_pin_output,HIGH);
  }
  else if(down_status == HIGH){
    digitalWrite(blinds_up_pin_output,HIGH);
    digitalWrite(blinds_down_pin_output,LOW);
  }

  Serial.printf("Up status is %d\n",up_status);
  Serial.printf("Down status is %d\n",down_status);
  delay(100);
}




