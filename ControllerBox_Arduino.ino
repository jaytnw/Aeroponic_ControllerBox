#include <AuthClient.h>
#include <MicroGear.h>
#include <MQTTClient.h>
#include <SHA1.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <DHT.h>


#include <Wire.h>
#include <RtcDS3231.h>

RtcDS3231<TwoWire> rtcObject(Wire);
#define DHTPIN D3
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
const char* ssid     = "Test2";
const char* password = "123456799";

#define APPID       "Aeroponics"
#define GEARKEY     "UNjM7YDlpbaqrhC"
#define GEARSECRET  "Rf1hyCtXeVhRTaoAXvKtcALSb"
#define SCOPE       ""
#define ALIAS       "AeroGear"

WiFiClient client;
AuthClient *authclient;

int pin_misk = D7;
int pin_led = D8;
int HourOn = 0;
int MinOn = 0;
int HourOff = 0;
int MinOff = 0;
int eve_misk = 30 ;

int pin_water_pump = D0;
int pin_pui_pump = D5;
int A2 = D4;
int B2 = D6;

float phStart, phStop;

int ph_pin = A0;


int cLED = 0;
int cMisk = 0;

MicroGear microgear(client);

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("Incoming message --> ");
  Serial.print(topic);
  Serial.print(" : ");
  char strState[msglen];
  for (int i = 0; i < msglen; i++) {
    strState[i] = (char)msg[i];
    Serial.print((char)msg[i]);
  }
  Serial.println();

  String stateStr = String(strState).substring(0, msglen);

  if (stateStr == "OnLight") {
    digitalWrite(pin_led, LOW);
    cLED = 1;
  } else if (stateStr == "OffLight") {
    digitalWrite(pin_led, HIGH);
    cLED = 2;
    microgear.chat("controllerplug", "OFF");
  }
  if (stateStr == "OnMisk") {
    digitalWrite(pin_misk, LOW);
    cMisk = 1;
  } else if (stateStr == "OffMisk") {
    digitalWrite(pin_misk, HIGH);
    cMisk = 2;

  }
  if (stateStr != "OnLight" && stateStr != "OffLight" && stateStr != "OnMisk" && stateStr != "OffMisk"   ) {
    String message  = stateStr;    //serial message comes in like this 10,A3,3F

    HourOn = message.substring(0, 2).toInt();
    MinOn = message.substring(2, 4).toInt();
    HourOff = message.substring(4, 6).toInt();
    MinOff = message.substring(6, 8).toInt();
    eve_misk = message.substring(8, 10).toInt();
    phStart = message.substring(10, 13).toFloat();
    phStop  = message.substring(13, 16).toFloat();

  }
  Serial.print("String ----->>");
  Serial.println(stateStr);
  Serial.print("HourOn ------->>");
  Serial.println(HourOn);
  Serial.print("MinOn ------->>");
  Serial.println(MinOn);
  Serial.print("HourOff ------->>");
  Serial.println(HourOff);
  Serial.print("MinOff ------->>");
  Serial.print(MinOff);
  Serial.print("Eve Time ------->>");
  Serial.println(eve_misk);
  Serial.print("Ph Start ------->>");
  Serial.println(phStart);
  Serial.print("Ph Stop ------->>");
  Serial.print(phStop);


}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setName(ALIAS);
}


//--------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  Serial.println("Starting...");

  microgear.on(MESSAGE, onMsghandler);
  microgear.on(CONNECTED, onConnected);

  dht.begin();
  pinMode(pin_led, OUTPUT);
  pinMode(pin_misk, OUTPUT);
  digitalWrite(pin_led, HIGH);
  digitalWrite(pin_misk, HIGH);

  pinMode(pin_water_pump, OUTPUT);
  pinMode(pin_pui_pump, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(B2, OUTPUT);

  rtcObject.Begin();     //Starts I2C

  RtcDateTime currentTime = RtcDateTime(__DATE__, __TIME__);
  rtcObject.SetDateTime(currentTime);


  if (WiFi.begin(ssid, password)) {

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    microgear.resetToken();
    microgear.init(GEARKEY, GEARSECRET, SCOPE);
    microgear.connect(APPID);
  }
}

