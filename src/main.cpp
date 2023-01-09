#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <AHT10.h>

#define aht10_i2c_adress 0x38
#define aht10_init_adress 0b011100001
#define ah10_trigger_measure 0b10101100

unsigned char eSensorCalibrateCmd[3] = {0xE1, 0x08, 0x00};
unsigned char eSensorNormalCmd[3]    = {0xA8, 0x00, 0x00};
unsigned char eSensorMeasureCmd[3]   = {0xAC, 0x33, 0x00};
unsigned char eSensorResetCmd        = 0xBA;

boolean    GetRHumidityCmd        = true;
boolean    GetTempCmd             = false;

int blinds_up_pin_input = 34;
int blinds_down_pin_input = 27;

int blinds_up_pin_output = 17;
int blinds_down_pin_output = 16;

void Read_I2C(unsigned char I2Caddress, unsigned char memmory_adress);

void relayModuleControl();


void AHT10_Begin(unsigned char _AHT10_address);
unsigned long readSensor(boolean GetDataCmd,unsigned char _AHT10_address);
float getTemperature(void);
float getHumidity(void);


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

void AHT10_Begin(unsigned char _AHT10_address)
{
    unsigned char AHT10_address = _AHT10_address;
    
    
    Wire.beginTransmission(AHT10_address);
    Wire.write(eSensorCalibrateCmd, 3);
    Wire.endTransmission();

    delay(500);
}

unsigned long readSensor(boolean GetDataCmd,unsigned char _AHT10_address)
{
    unsigned long result, temp[6];

    Wire.beginTransmission(_AHT10_address);
    Wire.write(eSensorMeasureCmd, 3);
    Wire.endTransmission();
    delay(100);

    Wire.requestFrom(_AHT10_address, 6);

    for(unsigned char i = 0; Wire.available() > 0; i++)
    {
        temp[i] = Wire.read();
    }   

    if(GetDataCmd)
    {
      // Its 8+8+4 bits for each measure, 0x0F its 1111

        result = ((temp[1] << 16) | (temp[2] << 8) | temp[3]) >> 4;
    }
    else
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




void Read_I2C(unsigned char I2Caddress, unsigned char memmory_adress){

  Wire.beginTransmission(I2Caddress);

  Wire.write(byte(memmory_adress));
  
  Wire.write(byte(ah10_trigger_measure));
  delay(100);
  Wire.write(byte(0b00110011));
  delay(100);
  Wire.write(byte(0b00000000));

  Wire.endTransmission(); // Needs to be terminated in order to switch the direction of the ports for reading instead of writting

  Wire.requestFrom(I2Caddress,6);

  char c;
  int switch_counter = 0;
  // Used in formulas
  int num_of_bytes_till_switch = 3;

  int temp_bytes[3];
  int rh_bytes[3];


  Serial.printf("Reading new measurement\n");
  while(Wire.available()){
    c = Wire.read();

    if(switch_counter<num_of_bytes_till_switch){
      // Serial.printf("Humidity byte value is %d\n",c);
      rh_bytes[switch_counter] = c;

      // s_rh = s_rh | (c<<(switch_measure*8));
      // printf("%d\n",(c<<(switch_measure*8)));
    }
    else{
      // Serial.printf("Temperature byte value is %d\n",c);
      temp_bytes[switch_counter-num_of_bytes_till_switch] = c;

      // s_t = s_t | (c<<((switch_measure-num_of_bytes_till_switch)*8));
      // printf("%d\n",(c<<((switch_measure-num_of_bytes_till_switch)*8)));
    }
      delay(200);
    switch_counter++;
  }  

    int b1 = temp_bytes[0];
    int b2 = temp_bytes[1];
    int b3 = temp_bytes[2];
  
    long int s_t = (b1 << 16) | (b2 << 8) | b3;

    int c1 = rh_bytes[0];
    int c2 = rh_bytes[1];
    int c3 = rh_bytes[2];
  
    long int s_rh = (c1 << 16) | (c2 << 8) | c3;

  long int temperature = ((s_t)/pow(2,20))*200-50;
  long int relative_humidity = ((s_rh)/pow(2,20))*100;

  Serial.printf("RH bytes are %ld %ld %ld\n",b1,b2,b3);
  Serial.printf("Temp bytes are %ld %ld %ld\n",c1,c2,c3);

  Serial.printf("\nS_t is %ld\n",s_t);
  Serial.printf("S_rh is %ld\n",s_rh);

  Serial.printf("\n\n\nRelative humidity is %ld\n",relative_humidity);
  Serial.printf("Temperature is %ld\n",temperature);

}