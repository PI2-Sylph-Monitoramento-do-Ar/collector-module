#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#include <Adafruit_BMP280.h>


// Conexão
const char* ssid = "La Lakers ";
const char* password = "pandaemel";
const char* mqtt_server = "mqtt://mqtt.eclipse.org";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;



// Sensores
Adafruit_BMP280 bmp; // I2C

const float max_volts = 5.0;
const float max_analog_steps = 4095;

const int Sensor_MQ2 = 4;
const int Sensor_MQ131 = 2;

const int NO2_SENSOR = 36;
const int NH3_SENSOR = 39;
const int CO_SENSOR = 34;



void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  /*baud rate for serial communication*/
  Serial.begin(9600);

  pinMode(NO2_SENSOR, INPUT);
  pinMode(NH3_SENSOR, INPUT);
  pinMode(CO_SENSOR, INPUT);

  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1); delay(100);
  }




  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}







void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();



  Serial.print(F("Temperatura = "));
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  //Imprimindo os valores de Pressão
  Serial.print(F("Pressao = "));
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");


  //Imprimindo os valores de Altitude Aproximada
  Serial.print(F("Altitude Aprox = "));
  Serial.print(bmp.readAltitude(1013.25)); /* Ajustar a pressão de nível do mar de acordo com o local!*/
  Serial.println(" m");
  Serial.print("\n");

  int sensor_analog_mq2 = analogRead(Sensor_MQ2);
  Serial.print("Gas Sensor MQ2: ");
  Serial.print(sensor_analog_mq2);
  Serial.print("\n");

  int sensor_analog_mq131 = analogRead(Sensor_MQ131);
  Serial.print("Gas Sensor MQ131: ");
  Serial.print(sensor_analog_mq131);
  Serial.print("\n");

  int no2_value = analogRead(NO2_SENSOR);
  int nh3_value = analogRead(NH3_SENSOR);
  int co_value = analogRead(CO_SENSOR);


  Serial.print("CO: ");
  Serial.print(co_value * (max_volts / max_analog_steps));
  Serial.print("\n");

  Serial.print("NH3: ");
  Serial.print(nh3_value * (max_volts / max_analog_steps));
  Serial.print("\n");

  Serial.print("NO2: ");
  Serial.print(no2_value * (max_volts / max_analog_steps));
  Serial.print("\n");


  Serial.print("\n");
  Serial.println();
  delay(1000);                 /*DELAY of 1 sec*/
}
