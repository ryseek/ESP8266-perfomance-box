#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "index.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Set these to your desired credentials. */
const char *ssid = "ryseek's iPhone";
const char *password = "titan6679";

ESP8266WebServer server(80);


#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

static const int RXPin = 15, TXPin = 13;
static const uint32_t GPSBaud = 57600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(13,15, false, 512);

unsigned long dragTime100;
unsigned long dragTime60;
unsigned long dragTimeStart;
const int resultsCount = 100;
float results60[resultsCount];
float results100[resultsCount];

unsigned long framesTime;
unsigned long frames;

bool isDrag = false;
bool isDrag60 = false;
bool firstRun = true;
float oldSpeed;
int buzzerPin = 14;
int buttonPin = 12; 

void setup()
{
  Serial.begin(115200);
  ss.begin(GPSBaud);
  ss.println("$PMTK220,100*2F"); // $PMTK220,200*2C -  5hz rate // $PMTK220,100*2F - 10HZ
  ss.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"); // only GPS signal


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.display();
  Wire.setClock(800000L);
  
  pinMode(buzzerPin, OUTPUT);
  
  //server
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);

  if (MDNS.begin("esprace")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.begin();
  //pinMode(buttonPin, INPUT_PULLUP);
  //digitalWrite(buttonPin, HIGH);
}

void loop()
{
  server.handleClient();
  
  if (firstRun){
    framesTime = millis();
    firstRun = false;
  }
  //bool button = digitalRead(buttonPin);
  //Serial.println("Button: " + String(button));
  //gps.encode(ss.read());
  //printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
 
  //printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);

 
  //printInt(gps.charsProcessed(), true, 6);
  //printInt(gps.sentencesWithFix(), true, 10);
  //printInt(gps.failedChecksum(), true, 9);
  Serial.println(String(gps.charsProcessed()));
  
  

  String disp = " " + String(gps.speed.kmph()) + " km/h";
  String sats = String(gps.satellites.value());

          float currentSpeed = gps.speed.kmph();
          float deltaSpeed = currentSpeed - oldSpeed;
          if (currentSpeed > 1 && oldSpeed < 0 && isDrag == false){
            isDrag = true;
            isDrag60 = true;
            dragTimeStart = millis();
          }else if (currentSpeed > 100 && isDrag == true) {
            isDrag = false;
            dragTime100 = millis() - dragTimeStart;
            buzz();
            buzz();
            saveResults(dragTime60,dragTime100);
          }else if (currentSpeed > 2 && currentSpeed < 60 && isDrag == true){
            dragTime100 = millis() - dragTimeStart;
            dragTime60 = millis() - dragTimeStart;
          }else if (currentSpeed > 60 && currentSpeed < 100 && isDrag == true){
            dragTime100 = millis() - dragTimeStart;
            if (isDrag60) {
              buzz();
              isDrag60 = false;
            }
            
          }else if (currentSpeed < 1){
            isDrag = false;
            isDrag60 = false;
            dragTime100 = 0;
            dragTime60 = 0;
          }

          oldSpeed = currentSpeed;

          display.clearDisplay();
         
          display.setTextColor(WHITE);
          display.setCursor(0,0);
         
          display.setTextSize(2);
          display.println("" + String(currentSpeed) + "km/h");
         // display.println("isDrag : " + String(isDrag));
          display.println("60:" + String(float(dragTime60) / 1000) + "s");
          display.println("100:" + String(float(dragTime100) / 1000) + "s");

          String message = "";
          if (!gps.speed.isValid()) {
            message = "updating";
          } else {
            message = "connected";
          }

          float fps = frames * 1000 / (millis() - framesTime);
          
          
          display.setTextSize(1);
          display.println("Satellites: " + String(sats));
          display.println(message + ", FPS: " + fps);
          display.display();
          
  frames = frames + 1;
  
  smartDelay(5);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}

void buzz(){
  digitalWrite(buzzerPin, HIGH);
  delay(10);
  digitalWrite(buzzerPin, LOW);
  delay(50);
}

void handleRoot() {
  String HTML = topPageHTML;
  HTML += createResultsTableHTML();
  HTML += middlePageHTML;
  HTML += createOverallTableHTML();
  HTML += bottomPageHTML;
  server.send(200, "text/html", HTML);
  
  Serial.println();
}

void saveResults(float time60, float time100){
  int i;
  for(i=resultsCount-1;i>-1;i--){
    if (results60[i] == results100[i] && results60[i] == float(0)){
      results60[i] = time60;
      results100[i] = time100;
      break;
    }
  }
}

String createResultsTableHTML() { 
  int i;
  String HTML = "";
  for(i=0;i<resultsCount;i++){
      if (results60[i] == results100[i] && results60[i] == float(0))
        continue;
      HTML += "<tr> <td><i class='fa fa-clock-o w3-text-black w3-large'></i></td>";
      HTML += "<td><i>" + String(results60[i]) + "</i></td> <td><i>"+ String(results100[i]) + "</i></td> </tr>";
  }
  return HTML;
}

String createOverallTableHTML() { 
  int i;
  String HTML = "";
  int sats = gps.satellites.value();
  
  //Satellites
  HTML += "<div class='w3-quarter'>";
  HTML += "   <div class='w3-container w3-green w3-padding-16'>";
  HTML += "       <div class='w3-left'><i class='fa fa-rss w3-xxxlarge'></i></div>";
  HTML += "         <div class='w3-right'>";
  HTML += "           <h1>" + String(sats) + "</h1>";
  HTML += "         </div>";
  HTML += "        <div class='w3-clear'></div>";
  HTML += "           <h4>Satellites</h4>";
  HTML += "     </div>";
  HTML += " </div>";
  
  //runs
  int runs = resultsCount;
  for(i=0;i<resultsCount;i++){
     if (results60[i] == results100[i] && results60[i] == float(0))
        runs--;
  }
  HTML += "<div class='w3-quarter'>";
  HTML += "   <div class='w3-container w3-blue w3-padding-16'>";
  HTML += "       <div class='w3-left'><i class='fa fa-road w3-xxxlarge'></i></div>";
  HTML += "         <div class='w3-right'>";
  HTML += "           <h1>" + String(runs) + "</h1>";
  HTML += "         </div>";
  HTML += "        <div class='w3-clear'></div>";
  HTML += "           <h4>Runs</h4>";
  HTML += "     </div>";
  HTML += " </div>";

  return HTML;
}


// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec)
{
  if (!valid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }
  
  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartDelay(0);
}
