#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <EEPROM.h>
#define EEPROM_SIZE sizeof(int)

#define MAX_REVERSE 1900 // Max pedal reading for reversin. So far readings are between 850 and 2950
int THROTTLE_FADE=250;
//Display libs
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
char buffer[255];

//Temp Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
#define DS18B20PIN 4
OneWire oneWire(DS18B20PIN);
DallasTemperature sensors(&oneWire);

//MULTITASK
void TaskUpdateSensors( void *pvParameters );
void TaskUpdateDisplay( void *pvParameters );
int tempinC;

//ESC controll LIbs
#include <ESP32Servo.h> // ESP32Servo library installed by Library Manager
#include "ESC.h" // RC_ESC library installed by Library Manager
#define ESC1_PIN (25) // connected to ESC control wire
#define ESC2_PIN (26) // connected to ESC control wire
#define LED_BUILTIN (2) // not defaulted properly for ESP32s/you must define it
// Note: the following speeds may need to be modified for your particular hardware.
int fadedPedalValue = 860;
int programmingMode = 0;
int  PWM_LOW = 1000; // minimum PWM Signal
int  MIN_SPEED = PWM_LOW + PWM_LOW / 2; // speed just slow enough to turn motor off
#define MAX_SPEED 2000 // speed where my motor drew 3.6 amps at 12v.
//bATTERY sTATUS
#define batPin (35)
int batteryValue = 0;
//OTA
const char* ssid = "WIFI_SSID";
const char* password = "password";

//Acelerator Pedal PIN
const int pedalPin = 34;
// variable for storing the PEdal value
int pedalValue = 1;

//REVERSE SIGNAL
#define REVERSE_PIN 18 // GIOP18 pin connected to button
#define FORWARD_PIN 17 // GIOP17 pin connected to button
#define PROGRAMMING_PIN 15 // Pin to setup thtottle fade and others
int goForward = 0;
int goReverse = 0;

int batteryPercentage = 0;
//cpu sppeed
uint32_t Freq = 0;


// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

