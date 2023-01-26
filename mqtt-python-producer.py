
broker = 'broker.emqx.io'
port = 1883

#username = "sylph"
#password = "Sylph2023"
topic = 'measurements'

import time
import paho.mqtt.client as paho
import ssl


# python 3.6

import random
import time
from paho.mqtt import client as mqtt_client

# generate client ID with pub prefix randomly
client_id = f'python-mqtt-{random.randint(0, 1000)}'
# username = 'emqx'
# password = 'public'

CONNECTED = False



import json



def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            global CONNECTED
            CONNECTED = True
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client: mqtt_client.Client = mqtt_client.Client(client_id)
    #client.username_pw_set(username, password)
    client.on_connect = on_connect

    # client.tls_set("C:/Windows/system32/config/systemprofile/Desktop/attachments/server iot.crt", tls_version=ssl.PROTOCOL_TLSv1_2)
    # client.tls_insecure_set(True)



    client.connect(broker, port)
    return client


def publish(client):
    msg_count = 0
    while True:
        time.sleep(1)
        if CONNECTED is False:
            print("Not connected...")
            continue
        msg = {"messages": msg_count}
        msg = json.dumps(msg)
        result = client.publish(topic, msg)
        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{msg}` to topic `{topic}`")
        else:
            print(f"Failed to send message to topic {topic}")
        msg_count += 1


def run():
    client = connect_mqtt()
    client.loop_start()
    publish(client)


if __name__ == '__main__':
    run()
