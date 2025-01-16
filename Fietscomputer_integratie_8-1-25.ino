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

#define LED_pin    4

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
unsigned long previousMillis = 0;  // Tijdstip van de laatste schakeling
const long interval = 200;
bool ledState = false;  
int vermogen = 0;
int temperatuur = 0;
int rusthartslag = 0;
int maxhartslag = 0;
float vo2Max = 0.0;
float last_speed = 0;
float avg_speed = 0;
float metingen = 0;
int speed_sum = 0;
int led_state= 0;

unsigned long vorigeMillis = 0;  // Slaat de laatste tijd op dat de LED werd bijgewerkt
const long wacht = 1000;       // Interval om de LED te laten knipperen (in milliseconden)

//Hall effect sensor
float wheelCircumference = 0.062; // Omtrek van het wiel in meters
unsigned long lastTime = 0;     // Laatste pulseberekening
float speed = 0.0;  
volatile int cnt = 0;

void count() {
  cnt++;
}

//Blynk versturen
void myTimerEvent(){
  //Blynk.virtualWrite(V0, temperatuur);
  //Blynk.virtualWrite(V1, snelheid);
  //Blynk.virtualWrite(V2, cadans);
  //Blynk.virtualWrite(V3, hartslag);
  //Blynk.virtualWrite(V4, lichtsterkte);
  //Blynk.virtualWrite(V5, VO2max);
  //Blynk.virtualWrite(V6, vermogen);
  //Blynk.virtualWrite(V7, snelheidwaarschuwing); bij waarschuwing 1 geven anders is het standaard 0
}

void setup(){
  Serial.begin(115200); //Serial monitor output (niet in eindcode
  pinMode(4, OUTPUT); 
 
    
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

  //HX711
  scale.begin(18, 19); // GPIOs LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN
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
}

void loop() {

  //BH1750 Lichtsensor
  float lux = lightMeter.readLightLevel();

  //AHT Temperatuursensor
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); //Nieuwe data
  temperatuur = (temp.temperature, 1); //Blynk

  //HX711 Load cell
  int kracht = scale.read(); //Meting
  scale.power_down(); //Load cell uit

  //Hall effect sensor
  unsigned long currentTime = millis(); // Bereken snelheid elke seconde (1000 ms)
  if (currentTime - lastTime >= 100) {
    unsigned long currentMillis = millis();  // Huidige tijd ophalen
    // Controleer of het interval is verstreken
    metingen++;
    float pulsesPerSecond = cnt;  // Pulses per seconde
    float distanceTraveled = (pulsesPerSecond * wheelCircumference);  // Totale afstand in meter
    last_speed = speed;
    speed = distanceTraveled * 3.6;  // Snelheid in kilometers per seconde
    speed_sum += speed;
    avg_speed = (speed_sum)/metingen;
    
    
    if (((avg_speed - speed)>= 0.3)|| (avg_speed - speed) <= -0.3){
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(2); //Tekstgrootte
      tft.setCursor (40,260); //Cursorpositie
      tft.print("WAARSCHUWING: \n"); //Snelheidswaarde
      tft.setCursor (40,280); //Cursorpositie
      tft.print("hoge snelheid \n"); //Snelheidswaarde
    }
    if (!(((avg_speed - speed)>=0.3)|| (avg_speed - speed) <= -0.3)){
      tft.fillRect(40, 250, 260, 300, ST77XX_BLACK); 
    }
    cnt = 0; // Reset pulseaantal


    previousMillis = currentMillis;
    if (last_speed-speed > 0.3){


// Haal de huidige tijd op
  unsigned long huidigeMillis = millis();

  // Start een while-loop als het tijd is om de LED om te schakelen
  int i = 0;
  while (i<3) {
    i++;
    // Zet de LED aan
    digitalWrite(4, HIGH);
    // Wacht 1 seconde (1000 milliseconden)
    delay(100);
    // Zet de LED uit
    digitalWrite(4, LOW);
    // Wacht weer 1 seconde
    delay(100);
  }
  i = 0;


    }
    if (last_speed-speed < 0.3){
      
      digitalWrite(4,LOW);
    }
    
   
    lastTime = currentTime; // Update lastTime
    vermogen = abs(kracht * distanceTraveled); //absolute waarde van kracht*snelheid
  }

  //PulseSensor Hartslagsensor
  if (pulseSensor.sawStartOfBeat()){ //Test constant voor hartslag
    int myBPM = pulseSensor.getBeatsPerMinute();  //Bepaalt BPM
    tft.setTextColor(ST77XX_WHITE); tft.fillRect(50, 145, 140, 14, ST77XX_BLACK); tft.setCursor(70,145); tft.print(String(myBPM) + " BPM"); //Display hartslagwaarde
    if (rusthartslag == 0 || myBPM < rusthartslag) { //Bepaalt rusthartslag
      rusthartslag = myBPM;
    }
    if (myBPM > maxhartslag){ //Bepaalt maximale hartslag
      maxhartslag = myBPM;
    }
  }

  //DISPLAY Instellingen Waardes + Eenheden
  tft.setTextSize(2); //Tekstgrootte
  tft.setTextColor(ST77XX_WHITE); //Tekstkleur
  tft.fillRect(50, 25, 140, 14, ST77XX_BLACK); tft.setCursor (70, 25); tft.print(String(speed, 1) + " km/h");
  tft.fillRect(50, 65, 140, 14, ST77XX_BLACK); tft.setCursor (70,65); tft.print(String(last_speed-speed,1)+" r/m"); //Cadanswaarde
  tft.fillRect(50, 105, 140, 14, ST77XX_BLACK); tft.setCursor (70,105); tft.print(String(temp.temperature, 1) + " C"); //Temperatuurwaarde
  tft.fillRect(50, 185, 140, 14, ST77XX_BLACK); tft.setCursor (70,185); tft.print(String(lux, 1) + " Lx"); //Lichtsterktewaarde
  tft.fillRect(50, 225, 50, 14, ST77XX_BLACK); tft.setCursor (25,225); tft.print("xx"); //VO2Maxwaarde
  tft.fillRect(50, 225, 340, 14, ST77XX_BLACK); tft.setCursor (170,225); tft.print(vermogen); //Vermogenwaarde

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
  timer.run();

  scale.power_up(); //Load cell aan
}
