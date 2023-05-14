/* original design by niq_ro: https://github.com/tehniq3/
 * v.1 - test for have a stable version
 * v.2.0 - changed almost all.. e.g from FastLED to Adafruit_NeoMatrix libraries
 * v.2.1 - added weather info (scrolling) from openweathermap
 * v.2.1.a - compute lengh of string + extract more info from opemweathermap data
 * v.2.2 - sunset/sunrise time: not used anymore data from SolaCalculator library, used opemweathermap data
 * v.2.3 - weather info shown rarely (5 minutes, for example) and not in the midle of the night (00-05 hour)
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
//#include <SolarCalculator.h> //  https://github.com/jpb10/SolarCalculator
#include <ArduinoJson.h>

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
#define NIGHTBRIGHTNESS 2      // Brightness level from 0 (off) to 255 (full brightness)
#define DAYBRIGHTNESS 20

const long timezoneOffset = 2; // ? hours
const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed

//weather variables
WiFiClient client;
char servername[]="api.openweathermap.org";              // remote weather server we will connect to
String result;
String APIKEY = "ca55295c4a4b9a681dce2688e0751dde";                                 
String CityID = "680332"; // Craiova city                       
int  fetchweatherflag = 0; 
String weatherDescription ="";
String weatherLocation = "";
String Country;
float temperatura=0.0;
int tempint, temprest;
float tempmin, tempmax;
int umiditate;
int presiune;
/*
unsigned long tpafisare;
unsigned long tpinfo = 60000;  // 15000 for test in video
*/
unsigned long tpvreme;
unsigned long tpinterogare1 = 600000;

unsigned long tpsoare;
unsigned long tpinterogare2 = 60000;

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

byte aratadata = 0;
byte culoare = 1;

int vant, directie, nori;
unsigned long unixora, unixora1, unixrasarit, unixapus;
String descriere;
byte dn;
byte night1 = 1; // 0:00, 1:00, etc
byte night2 = 5; // 5:00, 6:00
byte minscroll = 5;  // minutes between info scrolling

