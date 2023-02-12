#include <Arduino.h>


//IO Variables
const byte tachPin = 8;
const byte ledPin = 2;
const byte errorPin = 9;

//Tach Configuration
const byte numMagnets = 2;                          //Number of magnets around specific shaft
const unsigned long zeroTimeout = 100000;           //Time before value zeros out. Lower for fast response, higher for reading low rpms
const byte numReadings = 2;                         //Number of readings to consider for smoothing

//Tach Variables 
volatile unsigned long lastMeasuredTime = 0;
volatile unsigned long pulsePeriod = zeroTimeout+1000;
volatile unsigned long averagePeriod = zeroTimeout+1000;



void pulseEvent(){
  pulsePeriod = esp_timer_get_time() - lastMeasuredTime;
  lastMeasuredTime = esp_timer_get_time();
}



void setup() {
  //Serial Setup
  Serial.begin(115200);
  
  //Pin Setup
  pinMode(tachPin,INPUT);
  pinMode(ledPin,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(tachPin), pulseEvent, FALLING);

  delay(1000);  //Delay so no negative values occur right at startup

}



void loop() {
  // put your main code here, to run repeatedly:
}