ESC myESC1 (ESC1_PIN, 1000, 2000, 500); // ESC_Name (PIN, Minimum Value, Maximum Value, Arm Value)
ESC myESC2 (ESC2_PIN, 1000, 2000, 500); // ESC_Name (PIN, Minimum Value, Maximum Value, Arm Value)

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long counter = 0;
unsigned long iterations = 0;
const long interval = 1000;  // interval at which to blink (milliseconds)
int ledState = LOW;  // ledState used to set the LED
void setup() {

  pinMode(led, OUTPUT);  
  Serial.begin(115200);
  Serial.println("Booting");

  EEPROM.begin(EEPROM_SIZE);
  //THROTTLE_FADE = 
  EEPROM.get(0,THROTTLE_FADE);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //Display writing
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Attempting WIFI");
  display.display(); 

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    static int cnt = 0;
    delay(500);
    cnt++;
    if (cnt > 10) {
      display.println("Giving up on Wifi");
      display.display(); 
      break;
    }
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("ESP32_Car_ESC");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      //Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      //Serial.printf("Error[%u]: ", error);
      //if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      //else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      //else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      //else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      //else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  if(WiFi.waitForConnectResult() == WL_CONNECTED){
    display.print("IP address:");
    display.println(WiFi.localIP());
    display.display(); 
    ArduinoOTA.begin();
  }

  
  


  //scale down CPU
  setCpuFrequencyMhz(160);//valid numers are 240/160/80/(40/20/10) //https://deepbluembedded.com/esp32-change-cpu-speed-clock-frequency/
  Freq = getCpuFrequencyMhz();

 //ESC PIN
  pinMode(ESC1_PIN, OUTPUT);
  pinMode(ESC2_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  sprintf(buffer, "CPU Frequency:%d", Freq);
  display.println(buffer);
 
  display.println("Arming ESC");
  display.display(); 
  myESC1.arm(); // Send the Arm command to ESC
  myESC2.arm(); // Send the Arm command to ESC
  delay(5000); // Wait a while
  digitalWrite(LED_BUILTIN, LOW); // led off to indicate arming completed
  display.display(); 
  pinMode(REVERSE_PIN, INPUT_PULLUP);
  pinMode(FORWARD_PIN, INPUT_PULLUP);
  pinMode(PROGRAMMING_PIN, INPUT_PULLUP);
  display.println("Starting Temp Sensor");
  display.display();
  sensors.begin();

  display.println("Setup Complete.");
  display.display();

  //multitask
  Serial.println("Task 1");
  xTaskCreatePinnedToCore(
    TaskUpdateSensors
    ,  "TaskUpdateSensors"   // A name just for humans
    ,  8192  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  1);
  Serial.println("task2");
  xTaskCreatePinnedToCore(
    TaskUpdateDisplay
    ,  "TaskUpdateDisplay"   // A name just for humans
    ,  8192  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  1); 
Serial.println("Tasks sent");
}
void loop() {
  
  // Display static text
  //Serial.println("Reading PEdal");
  pedalValue = analogRead(pedalPin);
  //Serial.println("pedalValue");
  //Serial.println("Check Temp");
  if (tempinC > 70){
    if(tempinC > 80){
      pedalValue=860; //display.println("Temperature Shuttoff");
    }else if(tempinC >75){
      if(pedalValue > 1900) {
        pedalValue=120+0;
      } 
      //display.println("Temperature Critical");
    }else {
      //display.println("Temperature Warning!");
      if(pedalValue > 1900) {
        pedalValue=1900;
      }
    }
  }
  //Serial.println("Check Battery");
  if (batteryPercentage < 20){
    if(batteryPercentage < 1){
      pedalValue=860;
      //display.println("Battery too Low");
    }else if(batteryPercentage < 10){
      if(pedalValue > 1900) {
        pedalValue=1200;
      } 
      //display.println("Battery Critical");
    }else {
      //display.println("Battery Warning!");
      if(pedalValue > 1900) {
        pedalValue=1900;
      }
    }
  }
  //Serial.println("Apply PEdal fade");
  //pedal value varies between 860 and 1200 at rest
  if (pedalValue < 1200 ){
    pedalValue=860;
  }else if (pedalValue > 2920 ){
    pedalValue=2940;
  }
  if ( pedalValue >  fadedPedalValue + THROTTLE_FADE ){
      fadedPedalValue+=THROTTLE_FADE; 
  }else if ( pedalValue < fadedPedalValue - (THROTTLE_FADE*3)) {  //decelarate faster
      fadedPedalValue-=THROTTLE_FADE*3;
  }else{
      fadedPedalValue=pedalValue;
  }
  //Serial.println("Send to ESC");
  //sprintf(buffer, "Ped:%4d Fad:%d", pedalValue, fadedPedalValue);
  //display.println(buffer);
  //display.display();
  if ((goForward == HIGH) && (goReverse == LOW)){
    //ESC commands
    pedalValue = map(fadedPedalValue, 860, 2940, 1500, 2000); // scale forward motor
    myESC2.speed(pedalValue); // sets the ESC speed
    myESC1.speed(pedalValue); // sets the ESC speed
    //pedalValue = map(fadedPedalValue, 860, 2940, 1500, 1000); // scale inverse motor
    //myESC1.speed(pedalValue); // sets the ESC speed
  }
  else if ((goReverse == HIGH) && (goForward == LOW)){
    //ESC commands
    //Limit Reverse speed
    if (MAX_REVERSE > 0 && fadedPedalValue > MAX_REVERSE) {
      fadedPedalValue = MAX_REVERSE;
    }
    pedalValue = map(fadedPedalValue, 860, 2940, 1500, 1000); // scale forward motor
    myESC2.speed(pedalValue); // sets the ESC speed
    myESC1.speed(pedalValue); // sets the ESC speed
    //pedalValue = map(fadedPedalValue, 860, 2940, 1500, 2000); // scale reverse motor
    //myESC1.speed(pedalValue); // sets the ESC speed
  }
  else {
    myESC1.speed(1500); // sets the ESC speed
    myESC2.speed(1500); // sets the ESC speed
    pedalValue=1500;
  }
  //Serial.println("Check Milis");
  //Programming Mode
  unsigned long currentMillis = millis();
  counter++;
  if (currentMillis - previousMillis >= interval) {
    //Serial.println("milis");
    previousMillis = millis();
    iterations = counter;
    counter=0;
  }
}

