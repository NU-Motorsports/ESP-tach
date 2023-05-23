#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/twai.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

// IO CONSTANTS
byte TACH_PIN = 7;
const byte HALL_PIN = 7;
const byte DIG_PIN = 8;
const byte LED_PIN = 2;
const byte ERROR_PIN = 3;
const byte MOD_SELECT = 0;
const byte MOD_SELECT_2 = 1;
const byte MOD_SELECT_3 = 19;
const byte UPDATE_PIN = 18; //TEMPORARY will be replaced by CAN
// CAN pins are configured in the setup_twai_driver() Function


//CAN Variables
unsigned int CAN_IDENTIFIER = 0x00;


// Tach Variables 
byte NUM_OF_MAGNETS = 1;           // Number of magnets around specific shaft
unsigned long TIMEOUT = 1000000;   // Time before value zeros out. Lower for fast response, higher for reading low rpms
volatile unsigned long last_measured_time = 0;
volatile unsigned long pulse_period = TIMEOUT + 1000;
unsigned int zero_debounce_extra = 0;
unsigned int last_measured_time_buffer = last_measured_time;
unsigned int current_time = 0;
unsigned int RPM = 0;


//OTA Update Variables
const char* host = "ESP32";
const char* ssid = "baja";
const char* password = "EatSand1";
byte OTA_SETUP_INDEX = 0;

//OTA Configuration
WebServer server(80);
/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";
/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>ESP32 Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='admin')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;


/*****************Funcions*********************/

//Module Select Funcions
bool engine_rpm() {
  return digitalRead(MOD_SELECT) == LOW && digitalRead(MOD_SELECT_2) == LOW && digitalRead(MOD_SELECT_3) == LOW;
}
bool gearbox_tach() {
  return digitalRead(MOD_SELECT)==LOW && digitalRead(MOD_SELECT_2)==LOW && digitalRead(MOD_SELECT_3)==HIGH;
}
bool driveshaft_tach() {
  return digitalRead(MOD_SELECT)==LOW && digitalRead(MOD_SELECT_2)==HIGH && digitalRead(MOD_SELECT_3)==LOW;
}
bool left_sprag_tach() {
  return digitalRead(MOD_SELECT)==LOW && digitalRead(MOD_SELECT_2)==HIGH && digitalRead(MOD_SELECT_3)==HIGH;
}
bool right_sprag_tach() {
  return digitalRead(MOD_SELECT) == HIGH && digitalRead(MOD_SELECT_2) == LOW && digitalRead(MOD_SELECT_3) == LOW;
}


//Tach Setup Functions
void configure_tach(){
  // if (digitalRead(MOD_SELECT) == LOW && digitalRead(MOD_SELECT_2) == LOW && digitalRead(MOD_SELECT_3) == LOW) {   //ENGINE TACH
  //   TACH_PIN = DIG_PIN;
  //   NUM_OF_MAGNETS = 1;
  //   CAN_IDENTIFIER = 0xD0;
  //   //host = "EngineTach";
  //   Serial.println("Engine");
  // } 
  // if (digitalRead(MOD_SELECT)==LOW && digitalRead(MOD_SELECT_2)==LOW && digitalRead(MOD_SELECT_3)==HIGH) {
  //   TACH_PIN = HALL_PIN;
  //   NUM_OF_MAGNETS = 3;
  //   CAN_IDENTIFIER = 0xC8;
  //   Serial.println("Gbox");
  //   //host = "GearboxTach";
  // } else if(driveshaft_tach()) {
  //   TACH_PIN = HALL_PIN;
  //   NUM_OF_MAGNETS = 2;
  //   CAN_IDENTIFIER = 0xCB;
  //   Serial.println("Driveshaft");
  //   //host = "DriveshaftTach";
  // } else if(left_sprag_tach()) {
  //   TACH_PIN = HALL_PIN;
  //   NUM_OF_MAGNETS = 3;
  //   CAN_IDENTIFIER = 0x13A;
  //   Serial.println("Left Sprag");
  //   //host = "LeftSpragTach";
  // } else if(right_sprag_tach()){
  //   TACH_PIN = HALL_PIN;
  //   NUM_OF_MAGNETS = 3;
  //   CAN_IDENTIFIER = 0x13B;
  //   Serial.println("Right Sprag");
  //   //host = "RightSpragTach";
  // }
  TACH_PIN = HALL_PIN;
  NUM_OF_MAGNETS = 3;
  CAN_IDENTIFIER = 0xC8;
  Serial.println("Gbox");
}



