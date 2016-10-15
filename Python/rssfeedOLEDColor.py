#!/usr/bin/python
import paho.mqtt.client as mqtt
def on_connect(client, userdata, flag, rc):
    print "Connected"

client= mqtt.Client()
client.connect_async("0.0.0.0",1883,60)
client.loop_start()

import time
import feedparser

def get_feed():
    url = "http://feeds.bbci.co.uk/news/rss.xml?edition=int"
    feed =feedparser.parse(url)
    for f in feed["entries"]:
       try:
	 print f["title"], "\n"
         import numpy
         import urllib, cStringIO
         from PIL import Image
         url = cStringIO.StringIO(urllib.urlopen(f['media_thumbnail'][0]['url']).read())
         image = Image.open(url)
         image.thumbnail((96,64))
         image.save('fd.bmp','bmp')
         file1 = open('fd.bmp','rb')
         file1 = file1.read()
         client.publish("news/bbc/international/oled/color",f["title"])
         time.sleep(5)
         client.publish('news/bbc/international/oled/color',bytearray(file1))
         time.sleep(5)
       except Exception:
	 pass

while True:
    get_feed()