void setup() {
  Serial.begin(115200);
  pinMode (DSTpin, INPUT);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(NIGHTBRIGHTNESS);  // original 255
  matrix.setTextColor(colors[6]);
  matrix.fillScreen(0);
matrix.setCursor(0, 0);
matrix.print(F("v.2.3"));
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
  
checkTime();
getWeatherData();
verificaresoare();
if (dn == 0)
      {
        matrix.setBrightness (NIGHTBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      matrix.setBrightness (DAYBRIGHTNESS);
      }
    matrix.show();
tpsoare = millis();
tpvreme = millis();
}

void loop() {
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (WiFi.status() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
   updated = timeClient.update();
    if (updated) {
    checkTime();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }
 
if (millis() - tpvreme >= tpinterogare1)
{
getWeatherData();
tpvreme = millis();  
Serial.println("Weather site is check !");
}

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
//Soare();
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
//int ldata = 6*10; 
int ldata = 6*data.length(); // https://reference.arduino.cc/reference/en/language/variables/data-types/string/functions/length/

String tempe = "      Weather: ";
if (dn == 1)
   tempe = tempe + "Day, ";
   else
   tempe = tempe + "Night, ";
tempe = tempe + descriere;
if (nori > 0) 
tempe = tempe + " (clouds: " + nori + "%)";
tempe = tempe + ", temperature: ";
if (temperatura > 0) tempe = tempe + "+";
tempe = tempe + tempint + ","+ temprest;
tempe = tempe + ((char)247) + "C, humidity: ";  // https://forum.arduino.cc/t/degree-symbol/166709/9
/*
tempe = tempe + ((char)247) + "C (";
tempe = tempe + tempmin + "/";
tempe = tempe + tempmax + "), humidity: ";
*/
tempe = tempe + umiditate;
tempe = tempe + "%RH, pressure: ";
tempe = tempe + presiune + "mmHg, wind: ";
tempe = tempe + vant + "km/h (";
tempe = tempe + directie +  ((char)247) + ") ";
//int ltempe = 6*56;  // 6*72;
int ltempe = 6*tempe.length(); // https://reference.arduino.cc/reference/en/language/variables/data-types/string/functions/length/
//x = matrix.width();


if ((ora <= night1) and (ora > night2))  // if can be shown the extra info
  {
    Serial.print("/");
    delay(100);
  }
else
if ((minut%minscroll == 0) and (aratadata == 1))  // minscroll = 5 -> every 5 minutes
{
matrix.fillScreen(0);
  for (x; x > -ltempe ; x--)
  {
  matrix.fillScreen(0); 
  matrix.setCursor(x, 0);
  matrix.print(tempe);  
  matrix.show();
  delay(75);
//  Serial.println(x);
 }
  aratadata = 0; 
  Serial.println("show -> 0");
  culoare++;
  if (culoare > 6) culoare = 0;
}

if (millis() - tpsoare >= tpinterogare2)
{
verificaresoare();
if(minut%minscroll >1)
{
  aratadata = 1;  // next minutes 
  Serial.println("show -> 1");
}
      if (dn == 0)
      {
        matrix.setBrightness (NIGHTBRIGHTNESS);
        Serial.println("NIGHT !");
      }
      else
      {
      matrix.setBrightness (DAYBRIGHTNESS);
      }
      matrix.show();
tpsoare = millis();  
}

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


  void getWeatherData()     //client function to send/receive GET request data.
{
  if (client.connect(servername, 80))   
          {                         //starts client connection, checks for connection
          client.println("GET /data/2.5/weather?id="+CityID+"&units=metric&APPID="+APIKEY);
          client.println("Host: api.openweathermap.org");
          client.println("User-Agent: ArduinoWiFi/1.1");
          client.println("Connection: close");
          client.println();
          } 
  else {
         Serial.println("connection failed");        //error message if no client connect
          Serial.println();
       }

  while(client.connected() && !client.available()) 
  // delay(1);  
  yield();//waits for data
  while (client.connected() || client.available())    
       {                                             //connected or data available
         yield(); // https://stackoverflow.com/questions/75228952/soft-wdt-reset-on-esp8266-when-using-speed-sensor
         char c = client.read();                     //gets byte from ethernet buffer
         result = result+c;
       }

client.stop();                                      //stop client
result.replace('[', ' ');
result.replace(']', ' ');
Serial.println(result);
char jsonArray [result.length()+1];
result.toCharArray(jsonArray,sizeof(jsonArray));
jsonArray[result.length() + 1] = '\0';
StaticJsonBuffer<1024> json_buf;
JsonObject &root = json_buf.parseObject(jsonArray);

if (!root.success())
  {
    Serial.println("parseObject() failed");
  }

String location = root["name"];
String icon = root["weather"]["icon"];    // "weather": {"id":800,"main":"Clear","description":"clear sky","icon":"01n"} 
String id = root["weather"]["description"]; 
String description = root["weather"]["description"];
int clouds = root["clouds"]["all"]; // "clouds":{"all":0},
String country = root["sys"]["country"];
float temperature = root["main"]["temp"];
float temperaturemin = root["main"]["temp_min"];
float temperaturemax = root["main"]["temp_max"];
float humidity = root["main"]["humidity"];
String weather = root["weather"]["main"];
//String description = root["weather"]["description"];
float pressure = root["main"]["pressure"];
float wind = root["wind"]["speed"];//"wind":{"speed":2.68,"deg":120},
int deg = root["wind"]["deg"];

// "sys":{"type":2,"id":50395,"country":"RO","sunrise":1683428885,"sunset":1683480878},
unixrasarit = root["sys"]["sunrise"];
unixapus = root["sys"]["sunset"];

//weatherDescription = description;
weatherDescription = weather;
weatherLocation = location;
Country = country;
temperatura = temperature;
tempint = (int)(10*temperatura)/10;
temprest = (int)(10*temperatura)%10;
tempmin = temperaturemin;
tempmax = temperaturemax;
umiditate = humidity;
presiune = pressure*0.75006;  // mmH20
vant = (float)(3.6*wind+0.5); // km/h
directie = deg;
nori = clouds;
descriere = description;


Serial.print(weather);
Serial.print(" / ");
Serial.println(description);
Serial.print(temperature);
Serial.print("°C / ");  // as at https://github.com/tehniq3/datalloger_SD_IoT/
Serial.print(umiditate);
Serial.print("%RH, ");
// check https://github.com/tehniq3/matrix_clock_weather_net/blob/master/LEDMatrixV2ro2a/weather.ino
Serial.print(presiune);
Serial.print("mmHg / wind = ");
Serial.print(vant);
Serial.print("km/h ,  direction = ");
Serial.print(deg);
Serial.println("° ");
Serial.print("ID weather = ");
Serial.print(id);
Serial.print(" + clouds = ");
Serial.print(nori);
Serial.print("% + icons = ");
Serial.print(icon);
Serial.println(" (d = day, n = night)");
}

void verificaresoare()
{
unixora = timeClient.getEpochTime();
Serial.print("unixtime = ");
Serial.print(unixora);
unixora1 = unixora - (timezoneOffset + DST) * 3600;
Serial.print(" -> unixtime1 = ");
Serial.print(unixora1);
Serial.print("   sunrise = ");
Serial.print(unixrasarit);
Serial.print(" / sunset = ");
Serial.print(unixapus); 
Serial.print(" => ");
  if ((unixora1 < unixrasarit) or (unixora1 > unixapus)) 
  dn = 0;
  else
  dn = 1; 
  Serial.println(dn);
 Serial.print(" (0 = night / 1 = day ! ");
 Serial.println(" -------------------");
}

void checkTime()
{
     updated = timeClient.update();
    if (updated) {
      Serial.println("NTP time updated.");
      Serial.print(timeClient.getFormattedTime());
      Serial.print(" ->");
      Serial.println(timeClient.getEpochTime());
      unixora = timeClient.getEpochTime();
      Serial.print(" =? ");
      Serial.println(unixora);
      getYear();
      getMonth();
      zi = timeClient.getDay();
      lastUpdatedTime = millis();
      verificaresoare();
    }
}
