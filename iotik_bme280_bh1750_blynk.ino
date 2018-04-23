#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750FVI.h>
#include <ArduinoJson.h>
#include <SimpleTimer.h>

// Точка доступа Wi-Fi
char ssid[] = "IOTIK";
char pass[] = "Terminator812";

// Датчик освещенности
BH1750FVI bh1750;

// Датчик температуры/влажности и атмосферного давления
Adafruit_BME280 bme280;

// Выход реле
#define RELAY_PIN 12

// Датчик влажности почвы емкостной
#define MOISTURE_SENSOR A0
const float air_value1 = 422.0;
const float water_value1 = 240.0;
const float moisture_0 = 0.0;
const float moisture_100 = 100.0;

// Периоды для таймеров
#define BME280_UPDATE_TIME  5000
#define BH1750_UPDATE_TIME  5000
#define ANALOG_UPDATE_TIME  5000
#define IOT_UPDATE_TIME     10000

// Таймеры
BlynkTimer timer_bme280;
BlynkTimer timer_bh1750;
BlynkTimer timer_analog;
BlynkTimer timer_iot;

// Состояния управляющих устройств
int relay_control = 0;

// Параметры IoT сервера
char auth[] = "c6c6513b7997426181fd8d386e1c33b1";
IPAddress blynk_ip(139, 59, 206, 133);

// Параметры сенсоров для IoT сервера
#define sensorCount 5
char* sensorNames[] = {"air_temp", "air_hum", "air_press", "sun_light", "soil_hum"};
float sensorValues[sensorCount];
// Номера датчиков
#define air_temp     0x00
#define air_hum      0x01
#define air_press    0x02
#define sun_light    0x03
#define soil_hum     0x04

// Максимальное время ожидания ответа от сервера
#define IOT_TIMEOUT1 5000
#define IOT_TIMEOUT2 100

// Таймер ожидания прихода символов с сервера
long timer_iot_timeout = 0;

// Размер приемного буффера
#define BUFF_LENGTH 256

// Приемный буфер
char buff[BUFF_LENGTH] = "";

void setup()
{
  // Инициализация последовательного порта
  Serial.begin(115200);
  delay(512);

  // Инициализация Wi-Fi и поключение к серверу Blynk
  Blynk.begin(auth, ssid, pass, blynk_ip, 8442);
  Serial.println();

  // Инициализация выхода реле
  pinMode(RELAY_PIN, OUTPUT);

  // Однократный опрос датчиков
  readSensorBME280(); readSensorBME280();
  readSensorBH1750(); readSensorBH1750();
  readSensorANALOG(); readSensorANALOG();

  // Вывод в терминал данных с датчиков
  printAllSensors();

  // Инициализация таймеров
  timer_bme280.setInterval(BME280_UPDATE_TIME, readSensorBME280);
  timer_bh1750.setInterval(BH1750_UPDATE_TIME, readSensorBH1750);
  timer_analog.setInterval(ANALOG_UPDATE_TIME, readSensorANALOG);
  timer_iot.setInterval(IOT_UPDATE_TIME, sendDataBlynk);
}

void loop()
{
  Blynk.run();
  timer_bme280.run();
  timer_bh1750.run();
  timer_analog.run();
  timer_iot.run();
}

// Чтение датчика BH1750
void readSensorBH1750()
{
  Wire.begin(0, 2);         // Инициализация I2C на выводах 0, 2
  Wire.setClock(100000L);   // Снижение тактовой частоты для надежности
  bh1750.begin();           // Инициализация датчика
  bh1750.setMode(Continuously_High_Resolution_Mode); // Установка разрешения датчика
  sensorValues[sun_light] = bh1750.getAmbientLight();
}

// Чтение датчика BME280
void readSensorBME280()
{
  Wire.begin(4, 5);         // Инициализация I2C на выводах 4, 5
  Wire.setClock(100000L);   // Снижение тактовой частоты для надежности
  bme280.begin();
  sensorValues[air_temp] = bme280.readTemperature();
  sensorValues[air_hum] = bme280.readHumidity();
  sensorValues[air_press] = bme280.readPressure() * 7.5006 / 1000.0;
}

// Чтение аналоговых датчиков
void readSensorANALOG()
{
  float sensor_data = analogRead(MOISTURE_SENSOR);
  //Serial.println("ADC0: " + String(sensor_data, 3));
  sensorValues[soil_hum] = map(sensor_data, air_value1, water_value1, moisture_0, moisture_100);
}

// Print sensors data to terminal
void printAllSensors()
{
  for (int i = 0; i < sensorCount; i++)
  {
    Serial.print(sensorNames[i]);
    Serial.print(" = ");
    Serial.println(sensorValues[i]);
  }
  Serial.println();
}

// Отправка данных на сервер Blynk
void sendDataBlynk()
{
  Blynk.virtualWrite(V0, sensorValues[air_temp]); delay(50);
  Blynk.virtualWrite(V1, sensorValues[air_hum]); delay(50);
  Blynk.virtualWrite(V2, sensorValues[air_press]); delay(50);
  Blynk.virtualWrite(V3, sensorValues[sun_light]); delay(50);
  Blynk.virtualWrite(V4, sensorValues[soil_hum]); delay(50);
}

// Управление реле с Blynk
BLYNK_WRITE(V5)
{
  relay_control = param.asInt();
  Serial.print("Relay control: ");
  Serial.println(relay_control);
  digitalWrite(RELAY_PIN, relay_control);
}

