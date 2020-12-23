#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Timezone.h>
#include <TimeLib.h>

#define SCL D1
#define SDA D2

#define H_ADDR 0x20
#define M_ADDR 0x22
#define S_ADDR 0x23

#define GTM_OFFSET 2

void writeValue(uint8_t i2cAddr, uint8_t value);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

TimeChangeRule EEST = {"EEST", Last, Sun, Mar, 2, 180}; //DST
TimeChangeRule EET = {"EET ", Last, Sun, Oct, 3, 120};  //standard
Timezone timezone(EEST, EET);

void setup()
{
  WiFi.begin("SSID", "pass");
  Wire.begin(SDA, SCL);

  while (WiFi.status() != WL_CONNECTED)
  {
    writeValue(H_ADDR, rand());
    writeValue(M_ADDR, rand());
    writeValue(S_ADDR, rand());
    delay(500);
  }

  timeClient.begin();
}

void loop()
{
  timeClient.update();

  auto localTime = timezone.toLocal(timeClient.getEpochTime());

  writeValue(H_ADDR, hour(localTime));
  writeValue(M_ADDR, minute(localTime));
  writeValue(S_ADDR, second(localTime));

  delay(1000);
}

void writeValue(uint8_t i2cAddr, uint8_t value)
{
  uint8_t map[] = {1, 0, 6, 9, 8, 7, 2, 5, 4, 3};
  uint8_t high = map[(value / 10) % 10];
  uint8_t low = map[value % 10];
  uint8_t portValue = 0;
  if (high & 0x01)
    portValue |= 1 << 7;
  if (high & 0x02)
    portValue |= 1 << 5;
  if (high & 0x04)
    portValue |= 1 << 4;
  if (high & 0x08)
    portValue |= 1 << 6;

  if (low & 0x01)
    portValue |= 1 << 0;
  if (low & 0x02)
    portValue |= 1 << 2;
  if (low & 0x04)
    portValue |= 1 << 3;
  if (low & 0x08)
    portValue |= 1 << 1;

  Wire.beginTransmission(i2cAddr);
  Wire.write(portValue);
  Wire.endTransmission(true);
}
