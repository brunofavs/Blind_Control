#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "measure_sensor.h"

#include <WiFi.h>
// #include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"


AsyncWebServer server(80);
IPAddress staticIP(192,168,1,110);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

const char* ssid     = "Vodafone-E65D21";
const char* password = "784p8kcKUb";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}
// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(17) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}


#define aht10_i2c_adress 0x38
#define aht10_init_adress 0b011100001
#define ah10_trigger_measure 0b10101100

int blinds_up_pin_input = 34;
int blinds_down_pin_input = 27;

int blinds_up_pin_output = 17;
int blinds_down_pin_output = 16;



float temperature = 0;
float humidity = 0;

void relayModuleControl();

void setup() {

  Serial.begin(9600);

  pinMode(blinds_up_pin_input,INPUT);
  pinMode(blinds_down_pin_input,INPUT);


  pinMode(blinds_up_pin_output,OUTPUT);
  pinMode(blinds_down_pin_output,OUTPUT);

  digitalWrite(blinds_up_pin_output,HIGH);
  digitalWrite(blinds_down_pin_output,HIGH);

  //* Default pins for IC2 on ESP32 are 21 SDA 22 SCL
  // Wire.begin(); // Without specifying SDA,SCL (in this order) it assumes default pins
  // We can also specify just a adress if we wanted to have this uC as a slave
  
  Wire.begin();
  AHT10_Begin(aht10_i2c_adress);
  
  // Connect to WiFi
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Printing IP address
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}


void loop() {

  temperature = getTemperature();
  humidity = getHumidity();
  relayModuleControl();


  // Serial.printf("Temperature is \t\t%f\n",temperature);
  // Serial.printf("Relative humidity is \t%f%% \n",humidity);
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
}



