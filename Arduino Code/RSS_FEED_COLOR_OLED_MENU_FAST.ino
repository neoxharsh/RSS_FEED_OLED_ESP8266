/*
 * This project is a basic project that shows various RSS feed.
 * Most of the code is documented, so you can understand how the code works
 * 
 * ESP-8266 based NodeMCU is used for the following project.
 * Nodemcu have an inbuilt 4MB flash storage, that is used to store the images that need to be displayed
 * on the screen. MQTT is used a transport protocol, as it will help make communication with the server easily.
 * 
 * The MQTT based mosquito server is hosted on DigitalOcean https://m.do.co/c/35f511b6a29b 
 * Oled driver is provided by https://github.com/sumotoy/SSD_13XX.git
 * MQTT library is provided by https://github.com/marvinroger/async-mqtt-client
 * Button library is provided by https://github.com/Mokolea/InputDebounce
 * 
 * The whole project document can be obtained from ________
 * 
 * The feeds are parsed on the DigitalOcean server using python, and the documentation for the same is also provided on the project home page.
 * 
 * 
 * 
 * yield() is called at regular interval, so that the watch dog timer do not reset and restart the system.
*/


/*
 * User based modifiaction needs to be done in the following line of code
 * 
 * 70 - write the total number of feeds, if you have two feeds write 3, as the plus one is due to the last entry in the feed.
 * 76 - make entries in form of three strings, the first one is the name of the feed, the second one is the topic name and last 
 *      one is the image to be shown in the menu. Make sure the last entry in the list is OTA and remember to update the number 
 *      of feeds accordingly.
 * 88,89 - The wifi credential 
 * 94, 95 - the mqtt credential.
 */



//Include file for the Button calls
#include <InputDebounce.h>

//Include for OTA methods
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

//Total number of feeds, it inludes the last entry of OTA UPDATE. Whever you add a new feed always check the number of entires in the feed list and update
//the entry accordingly
const int feeds = 5;

/*
   An array that holds the feed mqtt data, the first entry is the name for the feed, the second one
   is the topic to be subscribed, and the last one is the correspondig image to be displayed in the menu
*/
char *  feed[feeds][3] = {
  {"International", "oled/color/news/bbc/international", "/bbc_worldwide.bmp"},
  {"Technology", "oled/color/news/bbc/technology", "/bbc_technology.bmp"},
  {"UK Politics", "oled/color/news/bbc/politics", "/bbc_politics.bmp"},
  {"Science and Environment", "oled/color/news/bbc/science_and_environment", "/bbc_science.bmp"},
  {"OTA UPDATE", " ", "/ota.bmp"}
};

/*
   Pretty much self explationary.
   Set the Wifi SSID and Password
*/
const char* ssid = "......";
const char* password = ".......";

/*
   Set the MQTT Username and Password
*/
const char* mqtt_username = ".....";
const char* mqtt_password = "..........";


//Check for OTA, checks if the OTA mode is set, If set, it will change the loop function to check OTA hadle and stop the Button callback handle.
int ota = 0;

//Check is the user is in the menu.
int inMenu = 0;

//Index of the current feed, used only to subscribe and unsubscribe.
int currentFeed = 0;

//Index for feed number while in menu.
int newFeed = 0;

//Check is subsribed, it is used to unsubscribe only when topic is already subscribed.
int isSubscribed = 0;


//Initialize the display object on SPI interface by passing in the pins to the constructor.
SSD_13XX display = SSD_13XX(__CS1, __DC);

//Internal variable to reduce the need to remember feeds.
const int feedNumber = feeds - 1;

//Pin numbers for Buttons.
static const int pinSwitchSelect = D3;
static const int pinSwitchA = D4;

//Create Two set of buttons.
static InputDebounce buttonSelect;
static InputDebounce buttonA;


//Create AsyncMqttClient.
AsyncMqttClient mqttClient;

//File storage for receiving images.
File file;

//Check to make sure that the file was not closed and the whole payload is not copied to the file.
int fileCheck = 0;

// The main Setup function.
void setup() {

  //Start the serial interface.
  Serial.begin(115200);
  Serial.println();

  //This routine is necessary to initialize the file system.
  SPIFFS.begin();
  yield();

  //Callback when OTA starts. It prints Start on the screen when the upload starts.
  ArduinoOTA.onStart([]() {
    display.fillScreen(0x0000);
    display.setCursor(0, 0);
    display.print("Start");
    //  Serial.println("Start");
  });

  //Callback when OTA Completes. It prints End on the screen when the upload is completed.
  ArduinoOTA.onEnd([]() {
    display.fillScreen(0x0000);
    display.setCursor(0, 0);
    display.print("End");

    //Restart the ESP module once OTA update is complete.
    ESP.restart();
    //Serial.println("\nEnd");
  });


  //Register callback for the button, Only one call backs is used for each button, the rest two parametes need to be passed NULL.
  buttonSelect.registerCallbacks(buttonSelectPressedCallback, NULL, NULL);

  // setup input button (debounced)
  buttonSelect.setup(pinSwitchSelect, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);

  buttonA.registerCallbacks(buttonAPressedCallback, NULL, NULL);

  // setup input button (debounced)
  buttonA.setup(pinSwitchA, BUTTON_DEBOUNCE_DELAY, InputDebounce::PIM_INT_PULL_UP_RES);



  //Set the WIFI persistent state to False.
  WiFi.persistent(false);

  //Set WIFI mode to Station and not Acess Point, as we are going to connect to a access point.
  WiFi.mode(WIFI_STA);

  //Debug routine
  Serial.print("Connecting to Wi-Fi");

  //Connect to the wifi by passing in the ssid and password.
  WiFi.begin(ssid, password);

  //Initate the OLED display.
  display.begin();

  //Set the text size to scale to 1, so that the fonts will be a bit bigger on the screen.
  display.setTextScale(1);

  //Fill the screen to black color. a HEX value needs to be passed.
  display.fillScreen(0x0000);

  // Draw the splash screen.
  bmpDraw("/splash_screen.bmp", 0, 0);


  //Register callback for the mqtt client.
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  //Set the mqtt server, by passing in the IP address as an IPAddress object and the port number, the default is 1883.
  mqttClient.setServer(IPAddress(0,0,0,0), 1883);

  //Set the keepAlive time to 30, as the payload will be big and it will take time for the onMessage call to complete, if set for a lower value, the connection will be droped. 
  mqttClient.setKeepAlive(30).setCredentials(mqtt_username, mqtt_password).setClientId("myDevice");
  
  Serial.println("Connecting to MQTT...");

  //Check if Wifi is connected.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //Once wifi is connected, call the connect fucntion to connect to the server. 
  mqttClient.connect();
  
}

