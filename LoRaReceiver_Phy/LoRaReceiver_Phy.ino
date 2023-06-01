#include <SPI.h>
#include <LoRa.h>
#include <ESP8266WiFi.h>

#define WIFI_SSID     "LoRaGhostRider"
#define WIFI_PASS     "00112233445"
// API Thinkspeak
String apiKey = "DV2R90R30K64GNKI";
char* server = "api.thingspeak.com";
// Parameters you can play with :

int txPower = 14; // from 0 to 20, default is 14
int spreadingFactor = 12; // from 7 to 12, default is 12
long signalBandwidth = 125E3; // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3,41.7E3,62.5E3,125E3,250E3,500e3, default is 125E3
int codingRateDenominator=5; // Numerator is 4, and denominator from 5 to 8, default is 5
int preambleLength=8; // from 2 to 20, default is 8
//String payload = "hello"; // you can change the payload

#define SS 15
#define RST 16
#define DI0 4
#define BAND 433E6  // Here you define the frequency carrier

byte localAddress = 0xFF;     // address of this device
//byte destination = 0xBB;      // destination to send to

String temperature;
String pressure;
String humidity;
String rainfall;
String lux;
String dust;
String noise;
WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa Receiver");
  Serial.print("SetFrequency : ");
  Serial.print(BAND);
  Serial.println("Hz");
  Serial.print("SetSpreadingFactor : SF");
  Serial.println(spreadingFactor);
  
  SPI.begin();
  LoRa.setPins(SS,RST,DI0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
 LoRa.setTxPower(txPower,1);
 LoRa.setSpreadingFactor(spreadingFactor);
 LoRa.setSignalBandwidth(signalBandwidth);
 LoRa.setCodingRate4(codingRateDenominator);
 LoRa.setPreambleLength(preambleLength);

// Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
 
}

void loop() {
   // try to parse packet
  onReceive(LoRa.parsePacket());
}
void onReceive (int packetSize){
  if (packetSize == 0) return;

   // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }
  int pos1, pos2, pos3, pos4, pos5, pos6;
  String LoRaData = (String)incoming;
  
    pos1 = LoRaData.indexOf('&');
    pos2 = LoRaData.indexOf('@');
    pos3 = LoRaData.indexOf('$');
    pos4 = LoRaData.indexOf('^');
    pos5 = LoRaData.indexOf('!');
    pos6 = LoRaData.indexOf('%');
    
    temperature = LoRaData.substring(0, pos1);
    pressure = LoRaData.substring(pos1 + 1, pos2);
    humidity = LoRaData.substring(pos2 + 1, pos3);
    rainfall = LoRaData.substring(pos3 + 1, pos4);
    lux = LoRaData.substring(pos4 + 1, pos5);
    dust = LoRaData.substring(pos5 + 1, pos6);
    noise = LoRaData.substring(pos6 + 1, LoRaData.length());

    Serial.println("Received from: 0x" + String(sender, HEX));
    Serial.println("Sent to: 0x" + String(recipient, HEX));
    Serial.println("Message ID: " + String(incomingMsgId));
    Serial.println("Message length: " + String(incomingLength));
 
    Serial.print(F("Temperature = "));
    Serial.print(temperature);
    Serial.println(F("*C"));
 
    Serial.print(F("Pressure = "));
    Serial.print(pressure);
    Serial.println(F("hPa"));

    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.println(F("%"));
 
    Serial.print(F("Rainfall = "));
    Serial.print(rainfall);
    Serial.println(F("%"));
 
    Serial.print(F("Light = "));
    Serial.print(lux);
    Serial.println(F(" lx"));

    Serial.print(F("Dust Denisty = "));
    Serial.print(dust);
    Serial.println(F(" ug/m3"));

    Serial.print(F("Noise = "));
    Serial.print(noise);
    Serial.println(F(" Decibel"));
 
    Serial.println(); 

    if (client.connect(server, 80))
    {
      String postStr = apiKey;
      postStr += "&field1=";
      postStr += String(temperature);
      postStr += "&field2=";
      postStr += String(pressure);
      postStr += "&field8=";
      postStr += String(humidity);
      postStr += "&field4=";
      postStr += String(rainfall);
      postStr += "&field5=";
      postStr += String(lux);
      postStr += "&field6=";
      postStr += String(dust);
      postStr += "&field7=";
      postStr += String(noise);
      postStr += "r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
 
      Serial.println("Data Send to Thingspeak");
      delay(500);
    }
    client.stop();
    Serial.println("Waiting...");
  }
  





 
