#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <MQUnifiedsensor.h>
#include <Adafruit_BMP280.h>
#include <PubSubClient.h>
#include <ESP32Time.h>
#include "SPIFFS.h"


#define uS_TO_S_FACTOR                  1000000ULL  /* micro seconds to seconds factor */
#define HIBERNATION_TIME_S              2           /* Tempo que a ESP32 fica em deep sleep */
#define AMOUNT_OF_AWAKENINGS_TO_UPLOAD  5           /* Após X despertares é feito upload */

#define SSID                    ("")
#define PASSWORD                ("")
#define DATA_FILE_NAME          ("/data.txt")
#define MQTT_SERVER_URI         ("broker.emqx.io")
#define MQTT_SERVER_PORT        (1883)
#define MQTT_TOPIC_NAME         ("esp32/pi2/sensor")

#define Board                   ("ESP-32")
#define Voltage_Resolution      (3.3)
#define MICS_Voltage_Resolution (5.0)
#define ADC_Bit_Resolution      (12)
#define MAX_ANALOG_STEP         (4095)

#define MQ2_GPIO_PIN            (36)
#define RatioMQ2CleanAir        (9.83)

#define MQ131_GPIO_PIN          (34)
#define RatioMQ131CleanAir      (15)

#define CO_GPIO_PIN             (5)
#define NO2_GPIO_PIN            (16)
#define NH3_GPIO_PIN            (17)

Adafruit_BMP280 bmp;
MQUnifiedsensor MQ2   (Board, Voltage_Resolution, ADC_Bit_Resolution, MQ2_GPIO_PIN,   "MQ-2");
MQUnifiedsensor MQ131 (Board, Voltage_Resolution, ADC_Bit_Resolution, MQ131_GPIO_PIN, "MQ-131");
ESP32Time       rtc;
WiFiClient      espClient;
PubSubClient    client(espClient);
RTC_DATA_ATTR   unsigned long amount_of_awakeups = 0;




// Variáveis de controle para saber se os sensores foram ou não inicializados com sucesso
bool mq2_is_working;
bool mq131_is_working;
bool bmp280_is_working;
bool mics6418_is_working;


void setup()
{
  Serial.begin(115200); delay(10);
  while ( !Serial )     delay(100);

  esp_sleep_enable_timer_wakeup(HIBERNATION_TIME_S * uS_TO_S_FACTOR);
  wakeup_reason();

  setup_SPIFFS_file_system();

  // Variáveis de flag para verificar se as rotinas de setup dos sensores estão funcionando
  mq2_is_working = false;
  mq131_is_working = false;
  bmp280_is_working = false;
  mics6418_is_working = false;

  mq2_is_working = setup_MQ2_sensor();
  mq131_is_working = setup_MQ131_sensor();
  bmp280_is_working = setup_BPM280_sensor();
  mics6418_is_working = setup_MICS6418_sensor();
}

void loop()
{
  Serial.print("Time: "); Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));

  collect_and_save_mq2_sensor_data();
  collect_and_save_mq131_sensor_data();
  collect_and_save_bmp280_sensor_data();
  collect_and_save_mics6418_sensor_data();

  if(time_to_upload()) {
    Serial.println("Time to upload!");
    upload_to_server();
  }

  Serial.println("--------------------"); delay(500);
  Serial.flush();
  esp_deep_sleep_start();
}

void wakeup_reason() {
  amount_of_awakeups++;
      Serial.print("Amount of awakeups: "); Serial.println(amount_of_awakeups);

  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0:     // Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     // Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    // Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: // Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      // Serial.println("Wakeup caused by ULP program"); break;
      break;

    default:
      Serial.println("Wakeup caused by unknown reason. Fixing time...");
      set_wifi_connection();

      configTime(0, 0, "pool.ntp.org");
      Serial.print(F("Waiting for NTP time sync... "));

      struct tm timeinfo;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time! Restarting...");
        ESP.restart();
      }

      rtc.setTimeStruct(timeinfo);
      Serial.println("Done!");
      break;
  }
}

void set_wifi_connection() {
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
      Serial.println("\nFailed to connect to WiFi! Restarting...");
      ESP.restart();
    }
  }
  Serial.println(" connected");
}

bool setup_MQ2_sensor() {
  MQ2.setRegressionMethod(1);
  MQ2.setA(987.99); MQ2.setB(-2.162);
  MQ2.init();

  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update();
    calcR0 += MQ2.calibrate(RatioMQ2CleanAir);
  }
  MQ2.setR0(calcR0/10);

  if(isinf(calcR0)) {
    Serial.println("[MQ2] - Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply. Restarting...");
    delay(3000);
    return false;
  }
  if(calcR0 == 0){
    Serial.println("[MQ2] - Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply. Restarting...");
    delay(3000);
    return false;
  }
  return true;
}