void TaskUpdateSensors(void *pvParameters)  { //task
  (void) pvParameters;
  
  for (;;)  {
    if(WiFi.waitForConnectResult() == WL_CONNECTED){
      ArduinoOTA.handle();  
    }
    sensors.requestTemperatures(); 
    tempinC = sensors.getTempCByIndex(0);
    batteryValue=analogRead(batPin);
    batteryPercentage=map(batteryValue,2000,3320,0,100);
    //Serial.print("Sensors stack usage: ");
    //Serial.println(uxTaskGetStackHighWaterMark( NULL ));
  }
}

void TaskUpdateDisplay(void *pvParameters)  { //this is a task
  (void) pvParameters;
  for (;;) {
    //Serial.print("programmingMode:");
    //Serial.println(programmingMode);
    //if(programmingMode==100) continue; //do not paint the screen while in programming mode.
    //Switch Position
    goForward = digitalRead(FORWARD_PIN);
    goReverse = digitalRead(REVERSE_PIN);

    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("T");
    display.print(tempinC);
    display.print("C B");
    display.print(batteryPercentage);
    display.println("%");
    //sprintf(buffer, "T:%2dC B:%d%%", tempinC, batteryPercentage);
    //display.println(buffer);
    display.setTextSize(1);
    if (tempinC > 70){
      if(tempinC > 80){
        display.println("Temperature Shuttoff");
      }else if(tempinC >75){
        display.println("Temperature Critical");
      }else {
        display.println("Temperature Warning!");
      }
    }else if (tempinC < 10) {
      display.println("Sensor disconnected ?");
    }
    if (batteryPercentage < 20){
      if(batteryPercentage < 1){
        display.println("Battery too Low");
      }else if(batteryPercentage < 10){
        display.println("Battery Critical");
      }else {
        display.println("Battery Warning!");
      }
    }
    display.setTextSize(2);
    if ((goForward == HIGH) && (goReverse == LOW)){
      display.println("Forward");
    }else if ((goReverse == HIGH) && (goForward == LOW)){
      display.println("Reverse");
    }else {
      display.println("Neutral");
    }
    display.setTextSize(1);
    sprintf(buffer, "It:%d Fa:%d,Ba:%d", iterations, THROTTLE_FADE, batteryValue);
    display.println(buffer);
    display.display();  
    //Serial.print("Display stack usage: ");
    //Serial.println(uxTaskGetStackHighWaterMark( NULL ));
    if(digitalRead(PROGRAMMING_PIN) == LOW){
      //Serial.println("propinread");
      programmingMode++;
      if(programmingMode > 100 && goReverse == LOW && goForward == LOW){
        iterations=0;
        while(iterations < 20){
          display.clearDisplay();
          display.setCursor(0,0);
          display.println("Programming Mode. Use Button to Switch");
          display.print("Current THROTTLE_FADE:");
          display.println(THROTTLE_FADE);
          display.println("Options: 1,10,50,100,250");
          display.display();
          delay(500);
          if(digitalRead(PROGRAMMING_PIN) == LOW){
            if(THROTTLE_FADE==1){
              THROTTLE_FADE=10;
              iterations=0;
            }else if(THROTTLE_FADE == 10){
              THROTTLE_FADE=50;
              iterations=0;
            }else if(THROTTLE_FADE == 50){
              THROTTLE_FADE = 100;
              iterations=0;
            }else if(THROTTLE_FADE == 100){
              THROTTLE_FADE=250;
              iterations=0;
            }else {
              THROTTLE_FADE=1;
              iterations=0;
            }
          }else {
            iterations++;
          }
          if(iterations >= 20){
            EEPROM.put(0, THROTTLE_FADE);
            EEPROM.commit();
            programmingMode=0;
            display.println("Setting Saved");
            display.display();
            break;
          }
        }
      }
    }
  }
}