//The main Loop.
void loop() {
 
 //Switch loop to check if OTA we are in OTA mode or normal operation.
  
  switch (ota)
  {
    case 0:
      {
        /*
         * Copied from the example. It is th loop that is required to pool the button state. 
         */
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
        // The OTA handle that call the OTA callback.
        yield();
        ArduinoOTA.handle();
        break;
      }
  }

}


/*
 * The following code is a modifed code from the https://github.com/sumotoy/TFT_ILI9163C/blob/master/examples/SD_example/SD_example.ino
 * The code takes the file name as a char string. and x and y as the location for the image to be drawen.
 * This code is specially for the ESP SPIFFS module as, this module have an inbuild Flash memory that is used to store data. 
 * The Adafruit library is really slow as compared to this call, and moreover the above libaray have many more user friendly fuction.
 * Kudos to the author for making such awesome library.
 */
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





//Callback for the Select button press event.

void buttonSelectPressedCallback()
{
  //delay to make sure some time is given once the button is pressed.
  delay(10);
  yield();

  //Checks if the we are already in the menu. This button have have no fuction if we are not in the menu.
  if (inMenu == 1)
  {
    //Checks if the current selected menu item is OTA, no need to mess with this part of the code. 
    if (feed[newFeed][0] == "OTA UPDATE")
    {
      //Begins the OTA Sequence, and after this the buttons become non operational. If ota is canceled, restart the ESP.
      ArduinoOTA.begin();
      ota = 1;
      display.fillScreen(0x0000);
      display.setCursor(0, 0);
      display.print("OTA Mode Started");
    }
    else
    {
      //If we are not in OTA mode, than set the index of currentFeed the same as the index of current menu item.
      currentFeed = newFeed;
      
      //Subscribe to the topic that belongs to the current feed.
      mqttClient.subscribe(feed[currentFeed][1], 2);

      //Draw a check to show that the feed have been selected.
      bmpDraw("/check.bmp", 0, 0);

      //triger the variable to set the current state as subscribed.
      isSubscribed = 1;

      //Reset the newFeed index, and set the inMenu to not in Menu.
      newFeed = 0;
      inMenu = 0 ;
    }
  }
}

void buttonAPressedCallback()
{
  delay(10);
  yield();

//Chekk if subscribed, if subscribed than unsubscribe, if this is not done, than as a new feed news is publised, the screen will be updated and we no longer will be in the menu.
  if (isSubscribed == 1)
  {
    
//Unsubscribe the current feed.
    mqttClient.unsubscribe(feed[currentFeed][1]);

    //Set the isSubscrbied check to not subscribed.
    isSubscribed = 0;
  }

//Checks if we are in the menu or not
  switch (inMenu)
  {
    case 0:
      {
        //if we are not in the menu, than draw the menu screen and after a while, draw the first item in the feed list.
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
        //If we are already in the menu, draw the next item in the feed, this button keeps on cycling the list.
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

  //Fill the screen to black color. a HEX value needs to be passed.
  display.fillScreen(0x0000);

  // Draw the splash screen.
  bmpDraw("/splash_screen.bmp", 0, 0);
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
    display.fillScreen(0x0000);
    display.setCursor(0, 50);
  display.print("Server Reconnecting");
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

   The whole process for image data is to first open a file, and check if the file is
   already open or not, if it is already open, than it means the complete image data is not obtained 
   and still the remaing data needs to be copied to the file.
   Once the index plus the lenght of the current data is equal to the total file size, it means the
   data have been collected and no more data will come, so the file is closed. This is necessary to 
   save the image file, it is saved as bmp file in the ESP SPIFFS. Then the image file is again opened
   as a readonly file by the bmpDraw function. The file path always needs to be followed by "/" as 
   this shows the root folder.
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

//This is to check, if the data obtained is obtained or not.
  if (total > 0)
  {
    //If the total length of the data is equal to the current length of the data, it means that only text have been obtained.
    if (len == total)
    {
      display.fillScreen(0x0000);
      display.setCursor(0, 0);
      display.print(payload);
      yield();
    }

    //else image have been obtained.
    else {
      //Checks if the file is already open or not, it also shows that the previous data is still pending.
      if (fileCheck == 0)
      {
        //Open the file to write too.
        file = SPIFFS.open("/temp_image.bmp", "w");
        //Set the file open check to open.
        fileCheck = 1;
      }

//Iterate till all the data is not obtained.
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
          //Write the payload to a file.
          file.write(payload[i]);
          yield();
        }

      }
    }
  }
}

//Callback for the mqtt publish call.
void onMqttPublish(uint16_t packetId) {

  /*

    Serial.println("** Publish acknowledged **");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  */
}


