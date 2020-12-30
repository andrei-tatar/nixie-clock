#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Timezone.h>
#include <TimeLib.h>
#include <Ticker.h>
#include <ESP8266WebServer.h>
#include <uri/UriBraces.h>

#define SCL D1
#define SDA D2

#define H_ADDR 0x20
#define M_ADDR 0x22
#define S_ADDR 0x23

#define SSID "ssid"
#define PASS "pass"

void writeValue(uint8_t i2cAddr, uint8_t value);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
TimeChangeRule EEST = {"EEST", Last, Sun, Mar, 2, 180}; //DST
TimeChangeRule EET = {"EET ", Last, Sun, Oct, 3, 120};  //standard
Timezone timezone(EEST, EET);
Ticker displayTime, updateTime;
ESP8266WebServer server(80);

void showTime()
{
  auto localTime = timezone.toLocal(timeClient.getEpochTime());

  writeValue(H_ADDR, hour(localTime));
  writeValue(M_ADDR, minute(localTime));
  writeValue(S_ADDR, second(localTime));
}

void setup()
{
  Wire.begin(SDA, SCL);
  WiFi.begin(SSID, PASS);

  timeClient.begin();

  WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &ip) { timeClient.update(); });
  updateTime.attach(60, []() { timeClient.update(); });
  displayTime.attach(.1, []() { showTime(); });

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/plain", "This is your nixie clock");
  });

  server.on(UriBraces("/{}"), HTTP_GET, []() {
    auto arg = server.pathArg(0);
    if (arg.length() == 1 && arg[0] >= '0' && arg[0] <= '9')
    {
      displayTime.detach();
      uint8_t digit = arg[0] - '0';
      digit = digit * 10 + digit;
      writeValue(H_ADDR, digit);
      writeValue(M_ADDR, digit);
      writeValue(S_ADDR, digit);
    }
    else if (arg.equalsIgnoreCase("count"))
    {
      writeValue(H_ADDR, 0);
      writeValue(M_ADDR, 0);
      writeValue(S_ADDR, 0);
      static uint8_t digit;
      digit = 1;

      displayTime.attach(1, []() {
        uint8_t d = digit * 10 + digit;

        writeValue(H_ADDR, d);
        writeValue(M_ADDR, d);
        writeValue(S_ADDR, d);

        digit++;
        if (digit == 10)
        {
          digit = 0;
        }
      });
    }
    else if (arg.equalsIgnoreCase("time"))
    {
      displayTime.attach(.1, []() { showTime(); });
    }
    server.send(200, "text/plain", "OK");
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not found");
  });

  server.begin();
}

void loop()
{
  server.handleClient();
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
