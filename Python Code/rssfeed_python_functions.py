'''
This is a helper class for the OLED project, it helps send the required amount of string data, and image data
with proper resizing and conversion to bmp format, fit for publishing.
'''

#import the Paho module for creating the mqtt client
import paho.mqtt.client as mqtt

#import the PIL module so for image formatting and conversion.
from PIL import Image

'''
This is the main helper class, that is created by passing in the server, username and password as string
it exposes three function

publishStringData('topic','message to be published)
publishImageData('topic','name of the temperory image to be used','the image file to be sent, it can be a url or file name')
reconnect()
isConnected()
'''

class RSSOLEDPublisher:


    _isConnected = 0

    #The Class initializer, this is necessary for creating a class.
    def __init__(self,server,username,password):

        #callback when the client in connected
        def on_connect(client, userdata, flags, rc):
            print("Connected with result code " + str(rc))
            self._isConnected = 1

        # The callback for when a PUBLISH message is received from the server.
        def on_publish(client, userdata, mid):
            pass

        #Callback when the client is disconnected
        def on_disconnect():
            self._isConnected = 0
            print "Disconnected"

        #Initiate the mqtt client
        self.client = mqtt.Client()

        #set Callbacks
        self.client.on_connect = on_connect
        self.client.on_publish = on_publish
        self.client.on_disconnect = on_disconnect

        #set the username for mqtt
        self.client.username_pw_set(username, password)

        #Set the server address and port.
        self.client.connect_async(server, 1883, 60)

        print "connecting"

        # Blocking call that processes network traffic, dispatches callbacks and
        # handles reconnecting.
        # Other loop*() functions are available that give a threaded interface and a
        # manual interface.
        self.client.loop_start()

    def publishStringData(self,topic,msg):

        self.client.publish(topic,msg)

    def publishImageData(self,topic,img_name,image_file):

        #open the image file
        image = Image.open(image_file)

        #Resize the opened image
        image = image.resize((96, 64))

        #Save the image as BMP
        image.save(img_name+'.bmp', 'bmp')

        #Open the image
        file1 = open(img_name+'.bmp', 'rb')

        #Read the file
        file1 = file1.read()

        #Publish the data as bytearray, as this is the only format that will help us send a file.
        #The maximum size of the data sent using MQTT is limited to 256MB
        self.client.publish(topic, bytearray(file1))

    def reconnect(self):
        self.client.reconnect()


    @property
    def isConnected(self):
        return self._isConnected