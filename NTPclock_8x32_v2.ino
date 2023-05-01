/* original design by niq_ro: https://github.com/tehniq3/
 * v.1 - test for have a stable version
 * v.2.0 - change almost all.. e.g from FastLED to Adafruit_NeoMatrix libraries
 * 
*/

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>  // https://github.com/adafruit/Adafruit_NeoMatrix
#include <Adafruit_NeoPixel.h>  
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <SolarCalculator.h> //  https://github.com/jpb10/SolarCalculator

#define DSTpin 12 // GPIO14 = D6, see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
#define PIN 14 // D5 = GPIO14  https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

// MATRIX DECLARATION:
// Parameter 1 = width of the matrix
// Parameter 2 = height of the matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (RGB+W NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);   // https://www.adafruit.com/product/2294

const uint16_t colors[] = {
  matrix.Color(255, 0, 0),    // red
  matrix.Color(0, 255, 0),    // green
  matrix.Color(255, 255, 0),  // yellow
  matrix.Color(0, 0, 255),    // blue 
  matrix.Color(255, 0, 255),  // mouve
  matrix.Color(0, 255, 255),  // bleu (ligh blue)
  matrix.Color(255, 255, 255) // white 
  };

// WS2812b day / night brightness.
#define NIGHTBRIGHTNESS 4      // Brightness level from 0 (off) to 255 (full brightness)
#define DAYBRIGHTNESS 20

const long timezoneOffset = 2; // ? hours
const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed

unsigned long tpafisare;
unsigned long tpinfo = 60000;  // 15000 for test in video

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;
bool          colon_switch = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

byte DST = 0;
byte DST0;
bool updated;
byte a = 0;

String text = "NTP clock with weather data by niq_ro on 8x32 addressable led display !";
int ltext = 6*71;  // lengh of string
int x;

int ora = 20;
int minut = 24;
int secundar = 0;
int zi, luna, an;

// Location - Craiova: 44.317452,23.811336
double latitude = 44.31;
double longitude = 23.81;
double transit, sunrise, sunset;
int ora1, ora2, oraactuala;
int m1, hr1, mn1, m2, hr2, mn2; 
byte aratadata = 0;
byte culoare = 1;

void setup() {
  Serial.begin(115200);
  pinMode (DSTpin, INPUT);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(NIGHTBRIGHTNESS);  // original 255
  matrix.setTextColor(colors[6]);
  matrix.fillScreen(0);
matrix.setCursor(0, 0);
matrix.print(F("v.2.0"));
matrix.setPixelColor(255, matrix.Color(0, 150, 0));
matrix.show();
delay(2000);
/*
x    = matrix.width();
matrix.fillScreen(0);
  for (x; x > -ltext ; x--)
  {
  matrix.fillScreen(0); 
  matrix.setCursor(x, 0);
  matrix.print(text);  
  matrix.show();
  delay(50);
  Serial.println(x);
 }
*/
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    wifiManager.autoConnect("AutoConnectAP");
      
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    delay(500);

  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  timeClient.setTimeOffset((timezoneOffset + DST)*3600);
  timeClient.begin();
  DST0 = DST; 
  
Soare();
if (night())
      {
        matrix.setBrightness (NIGHTBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      matrix.setBrightness (DAYBRIGHTNESS);
      }
    matrix.show();
}

void loop() {
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (WiFi.status() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    updated = timeClient.update();
    if (updated) {
      Serial.println("NTP time updated.");
      getYear();
      getMonth();
      zi = timeClient.getDay();
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }
 
unsigned long t = millis();

ora = timeClient.getHours();
minut = timeClient.getMinutes();
secundar = timeClient.getSeconds();

String ceas = "";
if (ora/10 == 0) 
  ceas = ceas + "0";
  ceas = ceas + ora;
  if (secundar%2 == 0)
    ceas = ceas + ":";
  else
    ceas = ceas + " ";
if (minut/10 == 0) 
  ceas = ceas + "0";
  ceas = ceas + minut;

matrix.fillScreen(0);
matrix.setTextColor(colors[culoare]);
matrix.setCursor(1, 0);
matrix.print(ceas);
//matrix.setPixelColor(255, matrix.Color(0, 150, 0));
matrix.show();

if (DST != DST0)
{
  timeClient.setTimeOffset((timezoneOffset + DST)*3600);
  timeClient.begin();
DST0 = DST;
Soare();
}

String data = "";
if (zi/10 == 0) 
  data = data + "0";
  data = data + zi;
  data = data + ".";
if (luna/10 == 0) 
  data = data + "0";
  data = data + luna;
  data = data + ".";
  data = data + an;
int ldata = 6*10;  

x    = matrix.width();
if ((secundar > 35) and (aratadata == 1))
{
matrix.fillScreen(0);
  for (x; x > -ldata ; x--)
  {
  matrix.fillScreen(0); 
  matrix.setCursor(x, 0);
  matrix.print(data);  
  matrix.show();
  delay(100);
  Serial.println(x);
 }
  aratadata = 0; 
  culoare++;
  if (culoare > 6) culoare = 0;
}

Soare();
      if (night())
      {
        matrix.setBrightness (NIGHTBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      matrix.setBrightness (DAYBRIGHTNESS);
      }
      matrix.show();

if (secundar == 0) aratadata = 1;
delay(100);
}  // end main loop

void getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  an = ti->tm_year + 1900;
  Serial.print("year = ");
  Serial.println(an);
}

void getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  luna = ti->tm_mon + 1;
  Serial.print("month = ");
  Serial.println(luna);
}

void Soare()
{
   // Calculate the times of sunrise, transit, and sunset, in hours (UTC)
  calcSunriseSunset(an, luna, zi, latitude, longitude, transit, sunrise, sunset);

m1 = int(round((sunrise + timezoneOffset + DST) * 60));
hr1 = (m1 / 60) % 24;
mn1 = m1 % 60;

m2 = int(round((sunset + timezoneOffset + DST) * 60));
hr2 = (m2 / 60) % 24;
mn2 = m2 % 60;

  Serial.print("Sunrise = ");
  Serial.print(sunrise+timezoneOffset+DST);
  Serial.print(" = ");
  Serial.print(hr1);
  Serial.print(":");
  Serial.print(mn1);
  Serial.println(" ! ");
  Serial.print("Sunset = ");
  Serial.print(sunset+timezoneOffset+DST);
  Serial.print(" = ");
  Serial.print(hr2);
  Serial.print(":");
  Serial.print(mn2);
  Serial.println(" ! ");
}

boolean night() { 
  ora1 = 100*hr1 + mn1;  // rasarit 
  ora2 = 100*hr2 + mn2;  // apus
  oraactuala = 100*ora + minut;  // ora actuala

  Serial.print(ora1);
  Serial.print(" ? ");
  Serial.print(oraactuala);
  Serial.print(" ? ");
  Serial.println(ora2);  

  if ((oraactuala > ora2) or (oraactuala < ora1))  // night 
    return true;  
    else
    return false;  
}
