/*
  This is based on ccs811envdata.ino but with a different  temp sensor - Reading temperature and humidity data from the DHT22, passing that to the CCS811, to get better CO2 readings from CCS811
  Created by Sebastian Friedrich
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>    // I2C library
#include <DHT.h>
#include "ccs811.h"  // CCS811 library

#define DHTTYPE   DHT22
#define DHTPIN    12

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT dht(DHTPIN, DHTTYPE, 22);
CCS811 ccs811(D3); // nWAKE on D3

float temperature, humidity;
int soil = analogRead(A0);
unsigned long previousMillis = 0;
const long interval = 1000;
float humidityThreshold = 60;
uint16_t eco2Threshold = 1000; // up to 1000 is ok over 2000 is bad, fresh air is needed
uint16_t etvocThreshold = 400; // up to 400 is ok

void setup() {
  // Enable serial
  Serial.begin(115200);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } 

  // Enable I2C, e.g. for ESP8266 NodeMCU boards: VDD to 3V3, GND to GND, SDA to D2, SCL to D1, nWAKE to D3 (or GND)
  Serial.print("setup: I2C ");
  Wire.begin(); 
  Serial.println("ok");
  
  // Enable DHT22
  Serial.print("setup: DHT22 ");
  dht.begin();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  pinMode(soil, INPUT);

  // Enable CCS811
  Serial.print("setup: CCS811 ");
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  ccs811.begin();

  // Start CCS811 (measure every 1 second)
  Serial.print("setup: CCS811 start ");
  ccs811.start(CCS811_MODE_1SEC);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  // Clear the buffer
  display.clearDisplay();

  pinMode(D5,OUTPUT);  //red led

}


void loop() {
  unsigned long currentMillis = millis();

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
                 
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.println("Reporting " + String((int)temperature) + "C and " + String((int)humidity) + " % humidity");

  Serial.println();
        //clear display
    display.clearDisplay();
   
    // display temperature
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print("Temperature: ");
    display.setTextSize(2);
    display.setCursor(0,10);
    display.print(temperature);
    display.print(" ");

    display.print("C");
    
    // display humidity
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print("Humidity: ");
    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print(humidity);
    display.print(" %");
    display.setTextSize(1);
    display.setCursor(0, 55);


  ccs811.set_envdata(temperature, humidity);
  Serial.print("  ");

  // Read CCS811
  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2,&etvoc,&errstat,&raw); 

  // Process CCS811
  Serial.print("CCS811: ");
  if( errstat==CCS811_ERRSTAT_OK ) {
    Serial.print("eco2=");  Serial.print(eco2);  Serial.print(" ppm  ");
    Serial.print("etvoc="); Serial.print(etvoc); Serial.print(" ppb  "); 
    display.print(eco2); display.print("ppm ");
    display.print(etvoc); display.print("ppb");
 
  } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
    Serial.print("waiting for (new) data");
    display.print("waiting for (new) data");
  } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
    Serial.print("I2C error");
    display.print("I2C error");
  } else {
    Serial.print( "error: " );
    Serial.print( ccs811.errstat_str(errstat) );
    display.print( "error: " );
    display.print( ccs811.errstat_str(errstat) );
  }
    
  display.display();

  if (humidity > humidityThreshold || eco2 > eco2Threshold || etvoc > etvocThreshold) {
    digitalWrite(D5,HIGH);
  } else {
    digitalWrite(D5,LOW);
  }

  // Wait
  delay(interval); 
}