bool setup_MQ131_sensor()
{
  MQ131.setRegressionMethod(1);
  MQ131.setA(23.943); MQ131.setB(-1.11);
  MQ131.init();

  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ131.update();
    calcR0 += MQ131.calibrate(RatioMQ131CleanAir);
  }

  MQ131.setR0(calcR0/10);

  if(isinf(calcR0)) {
    Serial.println("[MQ131] - Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    delay(3000);
    return false;
  }
  if(calcR0 == 0){
    Serial.println("[MQ131] - Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    delay(3000);
    return false;
  }
  return true;
}

bool setup_BPM280_sensor()
{
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");

    Serial.println("Trying again...");

    unsigned short tries = 0;

    while(!bmp.begin(0x76)) {
      Serial.print(".");
      delay(1000);
      tries += 1;

      if (tries > 5) {
        Serial.println("\nFailed to connect to BMP280!");
        return false;
      }
    }

    Serial.println(" connected!");
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  return true;
}

bool setup_MICS6418_sensor() {
  pinMode(CO_GPIO_PIN,  INPUT);
  pinMode(NO2_GPIO_PIN, INPUT);
  pinMode(NH3_GPIO_PIN, INPUT);

  return true;
}

int collect_and_save_mq2_sensor_data() {
  if(mq2_is_working) {
    MQ2.update();
    float smoke = MQ2.readSensor();

    // Serial.print("Smoke "); Serial.print(smoke); Serial.println(" PPM");
    return save_variable_to_file(smoke, "smoke");
  }
  Serial.println("MQ2 sensor is not working!");
  return -10;
}

int collect_and_save_mq131_sensor_data() {
  if(mq131_is_working) {
    MQ131.update();
    float co2 = MQ131.readSensor();

    // Serial.print("CO2 "); Serial.print(co2); Serial.println(" PPM");
    return save_variable_to_file(co2, "co2");
  }
  Serial.println("MQ131 sensor is not working!");
  return -11;
}

int collect_and_save_bmp280_sensor_data() {
  if(bmp280_is_working) {
    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure();
    float altitude = bmp.readAltitude(1013.25);

    save_variable_to_file(temperature, "temperature");
    save_variable_to_file(pressure, "pressure");
    save_variable_to_file(altitude, "altitude");
    return 0;
  }

  Serial.println("BMP280 sensor is not working!");
  return -12;
}

int collect_and_save_mics6418_sensor_data() {
  if(mics6418_is_working) {
    float CO_read  = analogRead(CO_GPIO_PIN)  * (MICS_Voltage_Resolution / MAX_ANALOG_STEP);
    float NO2_read = analogRead(NO2_GPIO_PIN) * (MICS_Voltage_Resolution / MAX_ANALOG_STEP);
    float NH3_read = analogRead(NH3_GPIO_PIN) * (MICS_Voltage_Resolution / MAX_ANALOG_STEP);

    save_variable_to_file(CO_read, "co");
    save_variable_to_file(NO2_read, "no2");
    save_variable_to_file(NH3_read, "nh3");
    return 0;
  }
  Serial.println("MICS6418 sensor is not working!");
  return -13;
}

int save_variable_to_file(float variable, const char* variable_label) {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    char msg[100];
    sprintf(msg, "[%s] - Failed to obtain time", variable_label);
    Serial.println(msg);
    return -2;
  }

  char datetime[128];
  strftime(datetime, sizeof(datetime), "%FT%T%z", &timeinfo);


  File file = SPIFFS.open(DATA_FILE_NAME, FILE_APPEND);

  if(!file){
    char msg[100];
    sprintf(msg, "[%s] - Failed to open file", variable_label);
    Serial.println(msg);
    return -1;
  }

  char buffer[256];
  sprintf(
    buffer,
    "{\"%s\": \"%.2f\", \"datetime\": \"%s\"}\n",
    variable_label,
    variable,
    datetime
  );

      // Serial.println(buffer);

  unsigned short return_code = 0;
  char msg[100];
  if(file.print(buffer)) {
    sprintf(msg, "[%s] - Data saved to file", variable_label);
  }

  else {
    sprintf(msg, "[%s] - Failed to save data to file", variable_label);
    return_code = -3;
  }

  Serial.println(msg);
  file.close();
  return return_code;
}

void setup_SPIFFS_file_system() {
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS. Restarting...");
    delay(3000);
    ESP.restart();
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
}

bool time_to_upload() {
  return amount_of_awakeups % AMOUNT_OF_AWAKENINGS_TO_UPLOAD == 0;
}

void upload_to_server() {
  set_mqtt_connection();

  File file = SPIFFS.open(DATA_FILE_NAME);

  if (!file) {
    Serial.println("An Error has occurred while opening file for reading! Going to Deep Sleep. Restarting...");
    delay(3000);
    ESP.restart();
  }

  char buffer2[512];
  unsigned int i = 0;

  while ( file.available() ) {
    char ch = file.read();
    if(ch != '\n') {
      buffer2[i++] = ch;
    }

    else {
      buffer2[i++] = '\0';
      client.publish(MQTT_TOPIC_NAME, buffer2);
      delay(500);
      i = 0;
                          Serial.println(buffer2);
    }
  }

  file.close();
  deleteFile(DATA_FILE_NAME);
}

void set_mqtt_connection() {
  set_wifi_connection();

  client.setServer(MQTT_SERVER_URI, MQTT_SERVER_PORT);

  Serial.print("Connecting to MQTT... ");

  unsigned int mqtt_connection_tries = 0;
  while ( !client.connected() ) {
    if (client.connect("ESP32Producer")) {
      Serial.println("connected!");
    }

    else {
      Serial.println("Failed to connect! Trying again...");
      delay(500);

      if((mqtt_connection_tries++) > 5) {
        Serial.println("Was not able to connect to MQTT server. Restarting...");
        delay(3000);
        ESP.restart();
      }
    }
  }

  client.loop(); // Do not delete this line
}

void deleteFile(String path) {
  Serial.print("Deleting file: "); Serial.print(path); Serial.print("...");

  if (SPIFFS.remove(path)) {
    Serial.println("File deleted");
  }

  else {
    Serial.println("Failed to delete file. Restarting...");
    delay(3000);
    ESP.restart();
  }
}
