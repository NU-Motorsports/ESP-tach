#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/twai.h>

// IO CONSTANTS
const byte TACH_PIN = 33;
const byte LED_PIN = 2;
// const byte ERROR_PIN = 9;
const byte MOD_SELECT = 22;
const byte MOD_SELECT_2 = 19;
const byte MOD_SELECT_3 = 15;
// CAN pins are configured in the setup_twai_driver() Function

// TACH CONFIGURATION CONSTANT
const byte NUM_OF_MAGNETS = 1;           // Number of magnets around specific shaft
const unsigned long TIMEOUT = 1000000;   // Time before value zeros out. Lower for fast response, higher for reading low rpms
// const byte NUM_OF_READINGS = 2;          // Number of readings to consider for smoothing

// Tach Variables 
volatile unsigned long last_measured_time = 0;
volatile unsigned long pulse_period = TIMEOUT + 1000;
// volatile unsigned long average_period = TIMEOUT + 1000;
unsigned int zero_debounce_extra = 0;
unsigned int last_measured_time_buffer = last_measured_time;
unsigned int current_time = 0;
unsigned int RPM = 0;

bool gearbox_tach() {
  return digitalRead(MOD_SELECT) == HIGH && digitalRead(MOD_SELECT_2) == HIGH;
}

bool engine_rpm() {
  return digitalRead(MOD_SELECT_3) == HIGH;
}

twai_message_t configure_message() {
  twai_message_t message;

  if (gearbox_tach()) {
    message.identifier = 0xC8;
  }

  else if (engine_rpm()) {
    message.identifier = 0xD0;
  }

  else if (digitalRead(MOD_SELECT) == LOW && digitalRead(MOD_SELECT_2) == LOW) {
    message.identifier = 0xA6;
  }
  
  else if(digitalRead(MOD_SELECT) == HIGH && digitalRead(MOD_SELECT_2) == LOW) {
    message.identifier = 0xC7;
  }

  // else if (digitalRead(MOD_SELECT) == LOW && digitalRead(MOD_SELECT_2) == HIGH) {
  else {
    message.identifier = 0xC6;
  }

  return message.data;
}

// Set up CAN driver
void setup_twai_driver() {      
  twai_general_config_t general_config={
    .mode = TWAI_MODE_NORMAL,
    .tx_io = (gpio_num_t)GPIO_NUM_5,
    .rx_io = (gpio_num_t)GPIO_NUM_4,
    .clkout_io = (gpio_num_t)TWAI_IO_UNUSED,
    .bus_off_io = (gpio_num_t)TWAI_IO_UNUSED,
    .tx_queue_len = 10, // 0 for non-transmitting
    .rx_queue_len = 65,
    .alerts_enabled = TWAI_ALERT_ALL,
    .clkout_divider = 0
  };

  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS(); // Set Bus Speed to 1000 kbit/s
  twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t error = twai_driver_install(&general_config, &timing_config, &filter_config);

  if (error == ESP_OK) {
    Serial.println("CAN Driver Installation Okay");
  } else {
    Serial.println("CAN Driver Installation Failed");
    return;
  }

  error = twai_start();

  if (error == ESP_OK) {
    Serial.println("CAN Driver Started");
  } else {
    Serial.println("CAN Driver Failed to Start");
  }

}

void pulseEvent(){
  pulse_period = esp_timer_get_time() - last_measured_time;
  last_measured_time = esp_timer_get_time();
}

void setup() {
  // Serial Setup
  Serial.begin(115200);
  
  // Pin Setup
  pinMode(TACH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(TACH_PIN), pulseEvent, FALLING);

  pinMode(MOD_SELECT, INPUT);
  digitalRead(MOD_SELECT);

  pinMode(MOD_SELECT_2, INPUT);
  digitalRead(MOD_SELECT_2);

  // CAN Setup
  setup_twai_driver();

  //Module Selection
  //if(modSelect==0&&modSelect2==0
  //const byte modSelect2 = 19;
  //const byte modSelect3 = 15;

  // Delay so no negative values occur right at startup
  delay(1000);
}

void loop() {
  last_measured_time_buffer = last_measured_time;  // Buffer time so that interrupt doesn't mess with the math
  current_time = esp_timer_get_time();

  if (current_time < last_measured_time_buffer) {
    last_measured_time_buffer = current_time;
    Serial.println("Inequality Triggered");
  }

  // Calculate Frequency
  float frequency = 10000000000 / pulse_period;

  // If frequency is too low => TIMEOUT
  if (pulse_period > TIMEOUT - zero_debounce_extra || current_time - last_measured_time_buffer > TIMEOUT - zero_debounce_extra) {
    frequency = 0;
    zero_debounce_extra = 2000; // Change the threshold a little so it doesn't bounce
  } else {
    zero_debounce_extra = 0; // Reset the threshold to the normal value so it doesn't bounce
  }

  RPM = ((frequency/10000)/(NUM_OF_MAGNETS)) * 60;

  twai_message_t message = configure_message();

  // if (RPM == 0) {
  //   message.data[0] = 0x00;
  //   message.data_length_code = 1;

  unsigned int num = RPM;
  int i = 0;

  // num = 3451

  while (num > 0) {
    int digit = num % 0x0a; // 0x0a = 19
    message.data[i] = digit;
    Serial.print(message.data[i]);
    Serial.print("   ");
    num /= 0x0a; 
    i += 1;
  }

  Serial.print(message.data_length_code);
  Serial.print(", ");
}


  // uint8_t n = RPM / 255;
  // uint8_t remainder = RPM % 255;

  // if(n == 0 && remainder == 0) {
  //   message.data_length_code = 0;
  //   message.data[0] = 0x00;
  // } else {
  //   message.data_length_code = n+1;
    
  //   for (int i = 0; i < n; i++) {
  //     message.data[i] = (uint8_t) 255;
  //   }
  //   message.data[n] = (uint8_t) (RPM % 255);
  // }

  
  //Queue message for transmission
  if(twai_transmit(&message, pdMS_TO_TICKS(1000))==ESP_OK) {
    Serial.print("RPM: ");
    Serial.print(RPM);
    Serial.print("   Data: ");
  } else {
        //printf("Failed to queue message for transmission\n");
  }

}
