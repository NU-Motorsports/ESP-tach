#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/twai.h>


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


void setup_twai_driver(){      //Function for setting up CAN driver
  twai_general_config_t general_config={
    .mode = TWAI_MODE_NORMAL,
    .tx_io = (gpio_num_t)GPIO_NUM_5,
    .rx_io = (gpio_num_t)GPIO_NUM_4,
    .clkout_io = (gpio_num_t)TWAI_IO_UNUSED,
    .bus_off_io = (gpio_num_t)TWAI_IO_UNUSED,
    .tx_queue_len = 0,     //0 for non transmitting
    .rx_queue_len = 65,
    .alerts_enabled = TWAI_ALERT_ALL,
    .clkout_divider = 0
  
  };

  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();   //Set Bus Speed to 1000 kbit/s
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
}

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

  //CAN Setup
  setup_twai_driver();

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

  //Configure message to transmit
  twai_message_t message;
  message.identifier = 0xC8;
  //message.flags = TWAI_MSG_FLAG_EXTD;
  message.data_length_code = 2;
  for (int i = 0; i < 4; i++) {
      message.data[i] = 0;
  }

  //Queue message for transmission
  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
      printf("Message queued for transmission\n");
  } else {
      printf("Failed to queue message for transmission\n");
  }


}