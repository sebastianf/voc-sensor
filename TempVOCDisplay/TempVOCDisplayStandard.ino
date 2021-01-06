/*
  This is based on ccs811envdata.ino but with a different  temp sensor - Reading temperature and humidity data from the DHT22, passing that to the CCS811, to get better CO2 readings from CCS811
  Created by Sebastian Friedrich
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_CCS811.h"
#include <Adafruit_Sensor.h>
#include <Wire.h>    // I2C library
#include <DHT.h>

#define DHTTYPE   DHT22
#define I2C_CCS811_ADDRESS 0x5A    // CCS811 I2C address

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_CCS811 ccs;     // CCS811 is connected (I2C) to D1-->SLC-->GPIO5-->Pin5 & D2-->SDA-->GPIO4-->Pin4

float temperature, humidity, eco2, tvoc;
int soil = analogRead(A0); 
const long updateIntervallInSeconds = 10;
unsigned long previousMillis = 0;
float humidityThreshold = 60;
uint16_t eco2Threshold = 1500; // up to 1000 is ok over 2000 is bad, fresh air is needed
uint16_t etvocThreshold = 400; // up to 400 is ok
volatile int interruptCounter = 0;
volatile bool buttonWasPressed = false;
int DHTPIN = 12;
int REDLEDPIN = 14;
int BUTTONPIN = 13;

DHT dht(DHTPIN, DHTTYPE, 22);

void ICACHE_RAM_ATTR buttonPressed() { // ICACHE_RAM_ATTR was necessary otherwise it hasn't worked
  interruptCounter++;
  buttonWasPressed = true;
  if (interruptCounter > 3) {
    interruptCounter = 0;
  }
    Serial.println("Button pressed. Show screen number: " + String((int)interruptCounter));
}

void setup() {
  pinMode(REDLEDPIN, OUTPUT);  
  pinMode(BUTTONPIN, INPUT_PULLUP);

  // Enable serial
  Serial.begin(115200);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } 

  // Enable I2C, e.g. for ESP8266 NodeMCU boards: VDD to 3V3, GND to GND, SDA to D2, SCL to D1, nWAKE to D3 (or GND)
  Wire.begin(); 
  // Enable DHT22
  dht.begin();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  pinMode(soil, INPUT);

  // Enable CCS811
  ccs.begin();
  delay(10);
  Serial.println("CCS811 test");
  if(!ccs.begin()){
    Serial.println("Failed to start CCS811 sensor! Please check your wiring!");
    while(1);
  }

  // Set CCS811 to Mode 2: Pulse heating mode IAQ measurement every 10 seconds
  ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  // Clear the buffer
  display.clearDisplay();
  attachInterrupt(digitalPinToInterrupt(BUTTONPIN), buttonPressed, RISING);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= updateIntervallInSeconds * 1000 || buttonWasPressed) {
    previousMillis = currentMillis;
    buttonWasPressed = false;
    
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
                   
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    Serial.println("Reporting " + String((int)temperature) + "C and " + String((int)humidity) + " % humidity");
  
    // Pass DHT22 temp & hum readings to CSS811 for compensation algorithm
    ccs.setEnvironmentalData(humidity, temperature);
  
    // Read CCS811
    if(ccs.available()){
      float temp = ccs.calculateTemperature();
      if(ccs.readData()){
        // Read CCS811 values
        eco2 = ccs.geteCO2();
        tvoc = ccs.getTVOC();
        Serial.println("eco2= " + String((float)eco2) + "ppm tvoc=" + String((float)tvoc) + "ppb");
      }
    }
    
    switch(interruptCounter) {
      case 0: 
        displayTwoValues("Temperature", temperature, "C", "Humidity", humidity, "%");  
      break;
      case 1:
        displayTwoValues("eco2 in ppm", eco2, "", "tvoc in ppb", tvoc, "");
      break;
      case 2:
        displayTwoValues("Temperature", temperature, "C", "Humidity", humidity, "%");
        display.setTextSize(1);
        display.setCursor(0, 55);
        display.print(eco2); display.print("ppm");
        display.print(tvoc); display.print("ppb");
        display.display();
      break;
      default:
        display.clearDisplay();
        display.display();
      break;
    }
  
  //  if (humidity > humidityThreshold || eco2 > eco2Threshold || etvoc > etvocThreshold) {
  //    digitalWrite(REDLEDPIN,HIGH);
  //  } else {
  //    digitalWrite(REDLEDPIN,LOW);
  //  }
  } 
}

void displayTwoValues(String descriptionTop, float valueTop, String unitTop, String descriptionBottom, float valueBottom, String unitBottom) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(descriptionTop + ":");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(valueTop);
  display.print(" " + unitTop);
  
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print(descriptionBottom + ":");
  display.setTextSize(2);
  display.setCursor(0, 40);
  display.print(valueBottom);
  display.print(" " + unitBottom);
  display.display();
}