#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>   
#include "DHT.h"
//#include <GP2Y1010AU0F.h>

int ledPin = 3;           
int voPin = A0;
//GP2Y1010AU0F dustSensor(ledPin, voPin);  
// Parameters you can play with :

int txPower = 14; // from 0 to 20, default is 14
int spreadingFactor = 12; // from 7 to 12, default is 12
long signalBandwidth = 125E3; // 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3,41.7E3,62.5E3,125E3,250E3,500e3, default is 125E3
int codingRateDenominator=5; // Numerator is 4, and denominator from 5 to 8, default is 5
int preambleLength=8; // from 2 to 20, default is 8

#define SS 10
#define RST 9
#define DI0 2
#define BAND 433E6  // Here you define the frequency carrier
#define rain_sensor A2
#define DHTPIN 5
#define DHTTYPE DHT11
Adafruit_BMP280 bmp; 
BH1750 lightMeter;
DHT dht(DHTPIN, DHTTYPE);
String LoRaMessage = "";
String outgoing;
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends
float dust = 0;
float voMeasured = 0;
float calcVoltage = 0;
void setup() {
  Serial.begin(115200);
  Wire.begin();
  // Start LightMeter
  lightMeter.begin();
  // Start DHT
  dht.begin();
  // PinMode Dust
  pinMode(ledPin,OUTPUT);
  //dustSensor.begin();
  // Start BMP280
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                  Adafruit_BMP280::SAMPLING_X2,     
                  Adafruit_BMP280::SAMPLING_X16,    
                  Adafruit_BMP280::FILTER_X16,      
                  Adafruit_BMP280::STANDBY_MS_500);
  pinMode (rain_sensor, INPUT);
  // Setup LoRa
  while (!Serial);

  Serial.println("LoRa Sender");
  Serial.print("SetFrequency : ");
  Serial.print(BAND);
  Serial.println("Hz");
  Serial.print("SetSpreadingFactor : SF");
  Serial.println(spreadingFactor);

  SPI.begin();
  LoRa.setPins(SS,RST,DI0);

  // Start LoRa Connecting
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
 LoRa.setTxPower(txPower,1);
 LoRa.setSpreadingFactor(spreadingFactor);
 LoRa.setSignalBandwidth(signalBandwidth);
 LoRa.setCodingRate4(codingRateDenominator);
 LoRa.setPreambleLength(preambleLength);
// LoRa.setPolarity(1);
 //LoRa.setFSK(); 
 
}

void loop() {
  float temperature = bmp.readTemperature();        
  float pressure = bmp.readPressure();
  float humidity = dht.readHumidity();
  int rainfall = map(analogRead(rain_sensor), 780, 0, 0, 100);
  float noise = analogRead(A1);
  if (rainfall >= 100)
  {
    rainfall = 100;
  }
  if (rainfall <= 0)
  {
    rainfall = 0;
  }
  float lux = lightMeter.readLightLevel();
  digitalWrite(ledPin,LOW);
  delayMicroseconds(280);
  voMeasured = analogRead(voPin);
  delayMicroseconds(40);
  digitalWrite(ledPin,HIGH);
  delayMicroseconds(9680);

  calcVoltage = voMeasured*(5.0/1024);
  dust = 0.17*calcVoltage-0.1;
  if ( dust < 0)
  {
    dust = 0.00;
  }
  //dust = dustSensor.read();
 
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

  Serial.print("Dust Density:");
  Serial.print(dust);
  Serial.println(F(" ug/m3"));

  Serial.print(F("Noise = "));
  Serial.print(noise);
  Serial.println(F(" Decibel"));
 
  Serial.println();

  LoRaMessage = String(temperature) + "&" + String(pressure)
                + "@" + String(humidity) + "$" + String(rainfall)
                + "^" + String(lux) + "!" + String(dust) + "%" + String(noise);
  sendMessage(LoRaMessage); 
  Serial.println("Sending: " + LoRaMessage);
  delay(10000);
//  if (millis() - lastSendTime > interval) {
//    sendMessage(LoRaMessage); 
//    Serial.println("Sending: " + LoRaMessage);
//    lastSendTime = millis();
//    interval = random(2000) + 10000;
//  }
}
void sendMessage (String outgoing){
  LoRa.beginPacket();
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket(); 
  msgCount++;
}
