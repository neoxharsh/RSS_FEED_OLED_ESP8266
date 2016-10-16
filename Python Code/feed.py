'''

This is the main script that uses feedParser to parse the rss feed.
Not all feeds can be parsed using feedPraser, so for that, own parser needs
to be created. The rssfeed python function wrapper is created to isolate the
format that need to be sent to the client.

This helps the creater of a new parser worry about the parsing and not on how to
send the appropraite data.

'''

import feedparser
import urllib, cStringIO
from threading import Thread

from rssfeed_python_functions import RSSOLEDPublisher
import time


#Create an insitance of RSSOLEDPublisher class, by passing in the server address, username and password
publisher = RSSOLEDPublisher("........",'.........','........')


#Creat a dictonary of feed, where the key is the feed name and the value is function to be called by the thread loop


feeds = {
         "Technology":"get_technology_feed",
         "World":"get_world_feed",
         "UK Politics":"get_uk_politics_feed"
         }


#First example of the BBC RSS Technology Feed.
def get_technology_feed():

    #This loop is present so that once all the entries in the feed are published, it again parses the feed, for new entries.
    while True:
        #The base URL for the feed.
        url = "http://feeds.bbci.co.uk/news/technology/rss.xml?edition=uk"

        #Parse the feed using feedparser.
        feed =feedparser.parse(url)

        #Loop to publish each entry of the aquired feed.
        for f in feed["entries"]:
           try:
             url = cStringIO.StringIO(urllib.urlopen(f['media_thumbnail'][0]['url']).read())
             publisher.publishStringData("oled/color/news/bbc/technology",f["title"])
             time.sleep(2)
             publisher.publishImageData('oled/color/news/bbc/technology','temp_image_technology',url)
             time.sleep(5)
           except Exception:
             pass



def get_world_feed():
        while True:
            url = "http://feeds.bbci.co.uk/news/world/rss.xml"
            feed =feedparser.parse(url)
            for f in feed["entries"]:
                try:
                    url = cStringIO.StringIO(urllib.urlopen(f['media_thumbnail'][0]['url']).read())
                    publisher.publishStringData("oled/color/news/bbc/international",f["title"])
                    time.sleep(2)
                    publisher.publishImageData('oled/color/news/bbc/international','temp_image_world',url)
                    time.sleep(5)
                except Exception:
                    pass


def get_uk_politics_feed():
    while True:
        url = "http://feeds.bbci.co.uk/news/politics/rss.xml"
        feed =feedparser.parse(url)
        for f in feed["entries"]:
           try:
             url = cStringIO.StringIO(urllib.urlopen(f['media_thumbnail'][0]['url']).read())
             publisher.publishStringData("oled/color/news/bbc/politics",f["title"])
             time.sleep(2)
             publisher.publishImageData('oled/color/news/bbc/politics','temp_image_politics',url)
             time.sleep(5)
           except Exception:
             pass


# Create a loop to start each thread from the list of feeds
for func_name in feeds.itervalues():

    print func_name
    #execute a raw python function, by substuting certain part of the code.
    exec ("%s_process = Thread(target=%s)"%(func_name,func_name))
    exec ("%s_process.start()"%(func_name))