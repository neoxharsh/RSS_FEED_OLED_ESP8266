#include <InputDebounce.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
//ESP WiFi include file.
#include <ESP8266WiFi.h>

//MQTT Client that implements ESPAsync to perform call back without blocking and transfer of large files, with Qos 1 and 2 support, and many more.
#include <AsyncMqttClient.h>

//SSD1331 Color OLED driver.
#include <SSD_13XX.h>
#include "_fonts/unborn_small.c"
//Arduino SPI Routine
#include <SPI.h>

//ESP SPIFFS module
#include "FS.h"


// Defination for SPI interface to OLED
#define __CS1   D2
#define __DC  D1
#define BUTTON_DEBOUNCE_DELAY   20   // [ms]

const int feeds = 3;
char *  feed[feeds][3] = {
  {"International", "news/bbc/international/oled/color", "/bbc_worldwide.bmp"},
  {"Technology", "news/bbc/technology/oled/color", "/bbc_technology.bmp"},
  {"OTA UPDATE", " ", "/ota.bmp"}
};

const char* ssid = "";
const char* password = "";

const char* mqtt_username = "";
const char* mqtt_password = "";


int ota = 0;
int inMenu = 0;

int currentFeed = 0;
int newFeed = 0;

int okHold = 0;

int isSubscribed = 0;


//Initialize the display object on SPI interface by passing in the pins to the constructor.
SSD_13XX display = SSD_13XX(__CS1, __DC);

const int feedNumber = feeds - 1;


static const int pinSwitchSelect = D3;
static const int pinSwitchA = D4;


static InputDebounce buttonSelect;
static InputDebounce buttonA;


//Create AsyncMqttClient.
AsyncMqttClient mqttClient;

//File storage for receiving images.
File file;

//Check to make sure that the file was not closed and the whole payload is not copied to the file.
int fileCheck = 0;



void buttonSelectPressedCallback()
{
  delay(10);
  yield();
if (inMenu==1)
{
  if (feed[newFeed][0] == "OTA UPDATE")
  {
   ArduinoOTA.begin();
    ota = 1;
    display.fillScreen(0x0000);
    display.setCursor(0, 0);
    display.print("OTA Mode Started");
  }
  else
  {
    currentFeed = newFeed;
    mqttClient.subscribe(feed[currentFeed][1], 2);
    bmpDraw("/check.bmp", 0, 0);
    isSubscribed = 1;
    newFeed = 0;
    inMenu = 0 ;
  }
}
}

void buttonAPressedCallback()
{
  delay(10);
  yield();

  if (isSubscribed == 1)
  {
     
    mqttClient.unsubscribe(feed[currentFeed][1]);
    isSubscribed = 0;
  }


  switch (inMenu)
  {
    case 0:
      {
        inMenu = 1;
        newFeed = 0;
        display.fillScreen(0x0000);
        bmpDraw("/menu.bmp", 0, 0);
        delay(2000);
        display.fillScreen(0x0000);
        bmpDraw(feed[newFeed][2], 0, 0);
        yield();
        break;
      }
    case 1:
      {
        if (newFeed != feedNumber)
        {
          newFeed++;
          display.fillScreen(0x0000);
          bmpDraw(feed[newFeed][2], 0, 0);
          yield();
        }
        else
        {
          newFeed = 0;
          display.fillScreen(0x0000);
          bmpDraw(feed[newFeed][2], 0, 0);
        }
        break;
      }
  }

}



//Callback for MQTT, fired when it connects to the server.
void onMqttConnect() {
  Serial.println("** Connected to the broker **");
  
  display.setCursor(0, 50);
  display.print("Server Connected");

}


//Callback when the client disconnects from the server. So we can reconnect, when the connectin drops.
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("** Disconnected from the broker **");
  Serial.println("Reconnecting to MQTT...");
  if (ota == 0)
  {
    //Call to reconnect, when the call back is fired.
    mqttClient.connect();
  }
}


//Callback when suceffuly subscribed to the topic.
void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("** Subscribe acknowledged **");
  Serial.println(feed[currentFeed][0]);
  Serial.println("Subscribed");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}


//Callback when the topic is unsubcribed.
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("** Unsubscribe acknowledged **");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.println(feed[currentFeed][0]);
  Serial.println(" Unsubsribed");
}


/*
   The main callback where all the fun takes place.
   Whenever any client publish to the topic this callback is fired.
   In our project, when the python client publishes the data.
   The inital data is just plain text. Then the next publish carries
   the image data. As the image data is realitvely big, the data is obtained in chunks.
   They need to be processed in order for the image to be shown.
*/
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  /*  Serial.println("** Publish received **");
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);
  */


if (total>0)
{
  if (len == total)
  {
    display.fillScreen(0x0000);
    display.setCursor(0, 0);
    display.print(payload);
    yield();
  }
  else {
    if (fileCheck == 0)
    {
      file = SPIFFS.open("/temp_image.bmp", "w");
      fileCheck = 1;
    }


    if (index + len == total)
    {
      file.close();
      display.clearScreen();
      bmpDraw("/temp_image.bmp", 0, 0);
      SPIFFS.remove("/temp_image.bmp");
      fileCheck = 0;
yield();
      //  Serial.println("Over");
    }
    else
    {
      // Serial.println(i++);

      for (int i = 0; i < len; i++)
      {
        file.write(payload[i]);
        yield();
      }

    }
  }
}
}

