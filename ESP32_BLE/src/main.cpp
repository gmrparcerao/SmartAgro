/*---------------Header----------------------------
    SENAI Technology College "Mariano Ferraz"
    Sao Paulo, 05/03/2022
    Postgraduate - Internet of Things

    Names of postgraduate students: Claudinei, Guilherme, Renan and Wellington
    Lecturer: André and Caio Vinícius

    Goals: 
    Hardware: ESP32, raspberry PI 3, rain sensor, humidity sensor, temperature and humidity sensor,
      atmospheric pressure sensor, solenoid valve

    Libraries:
    DHT sensor library by Adafruit,
    SparkFun BME280 by SparkFun Electronics

    Reviews: 
    R000 - begin
    R001 - BME280 connection is ok

    http://arduino.esp8266.com/stable/package_esp8266com_index.json
	  https://dl.espressif.com/dl/package_esp32_index.json

  */

//---------------Libraries-------------------------

// General
#include <Arduino.h>
#include <cstring>

// DHT11
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "DHT_U.h"

// Bluetooth low energy
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// BME280
#include "SparkFunBME280.h"

//---------------Global Variables------------------

// DHT11
#define DHTTYPE     DHT11 // Sensor that will be used
#define DHTPIN      4     // DHT 11 connected at pin 4

DHT_Unified dht(DHTPIN, DHTTYPE);

float humidity = 0.0;
float temperature = 0.0;

// Bluetooth low energy
//Site to generate UUID: https://www.uuidgenerator.net/
#define SERVICE_UUID             "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_RX   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pCharacteristicTX;
BLECharacteristic *pCharacteristicRX;
BLEAdvertising *pAdvertising;

bool devideConnected = false;
bool prepareConnection = false;

String txValue;

BLEService *pService;
BLEServer *pServer;

// Ground (humidity)
#define GROUNDPIN     2     // Humidity sensor connected at pin 2
float ground = 0.0;

// Atmospheric pressure sensor
float pressure = 0.0;

BME280 bme280;

// Rain sensor
#define RAINSENSORPIN 5     // rain sensor connected at pin 5
bool rainSensor = false;

// SOLENOIDPIN valve
#define SOLENOIDPIN   15    // SOLENOIDPIN valve connected at pin 15
bool solenoid = false;


//---------------Auxiliary Functions---------------

// DHT 11
void dhtReading(){

// (* function to read DHT sensor data *)

  sensors_event_t event;

  // Read temperature
  dht.temperature().getEvent(&event);
  
  if (isnan(event.temperature)) {
    
    temperature = -1.0; // value -1.0 will mean not a number (wrong medition)

  }
  
  else{    
    
    temperature = event.temperature; // [ºC]

  }
    
    // Read humidity
  dht.humidity().getEvent(&event);
    
  if (isnan(event.relative_humidity)) {
    
    humidity = -1.0; // value -1.0 will mean not a number (wrong medition)

    }
    
    else{
      
      humidity = event.relative_humidity; // [%]

    }
    
  }

// Bluetooth low energy
  // Callback from connections
  class MyServerCallbacks:
    public BLEServerCallbacks{
      void onConnect (BLEServer *pServer){
        devideConnected = true;
        Serial.println("Bluetooth connected");
      };

      void onDisconnect (BLEServer *pServer){
        devideConnected = false;
        prepareConnection = true;
        Serial.println("Bluetooth disconnected");

      }

  };

// Callbacks from reading  
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);
          Serial.println();

      }
    }
};

void initializeBLU(){
  // (* function to initialize bluetooth low energy *)
  // Create the BLE device
  BLEDevice::init ("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();

  // Create the BLE Service
  pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristicTX = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_TX,
                                         BLECharacteristic::PROPERTY_READ
                                       );

  pCharacteristicRX = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pServer->setCallbacks(new MyServerCallbacks());
  pCharacteristicRX->setCallbacks(new MyCallbacks()); 

  //BLE2902 need to notify
  pCharacteristicTX->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
    
}

void reconnectBLE(){
  // (* function to reconnect bluetooth low energy after communication are over *)
  pService->stop();
  delay(200);
  pService->start();

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); 
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Read for a new connection...");
  prepareConnection = false;

}

// General functions
float toEnginnerUnit(int raw, float max){

  // (* function to convert raw data to enginner unit data *)    
  return (float(raw) / 4095.0) * max;

}

void sendData(){
  // (* function to send data to raspberry *)
  // standard: humidity;temperature;ground;pressure;rainSensor;solenoid    
  txValue = "";
  txValue = (String)humidity + ";" + (String)temperature + ";";
  txValue += (String)ground + ";" + (String)pressure + ";";
  txValue += (String)rainSensor + ";" + (String)solenoid;

  // Create a char of the send data string
  int n = txValue.length();
  char txString[n+1];

  strcpy(txString, txValue.c_str()); // Transform String at char to send by BLU
  
  pCharacteristicTX->setValue(txString); // Setting the value to the characteristic    
  pCharacteristicTX->notify(); // Notify the connected client
  
  Serial.println(String(txString)); // Test for BLE communication

}

void bme280Reading(){
  
  pressure = bme280.readFloatPressure(); // [pa]
  delay(50);
  
}

//---------------Setup function--------------------
void setup(){

	Serial.begin(11500);
  dht.begin();     // Initialize DHT sensor
  initializeBLU(); // Initialize bluetooth low energy

  // Communication with BME280
  Wire.begin();
  Wire.setClock(400000); //Increase to fast I2C speed!
  bme280.setI2CAddress(0x76);      
  if(bme280.beginI2C() == false) {        
    
    Serial.println("BME280 connect failed");
  
  }
  
  bme280.setReferencePressure(101200); //Adjust the sea level pressure used for altitude calculations
  bme280.setMode(MODE_NORMAL); //MODE_SLEEP, MODE_FORCED, MODE_NORMAL is valid. See 3.3
  pinMode(RAINSENSORPIN,INPUT);  // Define rain sensor pin as input
  pinMode(SOLENOIDPIN,OUTPUT);   // Define solenoid valve as output

}

//---------------Main Function---------------------
void loop(){

  dhtReading(); // Call DHT reading
  ground = analogRead(GROUNDPIN); // Read humidity
  rainSensor = digitalRead(RAINSENSORPIN); //read rain sensor
  bme280Reading(); // Call BME280 reading

  // When bluetooth connection are over, drop and start again
  if (prepareConnection){
    
    reconnectBLE();
    
  };

  sendData();   // Send data to Raspberry PI    
  delay(1000);

}