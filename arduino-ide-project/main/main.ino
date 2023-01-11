#include <WiFi.h>
#include <NTPClient.h>
#include <PubSubClient.h>

#include <Wire.h>               // This library allows you to communicate with I2C/TWI devices.
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;

// Connection settings
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.emqx.io";
const char* topic = "esp32/pi2/sensor/bmp280";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Connection objects
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;



#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  2        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR unsigned long last_epoch_time = 0;

void setup() {
  //Iniciando a comunicação serial
  Serial.begin(9600);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  if (!bmp.begin(0x76)) { /*Definindo o endereço I2C como 0x76. Mudar, se necessário, para (0x77)*/
    //Imprime mensagem de erro no caso de endereço invalido ou não localizado. Modifique o valor
    Serial.println(F(" Não foi possível encontrar um sensor BMP280 válido, verifique a fiação ou tente outro endereço!"));
    while (1) delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi ..");

  unsigned int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
    count += 1;
    if(count > 5) {
      return;
    }
  }

  timeClient.begin();
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883);
}

void loop() {

//  if (!client.connected()) {
//    mqtt_reconnect();
//  }

  client.loop();

  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude(1019);

  char buffer[150];
  sprintf(buffer, "{\"temperature\": \"%.2f\", \"pressure\": \"%.2f\", \"altitude\": \"%.2f\"}", temperature, pressure, altitude);
  Serial.println(buffer);
  client.publish(topic, buffer);





  timeClient.update();
  unsigned long current_epoch_time;
  if(last_epoch_time == 0) {
    last_epoch_time = timeClient.getEpochTime();
  }
  else {
    current_epoch_time = timeClient.getEpochTime();
    Serial.println("Diff: " + String(current_epoch_time - last_epoch_time));
  }
  last_epoch_time = current_epoch_time;


  Serial.println("Going to sleep now\n");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}


//void mqtt_reconnect() {
//  while (!client.connected()) {
//    Serial.print("Attempting MQTT connection...");
//    // Attempt to connect
//    if (client.connect("esp32-devkit-pi2-testing")) {
//      Serial.println("connected");
//    }
//
//    else {
//      Serial.print("failed, rc=");
//      Serial.print(client.state());
//      Serial.println(" try again in 5 seconds");
//      delay(5000);
//    }
//  }
//}