void onMqttPublish(uint16_t packetId) {

  /*

    Serial.println("** Publish acknowledged **");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  */
}

void setup() {

  Serial.begin(115200);

  Serial.println();
  SPIFFS.begin();
yield();

  ArduinoOTA.onStart([]() {
    display.fillScreen(0x0000);
    display.setCursor(0, 0);
    display.print("Start");
    //  Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    display.fillScreen(0x0000);
    display.setCursor(0, 0);
    display.print("End");
    
    ESP.restart();
    //   Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

   
  });
  ArduinoOTA.onError([](ota_error_t error) {
 
  });
  // register callbacks
  for (int i = 0; i <= feedNumber; i++)
  {
    Serial.println(feed[i][0]);
  }
  buttonSelect.registerCallbacks(buttonSelectPressedCallback, NULL, NULL);

  // setup input button (debounced)
  buttonSelect.setup(pinSwitchSelect, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);

  buttonA.registerCallbacks(buttonAPressedCallback, NULL, NULL);

  // setup input button (debounced)
  buttonA.setup(pinSwitchA, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);



  Serial.println();
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  display.begin();
   //display.setFont(&unborn_small);
 display.setTextScale(1);
  display.fillScreen(0x0000);
  bmpDraw("/splash_screen.bmp", 0, 0);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" OK");

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(IPAddress(0,0,0,0), 1883);
  mqttClient.setKeepAlive(30).setCredentials(mqtt_username, mqtt_password).setClientId("myDevice");
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void loop() {
  switch (ota)
  {
    case 0:
      {
        unsigned long now = millis();

        // poll button state
        buttonSelect.process(now); // callbacks called in context of this function

        yield();
        buttonA.process(now);
        yield();

        break;
      }
    case 1:
      {
yield();
        ArduinoOTA.handle();
        break;
      }
  }

}


#define BUFFPIXEL 20

void bmpDraw(const char *filename, uint8_t x, uint16_t y) {
  if (1 == 1) {

    File     bmpFile;
    uint16_t bmpWidth, bmpHeight;   // W+H in pixels
    uint8_t  bmpDepth;              // Bit depth (currently must be 24)
    uint32_t bmpImageoffset;        // Start of image data in file
    uint32_t rowSize;               // Not always = bmpWidth; may have padding
    uint8_t  sdbufferLen = BUFFPIXEL * 3;
    uint8_t  sdbuffer[sdbufferLen]; // pixel buffer (R+G+B per pixel)
    uint8_t  buffidx = sdbufferLen; // Current position in sdbuffer
    boolean  goodBmp = false;       // Set to true on valid header parse
    boolean  flip    = true;        // BMP is stored bottom-to-top
    uint16_t w, h, row, col;
    uint8_t  r, g, b;
    uint32_t pos = 0;

    if ((x >= display.width()) || (y >= display.height())) return;

    // Open requested file on SD card
    if ((bmpFile = SPIFFS.open(filename, "r")) == NULL) {
      display.setCursor(0, 0);
      display.print("file not found!");
      return;
    }

    // Parse BMP header
    if (read16(bmpFile) == 0x4D42) { // BMP signature
      read32(bmpFile);
      (void)read32(bmpFile); // Read & ignore creator bytes
      bmpImageoffset = read32(bmpFile); // Start of image data
      // Read DIB header
      read32(bmpFile);
      bmpWidth  = read32(bmpFile);
      bmpHeight = read32(bmpFile);
      if (read16(bmpFile) == 1) { // # planes -- must be '1'
        bmpDepth = read16(bmpFile); // bits per pixel
        if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
          goodBmp = true; // Supported BMP format -- proceed!
          rowSize = (bmpWidth * 3 + 3) & ~3;// BMP rows are padded (if needed) to 4-byte boundary
          if (bmpHeight < 0) {
            yield();
            bmpHeight = -bmpHeight;
            flip      = false;
          }
          // Crop area to be loaded
          w = bmpWidth;
          h = bmpHeight;
          if ((x + w - 1) >= display.width())  w = display.width()  - x;
          if ((y + h - 1) >= display.height()) h = display.height() - y;
          display.startPushData(x, y, x + w - 1, y + h - 1);
          for (row = 0; row < h; row++) { // For each scanline...
            if (flip) { // Bitmap is stored bottom-to-top order (normal BMP)
              pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
              yield();
            }
            else {     // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + row * rowSize;
            }
            if (bmpFile.position() != pos) { // Need seek?
              bmpFile.seek(pos, SeekSet);
              buffidx = sdbufferLen; // Force buffer reload
            }
            for (col = 0; col < w; col++) { // For each pixel...
              // Time to read more pixel data?
              if (buffidx >= sdbufferLen) { // Indeed
                bmpFile.read(sdbuffer, sdbufferLen);
                buffidx = 0; // Set index to beginning
              }
              // Convert pixel from BMP to TFT format, push to display
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              yield();
              display.pushData(display.Color565(r, g, b));
              yield();
            } // end pixel
          } // end scanline
          display.endPushData();
          yield();
        } // end goodBmp
      }
    }

    bmpFile.close();
    if (!goodBmp) {
      display.setCursor(0, 0);
      display.print("file unrecognized!");
      yield();
    }
  }
}


uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}










