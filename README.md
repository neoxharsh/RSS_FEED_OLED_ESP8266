# RSS FEED OLED ESP8266
ESP8266 Based RSS Feed on color OLED

An extremely detailed version of this project is available on hackster.io

This is an example of RSS Feed implementaion on ESP8266 using MQTT as trasnport protocol and SSD1331 based color OLED screen as display


![alt tag](https://github.com/neoxharsh/RSS_FEED_OLED_ESP8266/blob/master/images/Project_Image_2.jpg)

The main arduino code is in the respective folder. 
The Ardiuno libraries that are used are kept in the folders too, but they are also available in the library manager in the IDE


## Mosquitto server hosting on Digital Ocean
The mosquitto server is hosted on Ubuntu on DigitalOcean, A montly plan can be created by as low as 5$. You can create an account using 
https://m.do.co/c/35f511b6a29b, if you use this link, I will get some credit, which I can use for my next project, but you are free to create your own account directly. 

## Seting up digital ocean server for first time
For this create an account, than create a droplet, for the image select Ubuntu x64 and for size select 5$/month. leave the rest as default and hit create, within few minutes, a new server will spin up, now use an SSH client such as putty and in the host name enter the IP address that you see in the said column beside the droplet name, now click open, after few seconds, the login prompt will be shown.
Now enter root as username and for the password, you need to go to your email as the root passwerd is emailed to you, copy the password and right click on the putty terminal window and hit enter. Now again right click on the same window, hit enter, now it will ask you to enter your new password, enter new password and hit enter, again re enter your password and hit enter. Now we need to update the system, so enter the following command in terminal

```
apt-get update
apt-get upgrade
```
## Setting up Python
Python is not installed by default, so we need to manual install it, so enter the following command in the terminal to install python 2.7 and pip.
```
apt-get install python
apt-get install python-pip
```


## Setting up mosquitto client
The client can be installed by issuing the following command,

```
sudo apt-get install mosquitto mosquitto-clients
```

## Setting up username and password for added security
Once mosquitto is installed, next step is to create a password file for security. 
Type the following code in the ubuntu terminal 

```
mosquitto_passwd -c "path to password file" "username"
```

Now enter a password in the promp, this will create a hashed password file, that will be used by the broker. Now we need to configure the mosquitto broker. Type the following in terminal.

```
nano /etc/mosquitto/mosquitto.conf
```
This will open the mosquitto.conf file

Now delete all the content of the file using Ctrl-K, and paste the following content after modifying the necessary parameters. and than saving it using Ctrl-X followed by Y and then Enter.

```

# Place your local configuration in /etc/mosquitto/conf.d/
#
# A full description of the configuration file is at
# /usr/share/doc/mosquitto/examples/mosquitto.conf.example

pid_file /var/run/mosquitto.pid

persistence true
persistence_location /var/lib/mosquitto/

log_dest file /var/log/mosquitto/mosquitto.log

include_dir /etc/mosquitto/conf.d
allow_anonymous false
password_file /etc/mosquitto/pwfile

```

Now start the mosquitto broker in daemon mode and with the selected config file, so that the client do not get closed when the treminal is closed.
Type the following command in terminal

```
mosquitto -d -c /etc/mosquitto/mosquitto.conf
```

## Python client modules Installation
The python program reqires paho client and PIL, which can be installed by

```
pip install paho-mqtt
pip install pillow
```

## NodeMCU (ESP8266) Setup
Now it is time to set up the actual hardware. For this project we have used ESP8266 based Node MCU. This have an inbuilt USB based serial port for easy programming. 
Connect the Color OLED display to NodeMCU as follow
```
VCC   -   3v3
GND   -   GND
NC    -   Do not Connect
DIN   -   D7
CLK   -   D5
CS    -   D2
D/C   -   D1
RES   -   3v3

Button Select - D3
Button A     - D4
Common pin from both button to GND of NodeMCU
```
## Schematics
![alt tag](https://github.com/neoxharsh/RSS_FEED_OLED_ESP8266/blob/master/images/schematics.jpg)
 
Then open the Arduino IDE, and make sure that you have already installed the ESP8266 Arduino core according to https://github.com/esp8266/Arduino

Now open the sketch from the Arduino folder, and make sure you copy all the libraries to the Documents/Arduino/library folder or install following library using the library manager

1. AsyncMQTT
2. ESPAsyncTCP
3. InputDebounce
4. SSD_13XX

Now just change the wifi ssid and password, and for the mqtt client username and password use the one that you entered [above](#setting-up-username-and-password-for-added-security). Now go to the setup function and in the setServer function pass the server address that you obtained through Digital Ocean over [here](#seting-up-digital-ocean-server-for-first-time) as (0,0,0,0), replace the 0 with actual number. 

Now connect a USB cable to the NodeMCU and the computer, wait for a while till all the drivers are installed. Now press Ctrl-K, this will open the current sketch folder. create a data folder if it is not present and copy the files from the data folder in my repository to your data folder, it depends if you are opening the example from my repository than no need to follow any of this step if not than you have to follow this step. Now in the Arduino IDE select Tools-> ESP8266 Sketch Data Upload. This will upload all the content from the data folder to the inbuilt flash. After this is sucessfully done, upload the sketch. Now only the splash screen will show up, you need to wait till "Server Connected" is shown. This is end of the Hardware setup. 

## Starting the python client
Fire up the putty client and login in to your droplet and now we need to install an app called byobu, this can be installed as follow
```
apt-get install byobu
```
now type this command to create first python file that contains the wrapper function for our project.
```
mkdir rss_python
cd rss_python
nano rssfeed_python_functions.py
```
now copy the content from the rssfeed_python_functions.py file in the Python Code folder and right click on the terminal window, this will copy the code to the file, hit Ctrl-X followed by Y and then Enter, this will close the file. After entering this command

```
nano feed.py
```

type this

```
#!/usr/bin/python 
```
now as above copy the content of the feed.py file and copy it in this file. 

Now let us make this file executable, to do this enter the following command.

```
chmod +x feed.py
```

Now we need to run this program in the background so for this enter the following command

```
byobu
./feed.py &
```

Now you close the window, and the program will keep on running in the background.
Now on click the A button to enter the menu. Once you are in the menu, you can select the current feed by clicking the other select button, or you can move forward by pressing the A button, A button will keep on scrolling through the list. The last item in the list is the OTA mode, once you select it, the Module will enter OTA mode and will be ready to get update through wifi, provided the Arduino IDE and NodeMCU are connected to the same network. 


## Screenshot
![alt tag](https://github.com/neoxharsh/RSS_FEED_OLED_ESP8266/blob/master/images/Project_Image_1.jpg?raw=true)
![alt tag](https://github.com/neoxharsh/RSS_FEED_OLED_ESP8266/blob/master/images/Project_Image_3.jpg)
