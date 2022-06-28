
#define VERSION "0.04"
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
float vin;
int i,barh=SCREEN_HEIGHT/3;
#if defined(__SAMD21G18A__)
HardwareSerial &MySerial=SerialUSB;
#define ADCREF 1.0
#endif
#if defined(__AVR_ATmega328P__)
#define ADCREF 1.10
HardwareSerial &MySerial=Serial;
#include <EEPROM.h>
#endif

struct{
  uint16_t ever=0;
  uint16_t avgn=5;
  int adcadj=0;
  uint16_t order=2;
  float a=-3.99322588231898e-5,b=0.24201168460174,c=33.900362365621,d=0.0;
} parms;

void setup(){
  int dver;
  
  pinMode(A0,INPUT);
#if defined(__SAMD21G18A__)
  analogReference(INTERNAL1V0);
  analogReadResolution(12);
#endif
#if defined(__AVR_ATmega328P__)
  analogReference(INTERNAL);
#endif
  // start serial port at 9600 bps and wait for port to open:
  MySerial.begin(9600);
  while (!MySerial) {;} // wait for serial port to connect. Needed for native USB port only
  MySerial.println("Serial started.");
  //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    MySerial.println(F("SSD1306 allocation failed"));
    while(1); // loop forever
  }
  display.clearDisplay();
  display.setTextSize(2);// normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); //draw white text
  display.cp437(true);//use full 256 char 'Code Page 437' font
  display.print("pmd\nv");
  display.println(VERSION);
  display.display();
  delay(1000);
#if defined(__AVR_ATmega328P__) && defined(USEEEPROM)
  //get the data block from eeprom
  dver=EEPROM.read(0);
  if(dver!=1){
    MySerial.print(F("Unsupported EEPROM version: "));
    MySerial.println(dver);
    while(1);
  }
  EEPROM.get(0,parms);
  MySerial.println(F("Read EEPROM."));
#endif
  MySerial.print("adcadj: ");
  MySerial.println(parms.adcadj);
  MySerial.print("a: ");
  MySerial.println(parms.a,10);
  MySerial.print("b: ");
  MySerial.println(parms.b,10);
  MySerial.print("c: ");
  MySerial.println(parms.c,10);
  MySerial.print("d: ");
  MySerial.println(parms.d,10);
}

void loop() {
  int prec;
  float pwr,dbm;
  AdcAccumulator=0;
  for(i=parms.avgn;i--;){
    // read the value from the detector
    AdcAccumulator+=analogRead(sensorPin);
    delay(100);
    }
  // calculate average vin
  vin=(float)AdcAccumulator/(1024+parms.adcadj)*ADCREF/parms.avgn;
  if(vin<0.002)vin=0.0;
  pwr=parms.a+parms.b*vin+parms.c*pow(vin,2)+parms.d*pow(vin,3);
  if(pwr<0.002){
    pwr=0.0;
    dbm=-1;
    prec=3;
    }
  else{
    dbm=10*log10(pwr/0.001);
    prec=5-floor(dbm/10);
    }
  // print a message to the display.
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);      // 2:1 pixel scale
  if(pwr<0.002)
    display.print("<0.002 W");
  else{
    display.print(pwr,prec);
    display.print(" W");
    }
  display.setCursor(0,21);
  if(dbm>-1){
    display.print(dbm,1);
    display.print(" dBm ");
    int w=(dbm*2.5)+0.5;
    // Draw filled part of bar starting from left of screen:
    display.fillRect(0,display.height()-barh,w,barh,1);
    display.fillRect(w+1,display.height()-barh,display.width()-w,barh,0);
    for(int i=24;i<128;i=i+25)
      display.fillRect(i,display.height()-barh/2,1,barh/2,0);
  }
  else
    display.fillRect(0,display.height()-barh,display.width(),barh,0);

 display.display();
 // delay(500);
}

