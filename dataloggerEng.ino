#define DSUSER
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiClientSecure.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ThingSpeak.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//BOTH LM35 - AND DS18B20 change this parameters to fit your setup:
#define red D6
#define green D7
#define blue D8
const char* host = "esp8266-webupdate";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";
unsigned long myChannelNumber = 413239;
const char * myWriteAPIKey = "Z0AO2JW22GLFDDMA";
//================================================================
#ifdef LM35USER
//LM35 USER RX, TX=======================
SoftwareSerial mySerial (D5, D4);
//=======================================
#endif
#ifdef DSUSER
//configuration for DS18B20=======================
#define ONE_WIRE_BUS D3
#define TEMPERATURE_PRECISION 12
#define delayDallas 15000
DeviceAddress blueSensor = { 0x28, 0xFF, 0x35, 0xD9, 0xB1, 0x17, 0x05, 0x26};
DeviceAddress redSensor = { 0x28, 0xFF, 0x0E, 0x25, 0xA3, 0x17, 0x04, 0xEA};
DeviceAddress greenSensor = {0x28, 0xFF, 0xC4, 0x92, 0xB2, 0x17, 0x04, 0x83 };
//================================================
#endif
//DO NOT CHANGE THESE!!==========================
float temperature;
unsigned long time1 = 0;
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient client;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int nDevices = 0; //number of temperature device
bool verdePlug = 0;
bool rossoPlug = 0;
bool bluPlug = 0;
long timeLast = 0, timeLastDallas = 0;
Ticker ticker;

//================================================

void tick1() {
  int state = digitalRead(red);
  digitalWrite(red, !state);
}
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  digitalWrite(blue, HIGH);
  ticker.attach(0.2, tick1);
}
void blinkFast (int pin, int rep) {
  for (int i = 0; i < rep; i++) {
    digitalWrite(pin, LOW);
    delay(60);
    digitalWrite(pin, HIGH);
    delay(60);
  }
}

void heartbreath() {
  bool state = digitalRead(blue);
  digitalWrite(blue, !state);
  digitalWrite(green, !state);
}
void fade (int pin) {
  for (int i = 1024; i > 0; i -= 5) {
    analogWrite(pin, i);
    delay(5);
  }
  for (int i = 0; i < 1024; i += 5) {
    analogWrite(pin, i);
    delay(5);
  }
  digitalWrite(pin, HIGH);
}
void checkSensors() {
  if ((sensors.isConnected(blueSensor)) && (bluPlug == 0)) {
    bluPlug = 1;
    Serial.print("Blue sensor found and working\n");
    fade(blue);
  }
  else if ((!sensors.isConnected(blueSensor)) && (bluPlug == 1)) {
    blinkFast(blue, 5);
    Serial.println("Blue sensor unplugged!");
    bluPlug = 0;
  }
  if ((sensors.isConnected(redSensor)) && (rossoPlug == 0)) {
    rossoPlug = 1;
    Serial.print("Red sensor found and workinge\n");
    fade(red);
  }
  else if ((!sensors.isConnected(redSensor)) && (rossoPlug == 1)) {
    blinkFast(red, 5);
    Serial.println("Red sensor unplugged!");
    rossoPlug = 0;
  }
  if ((sensors.isConnected(greenSensor)) && (verdePlug == 0)) {
    verdePlug = 1;
    fade(green);
    Serial.print("Green sensor found and working\n");
  }
  else if ((!sensors.isConnected(greenSensor)) && (verdePlug == 1)) {
    blinkFast(green, 5);
    Serial.println("Green sensor unplugged!");
    verdePlug = 0;
  }
}
//END =================

void setup(void) {

  Serial.begin(115200);
  Serial.println();
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
  digitalWrite(green, HIGH);
  digitalWrite(red, HIGH);
  digitalWrite(blue, HIGH);
  ticker.attach(0.6, tick1);
  Serial.println("Booting Sketch...");
#ifdef LM35USER
  mySerial.begin(9600);
#endif
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("AutoConnectAP");
  ticker.detach();
  digitalWrite(red, HIGH);
  digitalWrite(green, LOW);
  delay(200);
  digitalWrite(green, HIGH);
  delay(100);
#ifdef DSUSER
  Serial.println("Searching for Dallas temperature sensors...");
  sensors.begin();
  nDevices = sensors.getDeviceCount();
  Serial.printf ("Found %d sensors\n", nDevices);

  checkSensors();
  delay(100);
#endif
  digitalWrite(blue, LOW);
  digitalWrite(red, LOW);
  digitalWrite(green, LOW);

  MDNS.begin(host);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
  Serial.println("\nVersion v.1.0\n");
  ThingSpeak.begin(client);
  delay(300);
  digitalWrite(green, HIGH);
  digitalWrite(blue, HIGH);
  digitalWrite(red, HIGH);
}



void loop(void) {
  if (millis() - timeLastDallas > delayDallas) {
#ifdef DSUSER
    sensors.requestTemperatures();
    analogWrite(blue, 500);
    analogWrite(red, 500);
    if (verdePlug == 1) {
      float tempC = sensors.getTempC(greenSensor);
      ThingSpeak.setField(1, tempC);
      Serial.printf("\nGreen temperature sensor: %2.2f°C", tempC);
    }
    if (rossoPlug == 1) {
      float tempC = sensors.getTempC(redSensor);
      ThingSpeak.setField(2, tempC);
      Serial.printf("\nRed temperature sensor: %2.2f°C", tempC);
    }
    if (bluPlug == 1) {
      float tempC = sensors.getTempC(blueSensor);
      ThingSpeak.setField(3, tempC);
      Serial.printf("\nBlue temperature sensor: %2.2f\n°C", tempC);
    }
#endif
    timeLastDallas = millis();
#ifdef LM35USER
    ThingSpeak.setField(1, temperature);
    //Serial.printf("\n temperature letta da sensor analogico: %2.2f°C\n", temperature);
#endif
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    digitalWrite(blue, HIGH);
    digitalWrite(red , HIGH);
  }
#ifdef DSUSER
  checkSensors();
#endif
#ifdef LM35USER
  if (mySerial.available() > 0) {
    delay(10);
    char rec[6];
    while (mySerial.read() != 'a');
    rec[0] = mySerial.read();
    rec[1] = mySerial.read();
    rec[2] = mySerial.read();
    rec[3] = mySerial.read();
    temperature = ((rec[1] - 48) * 10) + (rec[2] - 48) + ((rec[3] - 48) * 0.1);
    if (rec[0] == '-') {
      temperature = -temperature;
    }
  }
#endif
  httpServer.handleClient();
}
