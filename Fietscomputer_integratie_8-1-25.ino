/*
  Complete code fietscomputer
  
  Benodigde libraries:
  Adafruit AHTX0 by Adafruit
  Adafruit BusIO by Adafruit
  Adafruit ST7735 and ST7789 Library by Adafruit
  HX711 Arduino Library by Bogdan Necula
  BH1750 by Christopher Laws
  PulseSensor Playground by Joel Murphy, ...
  Blynk by Volodymyr Shymanskyy
*/

//Blynk
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID           "TMPL4Yulodr-v"
#define BLYNK_TEMPLATE_NAME         "Project ESP32"
#define BLYNK_AUTH_TOKEN            "zFWLl2gME5_I9ySEqx2BpbQYU8OtEXV6"

//Libraries
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_AHTX0.h>
#include <PulseSensorPlayground.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_ST7789.h> 
#include <SPI.h> 
#include <BlynkSimpleEsp32.h>
#include <Arduino.h>
#include "HX711.h"
#include "soc/rtc.h"

BH1750 lightMeter; //Lichtsensor
Adafruit_AHTX0 aht; //Temperatuursensor
PulseSensorPlayground pulseSensor; //Hartslagsensor
Adafruit_ST7789 tft = Adafruit_ST7789(20, 21, 10, 6, 5); //Display (RX, TX, MOSI, SCLK, Data/command)
BlynkTimer timer; //Blynk
HX711 scale; //HX711

//Wifi
char ssid[] = "esp32hotspot";
char pass[] = "test12345";

//Variabelen
int temperatuur = 0;
int rusthartslag = 0;
int maxhartslag = 0;
float vo2Max = 0.0;
int last_speed = 0;
int avg_speed = 0;
int metingen = 0;
int speed_sum = 0;
int led_state= 0;
#define LED_pin    8

//Blynk versturen
void myTimerEvent()
{
  //Blynk.virtualWrite(V0, temperatuur);
  //Blynk.virtualWrite(V1, snelheid);
  //Blynk.virtualWrite(V2, cadans);
  //Blynk.virtualWrite(V3, hartslag);
  //Blynk.virtualWrite(V4, lichtsterkte);
  //Blynk.virtualWrite(V5, VO2max);
  //Blynk.virtualWrite(V6, vermogen);
  //Blynk.virtualWrite(V7, snelheidwaarschuwing); bij waarschuwing 1 geven anders is het standaard 0
}

//Hall effect sensor
float wheelCircumference = 2.0; // Circumference of the wheel in meters (adjust as needed)
unsigned long lastTime = 0;    // Last time a pulse count was calculated
float speed = 0.0;  
volatile int cnt = 0;
void count() {
  cnt++;
}


void setup(){
  Serial.begin(9600); //Serial monitor output (niet in eindcode)

  //Blynk setup
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);

  // Initialize I2C (SDA, SCL)
  Wire.begin(7, 9);

  //BH1750 Lichtsensor
  lightMeter.begin(BH1750::CONTINUOUS_LOW_RES_MODE);

  //AHT Temperatuursensor
  aht.begin();

  //Hall effect sensor
  pinMode(2, INPUT); //Hallpin = 2
  attachInterrupt(digitalPinToInterrupt(2), count, FALLING);

  // PulseSensor Hartslagsensor
  pulseSensor.analogInput(0); //Paarse draad aan pin 0
  pulseSensor.setThreshold(550); //Bepaalt wat als hartslag telt
  pulseSensor.begin();

  //HX711
  scale.begin(1, 4); // GPIOs LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN
  scale.set_scale(-1025); // calibratiefactor
  scale.tare(); // reset schaal naar 0
  
  //Display Scherminstellingen
  tft.init(240,320); //Schermresolutie
  tft.setRotation(2); //Rotatie van tekst naar de juiste positie
  tft.fillScreen(ST77XX_BLACK); //Leeg scherm naar zwart
  tft.drawLine(1, 40, 240, 40, ST77XX_WHITE);
  tft.drawLine(1, 80, 240, 80, ST77XX_WHITE);
  tft.drawLine(1, 120, 240, 120, ST77XX_WHITE);
  tft.drawLine(1, 160, 240, 160, ST77XX_WHITE);
  tft.drawLine(1, 200, 240, 200, ST77XX_WHITE);
  tft.drawLine(1, 240, 240, 240, ST77XX_WHITE);
  tft.drawLine(120, 200, 120, 240, ST77XX_WHITE); //Gridlijnen
  tft.setTextColor(ST77XX_WHITE); //Tekstkleur
 
  //Display Grootheden
  tft.setTextSize(1); //Tekstgrootte
  tft.setCursor (100,10); tft.print("Snelheid");
  tft.setCursor (100,50); tft.print("Cadans");
  tft.setCursor (100,90); tft.print("Temperatuur");
  tft.setCursor (100,130); tft.print("Hartslag");
  tft.setCursor (100,170); tft.print("Lichtsterkte");
  tft.setCursor (30,210); tft.print("VO2Max");
  tft.setCursor (170,210); tft.print("Vermogen");
 
  //Display Waarschuwingen
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2); //Tekstgrootte
  tft.setCursor (40,260); //Cursorpositie
  tft.print("WAARSCHUWING: \n"); //Snelheidswaarde
  tft.setCursor (40,280); //Cursorpositie
  tft.print("hoge snelheid \n"); //Snelheidswaarde
}

