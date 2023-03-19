#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>


#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <credentials.h>
bool ota_active = false;

//Display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Lights
#define N_PIXELS  2 // Number of pixels in strand
#define LED_PIN  D6 // NeoPixel LED strand is connected to this pin
#define TOP_LED   1
#define BOTTOM_LED 0
#define BOTTOM_BRIGHTNESS 70
#define TOP_BRIGHTNESS 140
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);


//Serial String
const int BUFFER_SIZE = 168;
char buf[BUFFER_SIZE];

//Serial
#define BAUDRATE_COUNT 7
uint32_t baudrate[BAUDRATE_COUNT] = {1200, 4800, 9600, 19200, 57600, 115200, 500000};
uint8_t baudrate_index = 2;

//Statemachine
typedef enum { START_SCREEN, SERIAL_READ, CHANGE_BAUD } state_t ;
state_t state = START_SCREEN;
bool first = true;

//Buttons
const uint8_t buttonPin = D0;


//Function vars
//void OLEDBlinkBaud(void);
unsigned long previousMillis_1 = 0;
const long interval_1 = 200;
bool blink = false;

//void ShortPress(void);
bool shortPress = false;

//void LongPress(void);
long buttonTimer = 0;
long longPressTime = 250;
bool buttonActive = false;
bool longPressActive = false;
bool longPress = false;



void setup()
{
  pinMode(buttonPin, INPUT_PULLDOWN_16);
  pixels.begin();
  pixels.setBrightness(20);
  Serial.begin(baudrate[baudrate_index]);
  if(digitalRead(buttonPin) == HIGH)
  {
    pixels.setPixelColor(BOTTOM_LED,colorPicker(0, BOTTOM_BRIGHTNESS));
    pixels.setPixelColor(TOP_LED,colorPicker(0, TOP_BRIGHTNESS));
    pixels.show();
    ota_active = true;
    setupOTA("SerialMon", mySSID, myPASSWORD);
  }
  OLEDInit();
}


void loop()
{
  statemachine();
  if(ota_active)
  {
    ArduinoOTA.handle();
  }
}

void statemachine(void)
{
  switch (state)
  {
    case START_SCREEN:
      if(first)
      {
        first = false;
        OLEDStart();
		    break;
      }
      if (Serial.available() > 0)
      {
        first = true;
        state = SERIAL_READ;
        break;
      }
      if(digitalRead(buttonPin) == HIGH)
      {
        first = true;
        state = CHANGE_BAUD;      
        break;
      }
      break;

    case SERIAL_READ: 
      OLEDSerialWrite();
      if(digitalRead(buttonPin) == HIGH)
      {
        first = true;
        state = CHANGE_BAUD; 
      }
      break;

    case CHANGE_BAUD:
      if(first)
      {
        first = false;
        OLEDChangeBaud();
		    break;
      }
      if(digitalRead(buttonPin) == HIGH) 
      {
        if (buttonActive == false)
        {
          buttonActive = true;
          buttonTimer = millis();
        }
        if(longPressActive == false) {
          if ((millis() - buttonTimer > longPressTime)) {
            longPressActive = true;
            longPress = true;
          }
        } else {
          OLEDBlinkBaud();
        }
      } else
      {
        if (buttonActive == true) {
			    if (longPressActive == true) {
            longPressActive = false;
			    } else {
				    shortPress = true;
			    }
			    buttonActive = false;
		    }
        if(longPress)
        {
          longPress = false;
          LongPress();
          break;
        }
        if(shortPress)
        {
          shortPress = false;
          ShortPress();
          break;
        }
      }
      break;
  }
}

void ResetLed(void)
{
  pixels.setPixelColor(BOTTOM_LED,colorPicker(baudrate_index, BOTTOM_BRIGHTNESS));
  pixels.setPixelColor(TOP_LED,colorPicker(baudrate_index, TOP_BRIGHTNESS));
  pixels.show();
}


void ShortPress(void)
{
  if( baudrate_index < BAUDRATE_COUNT-1 )
  {
    baudrate_index++;       
  } else
  {
    baudrate_index = 0;
  }
  pixels.setPixelColor(TOP_LED,colorPicker(baudrate_index, TOP_BRIGHTNESS));
  pixels.show();            
  OLEDChangeBaud();
}

void LongPress(void)
{
  first = true;
  state = START_SCREEN;
  ResetLed();
  SerialChangeBaud();
}


void OLEDInit(void) // OLED init function
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);
}

void OLEDStart(void)
{
  ResetLed();
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(F("Serial\nTerminal\n\nBaudrate: "));
  display.println(baudrate[baudrate_index]);
  display.display();
}