//--------------------------------------------------------------------------------------------------------------------------------------------

void loop() {
  RtcDateTime now = rtcObject.GetDateTime();

  if (microgear.connected()) {
    microgear.loop();
    Serial.print("Sending -- >\n");

  } else {
    Serial.println("connection lost, reconnect...");
    microgear.connect(APPID);
  }
  delay(1000);


  check_time_misk(eve_misk);

  if (now.Hour() >= HourOn && now.Minute() >= MinOn  ) {
    digitalWrite(pin_led, LOW);
  }

  if ( now.Hour() >= HourOff && now.Minute() >=  MinOff) {
    digitalWrite(pin_led, HIGH);
  }

  //  else {
  //    digitalWrite(pin_led, HIGH);
  //  }

  int sensorValue = analogRead(ph_pin);

  float voltage = (sensorValue * (5.0 / 1023.0)) + 3.9;
  Serial.println("Ph ---> ");
  Serial.println(voltage);


  if (voltage < phStart) {
    digitalWrite(pin_pui_pump, HIGH);
    digitalWrite(A2, LOW);
    digitalWrite(pin_water_pump, LOW);
    digitalWrite(B2, LOW);

  } else  if (voltage > phStop) {
    digitalWrite(pin_water_pump, HIGH);
    digitalWrite(B2, LOW);
    digitalWrite(pin_pui_pump, LOW);
    digitalWrite(A2, LOW);
  } else if (voltage >= phStart && voltage <= phStop) {
    digitalWrite(pin_pui_pump, LOW);
    digitalWrite(A2, LOW);
    digitalWrite(pin_water_pump, LOW);
    digitalWrite(B2, LOW);
  }

  Serial.print(now.Year(), DEC);
  Serial.print('/');
  Serial.print(now.Month(), DEC);
  Serial.print('/');
  Serial.print(now.Day(), DEC);
  Serial.print(' ');
  Serial.print(now.Hour(), DEC);
  Serial.print(':');
  Serial.print(now.Minute(), DEC);
  Serial.print(':');
  Serial.print(now.Second(), DEC);
  Serial.println();


  float h = dht.readHumidity();
  float t = dht.readTemperature();


  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");

  microgear.publish("/temp", t, 0, true);
  microgear.publish("/hum", h, 0, true);
  microgear.publish("/ph", voltage, 1, true);

  Serial.print("HourOn = ");
  Serial.println(HourOn);
  Serial.print("MinOn ");
  Serial.println(MinOn);
  Serial.print("HourOff  ");
  Serial.println(HourOff);
  Serial.print("MinOff  ");
  Serial.println(MinOff);
  Serial.print("Eve Time  ");
  Serial.println(eve_misk);
  Serial.print("Ph Start ->>  ");
  Serial.println(phStart);
  Serial.print("Ph Stop ->>  ");
  Serial.println(phStop);
  Serial.println();
  Serial.println("----------------------------------------------------------------------------------------------------------------------");
  delay(1000);





}


//--------------------------------------------------------------------------------------------------------------------------------------------

RtcDateTime now = rtcObject.GetDateTime();
int Min = now.Minute();
int time_check = 0;
int time_check_pre = 1;
int count = -1;

void check_time_misk(int times) {

  int timeReal = times * 60;

  time_check = Min ;
  time_check_pre = time_check - 1;
  if ( time_check != time_check_pre && count <= timeReal + 1) {
    count += 1;

  }

  if (timeReal > timeReal + 1) {
    count = 0;
  }

  if (count == 0) {
    digitalWrite(pin_misk, HIGH);
  } else if (count == timeReal) {
    digitalWrite(pin_misk, LOW);
  }
  Serial.print("Count ---- > ");
  Serial.println(count);
}
