#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <PubSubClient.h>

const char* mqtt_server = "broker.emqx.io";
const char* topic = "esp32/pi2/sensor/bmp280";

const char* SSID = "DiFi";
const char* PASSWORD = "durvallindo";

#include "SPIFFS.h"


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1           /* Time ESP32 will go to sleep (in seconds) */

#define UPLOAD_DATA_INTERVAL_S 5  /* De quanto em quanto tempo será feito o upload de dados  => 5 SEGUNDOS*/
RTC_DATA_ATTR unsigned long  amount_of_awakeups = 0; /* Quantidade de vezes que a ESP32 acordou */
RTC_DATA_ATTR unsigned long counter = 0;

time_t nowSecs;                        // Horário  atual que a ESP32 acordou


WiFiClient espClient;
PubSubClient client(espClient);


bool time_to_upload() {
  /* 15 * 60 / 5 -> A cada 180 vezes que a esp32 acordar, será executado a rotina de upload  */
  return amount_of_awakeups % ((UPLOAD_DATA_INTERVAL_S) / TIME_TO_SLEEP)== 0;
}


void setup(){
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  Serial.println(); Serial.println(); Serial.println();

  /*
    First we configure the wake up source
    We set our ESP32 to wake up every X seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  // wait for WiFi connection
  unsigned int tries = 0;
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFi.status() != WL_CONNECTED)) {
    Serial.print(".");
    delay(1000);
    tries += 1;
    if (tries > 5) {
      Serial.println("\nFailed to connect to WiFi! Deep Sleep");
      esp_deep_sleep_start();
    }
  }
  Serial.println(" connected");
  setClock();

  if(!SPIFFS.begin(true)){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }
  else {
    Serial.println("Mounting SPIFFS - OK");
  }


  client.setServer(mqtt_server, 1883);
}

void loop(){
  Serial.println("Acordei!");

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32Producer")) {

      Serial.println("connected");

    } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);

    }
  }

  client.loop();

  amount_of_awakeups += 1;
  Serial.print("Quantidade de vezes que acordei: ");
  Serial.println(String(amount_of_awakeups));




  // BLOCK TO GET SENSOR DATA
  float temperature = random(15, 30);
  Serial.print("Temperatura: ");
  Serial.println(String(temperature));





  // BLOCK TO SAVE SENSOR DATA IN FILE
  File file = SPIFFS.open("/test_plugin.txt", FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  counter += 1;
  char buffer[150];
  sprintf(buffer, "{\"temperature\": \"%.2f\"}\n", temperature);

  if(file.print(buffer)) {
    Serial.println("Data saved to file");
  }
  file.close();
































  if(time_to_upload()) {
    Serial.println("Time to upload!");

    file = SPIFFS.open("/test_plugin.txt");

    if (!file) {
      Serial.println(" - falha ao abrir arquivo para leitura");
      return;
    }

    char buffer2[1000];
    unsigned int i = 0;
    buffer2[i++] = '[';
    while ( file.available() ) {

      char ch = file.read();
      if(ch == '\n') {
        Serial.println();
        buffer2[i++] = ',';
      }
      else {
        Serial.print(ch);
        buffer2[i++] = ch;
      }
    }
    i--;
    buffer2[i++] = ']';
    buffer2[i++] = '\0';
    Serial.print("Buffer 2");
    Serial.println(buffer2);
    client.publish(topic, buffer2);
    Serial.println();
    file.close();
    deleteFile("/test_plugin.txt");
  }









  Serial.flush();
  esp_deep_sleep_start();
}












bool deleteFile(String path) {
  Serial.print("Deletando arquivo ");
  Serial.print(path);
  Serial.println(" : ");
  if (SPIFFS.remove(path)) // exclui o arquivo passando o
    //                          caminho/nome (path)
    //                        Se a exclusão der certo, ...
  {
    // informa ao usuário que deu certo
    Serial.println(" - arquivo excluído");
  } else {
    // informa ao usuário que deu erros e sai da função retornando false.
    Serial.println(" - falha na exclusão");
    return false;
  }
  return true; // retorna true se não houver nenhum erro
}





void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  nowSecs = time(nullptr);

//  while (nowSecs < 8 * 3600 * 2) {
//    delay(500);
//    Serial.print(F("."));
//    yield();
//    nowSecs = time(nullptr);
//  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);

  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}