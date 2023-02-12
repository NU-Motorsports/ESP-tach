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
unsigned int zeroDebounceExtra = 0;
unsigned int lastMeasuredTimeBuffer = lastMeasuredTime;
unsigned int currentTime = 0;


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

  //Delay so no negative values occur right at startup
  delay(1000);
}



void loop() {
  //
  lastMeasuredTimeBuffer = lastMeasuredTime;          //Buffer time so that interrupt doesn't mess with the math
  currentTime = esp_timer_get_time();

  int frequency = 10000000000/pulsePeriod; 
  if(pulsePeriod > zeroTimeout - zeroDebounceExtra || currentTime - lastMeasuredTimeBuffer > zeroTimeout - zeroDebounceExtra){
    frequency = 0;                                    // Set frequency as 0
    zeroDebounceExtra = 2000;                         // Change the threshold a little so it doesn't bounce
  }else{
    zeroDebounceExtra = 0;                            // Reset the threshold to the normal value so it doesn't bounce
  }


}