/*
  Complete code fietscomputer
  
  Benodigde libraries:
  Adafruit AHTX0 by Adafruit
  Adafruit BusIO by Adafruit
  Adafruit ST7735 and ST7789 Library by Adafruit
  Adafruit HX711 by Adafruit
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
#include <Arduino.h>//
#include "HX711.h"//
#include "soc/rtc.h"//

BH1750 lightMeter; //Lichtsensor
Adafruit_AHTX0 aht; //Temperatuursensor
PulseSensorPlayground pulseSensor; //Hartslagsensor
Adafruit_ST7789 tft = Adafruit_ST7789(20, 21, 10, 6, 5); //Display (RX, TX, MOSI, SCLK, Data/command)
BlynkTimer timer; //Blynk
//HX711 scale; //HX711

//Wifi
char ssid[] = "esp32hotspot";
char pass[] = "test12345";

//Variabelen
int temperatuur = 0;
int rusthartslag = 0;
int maxhartslag = 0;
float vo2Max = 0.0;

//Blynk versturen
void myTimerEvent()
{
  Blynk.virtualWrite(V0, temperatuur);
  //Blynk.virtualWrite(V1, snelheid);
  //Blynk.virtualWrite(V2, cadans);
  //Blynk.virtualWrite(V3, hartslag);
  //Blynk.virtualWrite(V4, lichtsterkte);
  //Blynk.virtualWrite(V5, VO2max);
  //Blynk.virtualWrite(V6, vermogen);
  //Blynk.virtualWrite(V7, snelheidwaarschuwing);
}

//Hall effect sensor
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
  pinMode(1, INPUT); //Hallpin = 1
  attachInterrupt(digitalPinToInterrupt(1), count, FALLING);

  // PulseSensor Hartslagsensor
  pulseSensor.analogInput(0); //Paarse draad aan pin 0
  pulseSensor.setThreshold(550); //Bepaalt wat als hartslag telt
  pulseSensor.begin();

  //HX711 Load cell
  //rtc_cpu_freq_config_t config;
  //rtc_clk_cpu_freq_get_config(&config);
  //rtc_clk_cpu_freq_to_config(RTC_CPU_FREQ_80M, &config);
  //rtc_clk_cpu_freq_set_config_fast(&config);

  //scale.begin(16, 4); // GPIOs LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN
  //scale.set_scale(INSERT YOUR CALIBRATION FACTOR); //calibratiefactor
  //scale.tare(); // reset schaal naar 0
  
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
  //Serial.print(scale.get_units(), 1); //Meting
  //scale.power_down(); //Load cell uit

  //BH1750 Lichtsensor
  float lux = lightMeter.readLightLevel();

  //AHT Temperatuursensor
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); //Nieuwe data

  //Hall effect sensor
  unsigned long start = micros();
  float seconds = (micros() - start) / 1000000.0;
  if(cnt == 0){
    tft.fillRect(50, 25, 140, 14, ST77XX_BLACK); tft.setCursor (70, 25); tft.print("0 RPM");
  }
  else{
    float rpm = cnt / seconds ;
    if(rpm < 1000){ //Realistische waarde
      tft.fillRect(50, 25, 140, 14, ST77XX_BLACK); tft.setCursor (70, 25); tft.print(String(rpm, 1) + " RPM");
    }
    else{
      tft.fillRect(50, 25, 140, 14, ST77XX_BLACK); tft.setCursor (70, 25); tft.print("Ongeldig");
    }
  }
  cnt = 0;

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
  tft.setCursor (70,65); tft.print("xx r/m"); //Cadanswaarde
  tft.fillRect(50, 105, 140, 14, ST77XX_BLACK); tft.setCursor (70,105); tft.print(String(temp.temperature, 1) + " C"); //Temperatuurwaarde
  tft.fillRect(50, 185, 140, 14, ST77XX_BLACK); tft.setCursor (70,185); tft.print(String(lux, 1) + " Lx"); //Lichtsterktewaarde
  tft.setCursor (25,225); tft.print("xx"); //VO2Maxwaarde
  tft.setCursor (170,225); tft.print("xx W"); //Vermogenwaarde

  //Aansturing LED
  if(lux<100){
    rgbLedWrite(RGB_BUILTIN, 10, 10, 10);
  } else{
    digitalWrite(RGB_BUILTIN, LOW);
  }

  temperatuur = temp.temperature;

  //VO2 max
  //if (StopKnop) {
  //  if (rusthartslag > 0 && maxhartslag > 0) {
  //    VO2Max = 15.0 * (maxhartslag / float(rusthartslag));
  //  }
  //}

  //Blynk activatie
  Blynk.run();
  timer.run(); // Initiates BlynkTimer

  delay(1000); 
  //scale.power_up(); //Load cell aan
}