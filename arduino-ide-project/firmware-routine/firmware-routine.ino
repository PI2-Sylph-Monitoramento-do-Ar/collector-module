#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "SPIFFS.h"


const char* mqtt_server = "broker.emqx.io";
const char* topic = "esp32/pi2/sensor";
const char* DATA_FILE_NAME = "/data.txt";
const char* SSID = "";
const char* PASSWORD = "";


/* Tempo que a ESP32 vai ficar dormindo */
#define TIME_TO_SLEEP  1

/* De quanto em quanto tempo será feito o upload de dados  => 5 SEGUNDOS */
#define UPLOAD_DATA_INTERVAL_S 5

/* Quantidade de vezes que a ESP32 acordou */
RTC_DATA_ATTR unsigned long amount_of_awakeups = 0;

// Último horário que a ESP32 acordou
time_t nowSecs;

// Client de WIFI e de MQTT
WiFiClient espClient;
PubSubClient client(espClient);


void setup(){
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  Serial.println(); Serial.println(); Serial.println();

  setDeepSleepConfig();
  setWiFiConnection();
  FixClockTime();
  MountSPIFFS();
  setMQTTConfig();
}

void loop(){
  while ( !client.connected() ) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Producer")) {
      Serial.println("connected");
    }

    else {
      Serial.print("failed with state: ");
      Serial.println(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
  client.loop(); // Do not delete this line

  amount_of_awakeups += 1;
  Serial.println("Quantidade de vezes que acordei: " + String(amount_of_awakeups));

  collect_and_save_temperature();
  collect_and_save_pressure();
  collect_and_save_carbon_monoxide();


  if(time_to_upload()) {
    Serial.println("Time to upload!");
    upload_to_server();
  }


  // client.loop();
  Serial.flush();
  esp_deep_sleep_start();
}



int collect_and_save_temperature() {
  float temperature = random(15, 30);
  return save_variable_to_file(temperature, "temperature");
}

int collect_and_save_pressure() {
  float pressure = random(950, 1150);
  return save_variable_to_file(pressure, "pressure");
}

int collect_and_save_carbon_monoxide() {
  float carbon_monoxide = random(0, 1000);
  return save_variable_to_file(carbon_monoxide, "carbon-monoxide");
}




















































void deleteFile(String path) {
  Serial.print("Deletando arquivo " + String(path) + "... ");

  if (SPIFFS.remove(path)) {
    Serial.println("Arquivo excluído");
  }

  else {
    Serial.println("Falha na exclusão do arquivo");
    esp_deep_sleep_start();
  }
}




void MountSPIFFS() {
  if(!SPIFFS.begin(true)){
     Serial.println("An Error has occurred while mounting SPIFFS! Going to Deep Sleep");
     esp_deep_sleep_start();
  }
  else {
    Serial.println("Mounting SPIFFS - OK");
  }
}


void setWiFiConnection() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  // wait for WiFi connection
  unsigned int tries = 0;
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFi.status() != WL_CONNECTED)) {
    Serial.print(".");
    delay(1000);
    tries += 1;

    // Se falhar mais de 5 vezes, a ESP32 volta a dormir
    if (tries > 5) {
      Serial.println("\nFailed to connect to WiFi! Going to Deep Sleep");
      esp_deep_sleep_start();
    }
  }
  Serial.println(" connected");
}

void setMQTTConfig() {
  client.setServer(mqtt_server, 1883);
}

void setDeepSleepConfig() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
}

void FixClockTime() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  nowSecs = time(nullptr);

  // while (nowSecs < 8 * 3600 * 2) {
  //   delay(500);
  //   Serial.print(F("."));
  //   yield();
  //   nowSecs = time(nullptr);
  // }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);

  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}


// Função para verificar se tá na hora de ler o arquivo de dados e fazer o upload para o servidor
bool time_to_upload() {
  /* 15 * 60 / 5 -> A cada 180 vezes que a esp32 acordar, será executado a rotina de upload  */
  return amount_of_awakeups % ((UPLOAD_DATA_INTERVAL_S) / TIME_TO_SLEEP)== 0;
}




int save_variable_to_file(float variable, const char* variable_label) {
  File file = SPIFFS.open(DATA_FILE_NAME, FILE_APPEND);

  if(!file){
    char msg[100];
    sprintf(msg, "[%s] - Failed to open file", variable_label);
    Serial.println(msg);
    return -1;
  }

  char datetime[128];
  nowSecs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &timeinfo);

  char buffer[256];
  sprintf(buffer, "{\"%s\": \"%.2f\", \"datetime\": \"%s\"}\n", variable_label, variable, datetime);

  char msg[100];
  if(file.print(buffer)) {
    sprintf(msg, "[%s] - Data saved to file", variable_label);
  }
  else {
    sprintf(msg, "[%s] - Failed to save data to file", variable_label);
  }

  Serial.println(msg);
  file.close();
  return 0;
}



void upload_to_server() {
  File file = SPIFFS.open(DATA_FILE_NAME);

  if (!file) {
    Serial.println("An Error has occurred while opening file for reading! Going to Deep Sleep");
    esp_deep_sleep_start();
  }

  char buffer2[512];
  unsigned int i = 0;
  // buffer2[i++] = '[';

  while ( file.available() ) {
    char ch = file.read();
    if(ch != '\n') {
      buffer2[i++] = ch;
    }
    else {
      buffer2[i++] = '\0';
      client.publish(topic, buffer2);
      delay(500);
      i = 0;

      Serial.println("Data sended to server");
      Serial.println(buffer2);
      Serial.println();
    }
  }


  // i--;
  // buffer2[i++] = ']';
  // buffer2[i++] = '\0';

  // client.loop();
  // setMQTTConnection();
  // client.publish(topic, buffer2);
  // client.loop();

  // delay(3000);

  Serial.println("Data sended to server");
  Serial.println(buffer2);
  Serial.println();
  file.close();

  deleteFile(DATA_FILE_NAME);
}
