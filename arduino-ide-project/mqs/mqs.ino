#include <WiFi.h>
#include <MQUnifiedsensor.h>



#define         Board                   ("ESP-32")
#define         Voltage_Resolution      (3.3)
#define         ADC_Bit_Resolution      (12)

#define         MQ2_GPIO_PIN            (15)
#define         RatioMQ2CleanAir        (9.83)

#define         MQ131_GPIO_PIN          (2)
#define         RatioMQ131CleanAir      (9.83)


MQUnifiedsensor MQ2  (Board, Voltage_Resolution, ADC_Bit_Resolution, MQ2_GPIO_PIN, "MQ-2");
MQUnifiedsensor MQ131(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ131_GPIO_PIN, "MQ-131");


void setup_MQ2_sensor()
{
  MQ2.setRegressionMethod(1);
  MQ2.setA(987.99); MQ2.setB(-2.162);

  MQ2.init();

  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update();
    calcR0 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(".");
  }
  MQ2.setR0(calcR0/10);
  Serial.println("  done!.");

  if(isinf(calcR0)) {
    Serial.println("[MQ2] - Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while(1);
  }
  if(calcR0 == 0){
    Serial.println("[MQ2] - Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while(1);
  }
}

void setup_MQ131_sensor()
{
  MQ131.setRegressionMethod(1);
  MQ131.setA(23.943); MQ131.setB(-1.11);

  MQ131.init();

  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ131.update();
    calcR0 += MQ131.calibrate(RatioMQ131CleanAir);
    Serial.print(".");
  }

  MQ131.setR0(calcR0/10);
  Serial.println("  done!.");

  if(isinf(calcR0)) {
    Serial.println("[MQ131] - Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while(1);
  }
  if(calcR0 == 0){
    Serial.println("[MQ131] - Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while(1);
  }
}

void setup()
{
  Serial.begin(115200); delay(10);
  setup_MQ2_sensor();
}

void loop()
{
  MQ2.update();
  Serial.print("Smoke "); Serial.print(MQ2.readSensor()); Serial.println(" PPM");


  MQ131.update();
  Serial.print("Ozone "); Serial.print(MQ131.readSensor()); Serial.println(" PPM");
  Serial.println("--------------------");
  delay(500);
}
