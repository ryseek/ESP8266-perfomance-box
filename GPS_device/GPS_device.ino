#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


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
  WiFi.disconnect(); 
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.display();
  Wire.setClock(800000L);
  
  pinMode(buzzerPin, OUTPUT);
  //pinMode(buttonPin, INPUT_PULLUP);
  //digitalWrite(buttonPin, HIGH);
}

void loop()
{

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

          float speed = gps.speed.kmph();
          float deltaSpeed = speed - oldSpeed;
          if (speed > 2 && deltaSpeed > 0 && speed < 30 && isDrag == false){
            isDrag = true;
            isDrag60 = true;
            dragTimeStart = millis();
          }else if (speed > 100 && isDrag == true) {
            isDrag = false;
            dragTime100 = millis() - dragTimeStart;
            buzz();
            buzz();
          }else if (speed > 2 && speed < 60 && isDrag == true){
            dragTime100 = millis() - dragTimeStart;
            dragTime60 = millis() - dragTimeStart;
          }else if (speed > 60 && speed < 100 && isDrag == true){
            dragTime100 = millis() - dragTimeStart;
            if (isDrag60) {
              buzz();
              isDrag60 = false;
            }
            
          }else if (speed < 2){
            isDrag = false;
            isDrag60 = false;
            dragTime100 = 0;
            dragTime60 = 0;
          }

          oldSpeed = speed;

          display.clearDisplay();
         
          display.setTextColor(WHITE);
          display.setCursor(0,0);
         
          display.setTextSize(2);
          display.println(" " + String(speed) + "km/h");
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
