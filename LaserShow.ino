#include "Laser.h"
#include <NativeEthernet.h>
#include <ArduinoJson.h>

byte mac[6];

void teensyMAC(uint8_t *mac)
{
  for (uint8_t by = 0; by < 2; by++)
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  for (uint8_t by = 0; by < 4; by++)
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
}

Laser laser;
IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);

bool alreadyConnected = false; // whether or not the client was connected previously
bool messageStarted = false;

void setup()
{
  // Set the MAC address.
  teensyMAC(mac);
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    // implement code
  }

  server.begin();

  laser.init();
  laser.setScale(0.5);
  laser.setOffset(2048, 2048);

  byte pins[7] = {2, A2, 3, 5, 6, 9, 13};
  for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    pinMode(i, OUTPUT);

  analogWriteFrequency(4, 292968.75);
  analogWriteFrequency(5, 292968.75);
  analogWriteFrequency(6, 292968.75);

  laser.turnLasersOff();
}

void executeJson(String json)
{
  StaticJsonDocument<64> doc;
  deserializeJson(doc, json);
  JsonArray rgbxy = doc["Rgbxy"];

  short red = rgbxy[0];
  short green = rgbxy[1];
  short blue = rgbxy[2];
  short x = rgbxy[3];
  short y = rgbxy[4];

  //laser.setLaserPower(red, green, blue);
  //laser.sendTo(x, y);
}

void handleMessages(EthernetClient client)
{
  String json = "";
  while (client.available() > 0)
  {
    char c = client.read();
    switch (c)
    {
      case '{':
        messageStarted = true;
        break;
      case '}':
        messageStarted = false;
        executeJson(json);
        json = "";
        break;
      default:
        json += c;
        break;
    }
  }
}

void loop()
{
  // wait for a new client:
  EthernetClient client = server.available();

  // when the client sends the first byte, say hello:
  if (client)
  {
    while (client.connected())
    {
      handleMessages(client);
    }

    client.stop();
  }
}
