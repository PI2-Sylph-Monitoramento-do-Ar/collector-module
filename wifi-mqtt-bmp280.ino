#include <WiFi.h>
#include <PubSubClient.h>

#include <Wire.h> // This library allows you to communicate with I2C/TWI devices.
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;

// Connection settings
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.emqx.io";
const char* topic = "esp32/pi2/sensor/bmp280";

// Connection objects
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


void setup() {
  //Iniciando a comunicação serial
  Serial.begin(9600);

  if (!bmp.begin(0x76)) { /*Definindo o endereço I2C como 0x76. Mudar, se necessário, para (0x77)*/
    //Imprime mensagem de erro no caso de endereço invalido ou não localizado. Modifique o valor
    Serial.println(F(" Não foi possível encontrar um sensor BMP280 válido, verifique a fiação ou tente outro endereço!"));
    while (1) delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
}

void loop() {

  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure();
    float altitude = bmp.readAltitude(1019);

    char buffer[150];
    sprintf(buffer, "{\"temperature\": \"%.2f\", \"pressure\": \"%.2f\", \"altitude\": \"%.2f\"}", temperature, pressure, altitude);
    Serial.println(buffer);
    client.publish(topic, buffer);
  }
}


void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32-devkit-pi2-testing")) {
      Serial.println("connected");
    }

    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