void SerialChangeBaud(void)
{
  Serial.flush();
  delay(200);
  Serial.end();
  delay(200);
  Serial.begin(baudrate[baudrate_index]);
}

void OLEDChangeBaud(void)
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Change Baudrate: "));
  for(uint8_t i = 0; i < BAUDRATE_COUNT; i++)
  {
    if(i == baudrate_index)
    {
       display.print(F(" >"));
    } else {
      display.print(F("  "));
    }
    display.println(baudrate[i]);
  }
  display.display();
}

void OLEDBlinkBaud(void)
{
  unsigned long currentMillis_1 = millis();
  if(currentMillis_1 - previousMillis_1 >= interval_1) {
    previousMillis_1 = currentMillis_1;
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("Change Baudrate: "));
    for(uint8_t i = 0; i < BAUDRATE_COUNT; i++)
    {
      if(i == baudrate_index)
      {
         if(!blink){
          blink = true;
          pixels.setPixelColor(BOTTOM_LED,colorPicker(3, BOTTOM_BRIGHTNESS));
          pixels.setPixelColor(TOP_LED,colorPicker(3, TOP_BRIGHTNESS));
          pixels.show();
          display.print(F(" >"));
         } else {
          blink = false;
          pixels.setPixelColor(BOTTOM_LED,0);
          pixels.setPixelColor(TOP_LED,0);
          pixels.show();
          display.print(F("  "));
         }
        
      } else {
        display.print(F("  "));
      }
      display.println(baudrate[i]);
    }
    display.display();
  }
}

void OLEDSerialWrite(void)
{
  if (Serial.available() > 0) {
    bool led_on = true;
    display.clearDisplay();
    display.setCursor(0,0);
    int rlen = Serial.readBytesUntil('\n', buf, BUFFER_SIZE);       // read the incoming bytes:
    for(int i = 0; i < rlen; i++)
    {
      if(led_on) 
      {
        led_on = false;
        pixels.setPixelColor(BOTTOM_LED,0);
        pixels.setPixelColor(TOP_LED,0);
        pixels.show();
      } else
      {
        led_on = true;
        pixels.setPixelColor(BOTTOM_LED,colorPicker(2, BOTTOM_BRIGHTNESS));
        pixels.setPixelColor(TOP_LED,colorPicker(baudrate_index, TOP_BRIGHTNESS));
        pixels.show();
      }
      display.write(buf[i]);        // prints the received data
      display.display();
      delay(10);
    }
    display.display();
    ResetLed();
  }
}

uint32_t colorPicker(uint8_t i, uint8_t brightness)
{
  uint32_t color = 0;
  switch(i)
  {
    case 0:
    color = pixels.Color(0,0,brightness); // blue
    break;
    case 1:
    color = pixels.Color(0,brightness/4,brightness); // light blue
    break;
    case 2:
    color = pixels.Color(0,brightness,0); // light green
    break;
    case 3:
    color = pixels.Color(brightness/4,brightness,0); // green
    break;
    case 4:
    color = pixels.Color(brightness,brightness,0); // yellow
    break;
    case 5:
    color = pixels.Color(brightness,brightness/2,0); // brown
    break;
    case 6:
    color = pixels.Color(brightness,0,0); // red
    break;
  }
  return color;
}


void setupOTA(const char* nameprefix, const char* ssid, const char* password) {
  // Configure the hostname
  uint16_t maxlen = strlen(nameprefix) + 7;
  char *fullhostname = new char[maxlen];
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(fullhostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
  ArduinoOTA.setHostname(fullhostname);
  delete[] fullhostname;

  // Configure and start the WiFi station
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  pixels.setPixelColor(BOTTOM_LED,colorPicker(4, BOTTOM_BRIGHTNESS));
  pixels.setPixelColor(TOP_LED,colorPicker(4, TOP_BRIGHTNESS));
  pixels.show();
  // Wait for connection
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    pixels.setPixelColor(BOTTOM_LED,colorPicker(6, BOTTOM_BRIGHTNESS));
    pixels.setPixelColor(TOP_LED,colorPicker(6, TOP_BRIGHTNESS));
    pixels.show();
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  pixels.setPixelColor(BOTTOM_LED,colorPicker(3, BOTTOM_BRIGHTNESS));
  pixels.show();

  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
	//NOTE: make .detach() here for all functions called by Ticker.h library - not to interrupt transfer process in any way.
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("\nAuth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("\nBegin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("\nConnect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("\nReceive Failed");
    else if (error == OTA_END_ERROR) Serial.println("\nEnd Failed");
  });

  ArduinoOTA.begin();

  Serial.println("OTA Initialized");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  pixels.setPixelColor(TOP_LED,colorPicker(3, TOP_BRIGHTNESS));
  pixels.show();
}

