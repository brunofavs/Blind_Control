#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "measure_sensor.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>


#include <time.h>
#include <Timezone.h>

#define aht10_i2c_adress 0x38
#define aht10_init_adress 0b011100001
#define ah10_trigger_measure 0b10101100

int blinds_up_pin_input = 34;
int blinds_down_pin_input = 27;

int blinds_up_pin_output = 17;
int blinds_down_pin_output = 16;

bool cmd_web_input_up   = false;
bool cmd_web_input_down   = false;
bool cmd_web_auto_mode = false;

bool up_global_cmd   = false;
bool down_global_cmd = false; 

bool down_auto_cmd = false;
bool up_auto_cmd = false;


int web_input_up = 33;
int web_input_down = 32;
int web_auto_mode = 13;

float temperature = 0;
float humidity = 0;

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
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  %BUTTONPLACEHOLDER%
<script>
  function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
    else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
    xhr.send();
  }
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 10000 ) ;

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
    buttons += "<h4>Ordem de abertura</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Ordem de fecho</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"32\" " + outputState(32) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Modo automatico consoante temperatura</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"13\" " + outputState(13) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  if(var == "TEMPERATURE"){
    return String(temperature);
  }
  return String();
}



void relayModuleControl();

void setup() {

  Serial.begin(9600);
  

  pinMode(blinds_up_pin_input,INPUT);
  pinMode(blinds_down_pin_input,INPUT);


  pinMode(blinds_up_pin_output,OUTPUT);
  pinMode(blinds_down_pin_output,OUTPUT);

  // Its output because the web turns the pin on
  pinMode(web_auto_mode,OUTPUT);
  pinMode(web_input_down,OUTPUT);
  pinMode(web_input_up,OUTPUT);

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

  // Syncronzied to some time online
  configTime(3 * 3600, 0, "pool.ntp.org");

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
      digitalWrite(inputMessage1.toInt(), !inputMessage2.toInt());

      // Serial.println(inputMessage1.toInt());

      if(inputMessage1.toInt() == web_auto_mode){
        cmd_web_auto_mode = !cmd_web_auto_mode;
      }
      else if(inputMessage1.toInt() == web_input_down){
        cmd_web_input_down = !cmd_web_input_down;
      }
      else if(inputMessage1.toInt() == web_input_up){
        cmd_web_input_up = !cmd_web_input_up;
      }
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    // Serial.print("GPIO: ");
    // Serial.print(inputMessage1);
    // Serial.print(" - Set to: ");
    // Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(temperature));
  });
  // Start server
  server.begin();
}


void loop() {

  temperature = getTemperature();
  humidity = getHumidity();
  relayModuleControl();


  // Serial.println("");
  // Serial.println("");
  // Serial.println("");
  // Serial.print("Auto mode is ");
  // Serial.println(cmd_web_auto_mode);
  // Serial.print("Web up cmd is ");
  // Serial.println(cmd_web_input_up);
  // Serial.print("Web down cmd is ");
  // Serial.println(cmd_web_input_down);

  Serial.printf("Temperature is \t\t%f\n",temperature);
  // Serial.printf("Relative humidity is \t%f%% \n",humidity);
  delay(1000);

}



void relayModuleControl(){

  int up_manual_cmd = digitalRead(blinds_up_pin_input);
  int down_manual_cmd = digitalRead(blinds_down_pin_input);
  
  if(temperature > 25 && cmd_web_auto_mode == true){
  // if(temperature > 25){
    down_auto_cmd = true;
    up_auto_cmd = false;  // Se tiver calor fecha o estoro
  }else if(temperature <25 && cmd_web_auto_mode == true){
  // }else if(temperature <25){
    down_auto_cmd = false ;
    up_auto_cmd = true ;  // Se tiver frio abre o estoro para bater o sol
  }else{
    down_auto_cmd = false ;
    up_auto_cmd = false ;  // Se o modo auto desativar e a temperatura for alta, nunca mais desativa o cmd down auto
  }

  up_global_cmd   = (up_manual_cmd || cmd_web_input_up ||up_auto_cmd);
  
  down_global_cmd = (!up_global_cmd) && (down_manual_cmd || cmd_web_input_down ||down_auto_cmd);   



  Serial.printf("\n\n\nUp global cmd is %d\n",up_global_cmd);

  Serial.printf("\t\tWeb auto mode cmd is %d\n",cmd_web_auto_mode);
  Serial.printf("\t\tWeb up cmd is %d\n",cmd_web_input_up);
  Serial.printf("\t\tManual up cmd is %d\n",up_manual_cmd);
  Serial.printf("\t\tAuto up cmd is %d\n",up_auto_cmd);




  Serial.printf("Down global cmd is %d\n",down_global_cmd);

  Serial.printf("\t\tWeb auto mode cmd is %d\n",cmd_web_auto_mode);
  Serial.printf("\t\tWeb down cmd is %d\n",cmd_web_input_down);
  Serial.printf("\t\tManual down cmd is %d\n",down_manual_cmd);
  Serial.printf("\t\tAuto down cmd is %d\n",down_auto_cmd);


  if(up_global_cmd == HIGH){
    digitalWrite(blinds_up_pin_output,LOW);
    digitalWrite(blinds_down_pin_output,HIGH);
  }
  else if(down_global_cmd == HIGH){
    digitalWrite(blinds_up_pin_output,HIGH);
    digitalWrite(blinds_down_pin_output,LOW);
  }
  else if(up_global_cmd == LOW && down_global_cmd == LOW){
    digitalWrite(blinds_up_pin_output,HIGH);
    digitalWrite(blinds_down_pin_output,HIGH);
  }

  // Serial.printf("Up status is %d\n",up_manual_cmd);
  // Serial.printf("Down status is %d\n",down_manual_cmd);
}