void loop() {

  //HX711 Load cell
  int gewicht = scale.read(); //Meting
  scale.power_down(); //Load cell uit

  //BH1750 Lichtsensor
  float lux = lightMeter.readLightLevel();

  //AHT Temperatuursensor
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); //Nieuwe data
  temperatuur = (temp.temperature, 1); //Blynk

  //Hall effect sensor
  unsigned long currentTime = millis(); // Calculate speed every second (1000 ms)
 
  if (currentTime - lastTime >= 1000) {
    // Calculate speed in meters per second
    metingen++:
    float pulsesPerSecond = cnt;  // Pulses per second
    float distanceTraveled = (pulsesPerSecond * wheelCircumference);  // Total distance traveled in meters
    last_speed = speed;
    speed = distanceTraveled * 3,6;  // Speed in meters per second


    avg_speed = (speed_sum+speed)/metingen;
    if (((avg_speed - speed)>= 3)|| (avg_speed - speed) >= -3)
    {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2); //Tekstgrootte
    tft.setCursor (40,260); //Cursorpositie
    tft.print("WAARSCHUWING: \n"); //Snelheidswaarde
    tft.setCursor (40,280); //Cursorpositie
    tft.print("hoge snelheid \n"); //Snelheidswaarde

    }
    if (!(((avg_speed - speed)>= 2)|| (avg_speed - speed) >= -2))
    {
      tft.fillRect(40, 100, 260, 300, ST77XX_BLACK); 
    

    }

    // Reset pulse count for the next calculation
    cnt = 0;
    if (last_speed-speed > 2)
    {
      led_state = !led_state;
      digitalWrite(LED_pin, led_state)

    }
    if (!(last_speed-speed>2))
    {
      digitalWrite(LED_pin, LOW);
    }
   
    // Update the last time
    lastTime = currentTime;
  }

  //PulseSensor Hartslagsensor
  if (pulseSensor.sawStartOfBeat()){ //Test constant voor hartslag
    int myBPM = pulseSensor.getBeatsPerMinute();  //Bepaalt BPM
    tft.fillRect(50, 145, 140, 14, ST77XX_BLACK); tft.setCursor(70,145); tft.print(String(myBPM) + " BPM"); //Display hartslagwaarde
    if (rusthartslag == 0 || myBPM < rusthartslag) { //Bepaalt rusthartslag
      rusthartslag = myBPM;
    }
    if (myBPM > maxhartslag) { //Bepaalt maximale hartslag
      maxhartslag = myBPM;
    }
  }

  //DISPLAY Instellingen Waardes + Eenheden
  tft.setTextSize(2); //Tekstgrootte
  tft.setTextColor(ST77XX_WHITE); //Tekstkleur
  tft.fillRect(50, 25, 140, 14, ST77XX_BLACK); tft.setCursor (70, 25); tft.print(String(speed, 1) + " m/s");
  tft.fillRect(50, 65, 140, 14, ST77XX_BLACK); tft.setCursor (70,65); tft.print("xx r/m"); //Cadanswaarde
  tft.fillRect(50, 105, 140, 14, ST77XX_BLACK); tft.setCursor (70,105); tft.print(String(temp.temperature, 1) + " C"); //Temperatuurwaarde
  tft.fillRect(50, 185, 140, 14, ST77XX_BLACK); tft.setCursor (70,185); tft.print(String(lux, 1) + " Lx"); //Lichtsterktewaarde
  tft.fillRect(50, 225, 50, 14, ST77XX_BLACK); tft.setCursor (25,225); tft.print("xx"); //VO2Maxwaarde
  tft.fillRect(50, 225, 340, 14, ST77XX_BLACK); tft.setCursor (170,225); tft.print(gewicht); //Vermogenwaarde

  scale.power_down(); //Load cell uit

  //Aansturing LED
  if(lux<100){
    rgbLedWrite(RGB_BUILTIN, 10, 10, 10);
  } else{
    digitalWrite(RGB_BUILTIN, LOW);
  }

  //VO2 max
  //if (StopKnop) {
  //  if (rusthartslag > 0 && maxhartslag > 0) {
  //    VO2Max = 15.0 * (maxhartslag / float(rusthartslag));
  //  }
  //}

  //Blynk activatie
  Blynk.run();
  timer.run(); // Initiates BlynkTimer

  delay(100); 
  scale.power_up(); //Load cell aan
}
