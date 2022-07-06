
#define VERSION "0.08"
#define USEEEPROM
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64// OLED display height, in pixels

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Owen Duffy 2022/06/22
//OLED variation
/*
  The circuit: see https://owenduffy.net/blog/?p=25710 .
 */

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);
int sensorPin=A0; // select the input pin for the detector
unsigned AdcAccumulator; // variable to accumulate the value coming from the sensor
float calfactor;
int i,barh=SCREEN_HEIGHT/3;
#if defined(__SAMD21G18A__)
HardwareSerial &MySerial=SerialUSB;
#endif
#if defined(__AVR_ATmega328P__)
HardwareSerial &MySerial=Serial;
#include <EEPROM.h>
#endif

struct{
  uint16_t ever=0;
  uint16_t avgn=4;
  float pmin=0.001;
  int adcoffsadj=0;
  int adcgainadj=0;
  uint16_t flags=0;
  float a=-3.99322588231898e-5,b=0.24201168460174,c=33.900362365621,d=0.0;
} parms;

void setup(){
  float adcref;
  long adcfs;
  int ever;
  
#if defined(__SAMD21G18A__)
  pinMode(A0,INPUT);
  analogReference(INTERNAL1V0);
  adcref=1.0;
  analogReadResolution(12);
  adcfs=4096;
#endif
#if defined(__AVR_ATmega328P__)
  pinMode(A0,INPUT);
  analogReference(INTERNAL);
  adcref=1.10;
  adcfs=1024;
#endif
  // start serial port at 9600 bps and wait for port to open:
  MySerial.begin(9600);
  while (!MySerial) {;} // wait for serial port to connect. Needed for native USB port only
  MySerial.println(F("Serial started."));
  //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    MySerial.println(F("SSD1306 allocation failed"));
    while(1); // loop forever
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(2);// normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); //draw white text
  display.cp437(true);//use full 256 char 'Code Page 437' font
  display.print(F("pmd\nv"));
  display.println(VERSION);
  display.display();
  delay(1000);
#if defined(__AVR_ATmega328P__) && defined(USEEEPROM)
  //get the data block from eeprom
  ever=EEPROM.read(0);
  if(ever!=1){
    MySerial.print(F("Unsupported EEPROM version: "));
    MySerial.println(ever);
    while(1);
  }
  EEPROM.get(0,parms);
  MySerial.println(F("Read EEPROM."));
#endif
  MySerial.print(F("pmin: "));
  MySerial.println(parms.pmin,6);
  MySerial.print(F("adcoffsadj: "));
  MySerial.println(parms.adcoffsadj);
  MySerial.print(F("adcgainadj: "));
  MySerial.println(parms.adcgainadj);
  MySerial.print(F("a: "));
  MySerial.println(parms.a,10);
  MySerial.print(F("b: "));
  MySerial.println(parms.b,10);
  MySerial.print(F("c: "));
  MySerial.println(parms.c,10);
  MySerial.print(F("d: "));
  MySerial.println(parms.d,10);
  calfactor=adcref/parms.avgn/(float)(adcfs-parms.adcgainadj);
}

void loop() {
  int prec;
  float vin,pwr,dbm;
  long AdcAccumulator=0;
  for(i=parms.avgn;i--;){
    // read the value from the detector
    AdcAccumulator+=analogRead(sensorPin);
    delay(100);
    }
  // calculate average vin
  vin=((float)AdcAccumulator+(float)parms.adcoffsadj*(float)parms.avgn)*calfactor;
  pwr=parms.a+parms.b*vin+parms.c*pow(vin,2)+parms.d*pow(vin,3);
  MySerial.print(F("AdcAccumulator: "));
  MySerial.print(AdcAccumulator);
  MySerial.print(F(", vin: "));
  MySerial.print(vin,3);
  MySerial.print(F(", pwr: "));
  MySerial.println(pwr,3);
  if(pwr<parms.pmin){
    pwr=0.0;
    dbm=-99;
    prec=3;
    }
  else{
    dbm=10*log10(pwr/0.001);
    prec=5-floor(dbm/10);
    }
  // print a message to the display.
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  if(pwr<parms.pmin)
    display.print(F("***W"));
  else{
    display.print(pwr,prec);
    display.print(F("W"));
    }
  display.setCursor(0,21);
  if(dbm>-98){
    display.print(dbm,1);
    display.print(F("dBm"));
    int w=(dbm*2.5)+0.5;
    // Draw filled part of bar starting from left of screen:
    display.fillRect(0,display.height()-1-barh,w,barh,1);
//    display.fillRect(w+1,display.height()-barh-1,display.width()-w,barh,0);
    for(int i=24;i<128;i=i+25)
      display.fillRect(i,display.height()-1-barh/2,1,barh/2,0);
      }
//    else
//      display.fillRect(0,display.height()-barh-1,display.width(),barh,0);
    display.display();
 // delay(500);
}

