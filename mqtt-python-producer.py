import paho.mqtt.client as paho

broker = 'broker.emqx.io'
port = 1883
topic = 'esp32/pi2/sensor/bmp280'


def on_publish(client,userdata,result):             #create function for callback
    return

client1= paho.Client("control1")                           #create client object
client1.on_publish = on_publish                          #assign function to callback
client1.connect(broker,port)
                                 #establish connection
ret= client1.publish(topic,"data")                   #publish