void setup_twai_driver() {      
  twai_general_config_t general_config={
    .mode = TWAI_MODE_NORMAL,
    .tx_io = (gpio_num_t)GPIO_NUM_5,    //CAN TX pin is GPIO 5
    .rx_io = (gpio_num_t)GPIO_NUM_4,    //CAN RX pin is GPIO 4
    .clkout_io = (gpio_num_t)TWAI_IO_UNUSED,
    .bus_off_io = (gpio_num_t)TWAI_IO_UNUSED,
    .tx_queue_len = 0, // 0 for non-transmitting
    .rx_queue_len = 65,
    .alerts_enabled = TWAI_ALERT_ALL,
    .clkout_divider = 0
  };

  twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS(); // Set Bus Speed to 500 kbit/s
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


//Tach Functions
void pulseEvent(){
  pulse_period = esp_timer_get_time() - last_measured_time;
  last_measured_time = esp_timer_get_time();
}


//OTA Functions
void OTA(){
  if(OTA_SETUP_INDEX==0){
    // Connect to WiFi network
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    /*use mdns for host name resolution*/
    if (!MDNS.begin(host)) { //http://esp32.local
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");
    /*return index page which is stored in serverIndex */
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", loginIndex);
    });
    server.on("/serverIndex", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
          Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });
    server.begin();
    OTA_SETUP_INDEX = 1;
  }

  server.handleClient();
  delay(1);
}



/***********************************Setup**********************************/
void setup() {
  // Serial Setup
  Serial.begin(115200);
  

  //Setup Tach
  configure_tach();


  //Setup CAN
  setup_twai_driver();


  // Pin Setup
  pinMode(TACH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(UPDATE_PIN, INPUT);
  pinMode(ERROR_PIN,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(TACH_PIN), pulseEvent, FALLING);

  if(digitalRead(GPIO_NUM_19)==HIGH){
    digitalWrite(ERROR_PIN,HIGH);
  }


  // Delay so no negative values occur right at startup
  delay(1000);
}



/**********************Loop*************************/
void loop() {
  // if(digitalRead(TACH_PIN) == LOW){
  //   digitalWrite(ERROR_PIN, HIGH);
  //   //Serial.println("Magnet");
  // } else{
  //   digitalWrite(ERROR_PIN,LOW);
  //   //Serial.println("No Magnet");
  // }
  
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

  RPM = ((frequency/10000)/(NUM_OF_MAGNETS)) * 60;    //Unit conversion and consideration for number of magnets
  
  //Serial.println(RPM);

  //CAN Message
  twai_message_t message;
  message.identifier = CAN_IDENTIFIER;

  unsigned int num = RPM;
  
  int i = 0;
  while (num >= 0) {
    uint8_t digit = num % 0x0a; // 0x0a = 10
    message.data[i] = digit;
    //Serial.print(message.data[i]);
    num /= 0x0a; // num /= 10
    i += 1;
    if (num == 0) {
      break;
    }
  }
  message.data_length_code = i;
  //Serial.println();
  //Serial.print("Data length of code is: ");
  //Serial.println(message.data_length_code);
  
  //Queue message for transmission
  if (twai_transmit(&message, pdMS_TO_TICKS(1000))== ESP_OK) {
    Serial.print(int(message.identifier));
    Serial.print(" is sending: [");
    for (int x = 0; x < i; x++) {
      Serial.print(message.data[x]);
    }
    Serial.println("]");
    
  } else {
    //Serial.println("Failed to queue message for transmission");
  }

  //if(digitalRead(UPDATE_PIN)==HIGH){
    //OTA();
  //}

}
