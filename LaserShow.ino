#include "Laser.h"
#include <NativeEthernet.h>
#include <ArduinoJson.h>

const int redLaserPin = 2;
const int greenLaserPin = 3;
const int blueLaserPin = 4;
const int xGalvoFeedbackPin = A0;
const int yGalvoFeedbackPin = A1;
const int solenoidPin = 14;

byte mac[6];

void teensyMAC(uint8_t *mac)
{
  for (uint8_t by = 0; by < 2; by++)
  {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++)
  {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
}

Laser laser(redLaserPin, greenLaserPin, blueLaserPin, xGalvoFeedbackPin, yGalvoFeedbackPin, solenoidPin);
IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);

bool alreadyConnected = false; // whether or not the client was connected previously
bool messageStarted = false;

void setAnalogWriteFrequency()
{
  analogWriteFrequency(redLaserPin, 292968.75);
  analogWriteFrequency(greenLaserPin, 292968.75);
  analogWriteFrequency(blueLaserPin, 292968.75);
}

void setInputPins()
{
  byte pins[6] = {xGalvoFeedbackPin, yGalvoFeedbackPin};
  for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
  {
    pinMode(i, INPUT);
  }
}

void setOutputPins()
{
  byte pins[7] = {redLaserPin, greenLaserPin, blueLaserPin, 5, 8, 9, 13};
  for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
  {
    pinMode(i, OUTPUT);
  }
}

void setup()
{
  // Set the MAC address.
  //  teensyMAC(mac);
  //  Ethernet.begin(mac, ip);
  //
  //  // Check for Ethernet hardware present
  //  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  //  {
  //    while (true)
  //    {
  //      delay(1); // do nothing, no point running without Ethernet hardware
  //    }
  //  }
  //  if (Ethernet.linkStatus() == LinkOFF)
  //  {
  //    // implement code
  //  }
  //
  //  server.begin();

  laser.init();
  laser.setScale(0.5);
  laser.setOffset(2048, 2048);

  setAnalogWriteFrequency();
  setInputPins();
  setOutputPins();

  //analogWriteFrequency(4, 292968.75);
  //analogWriteFrequency(5, 292968.75);
  //analogWriteFrequency(6, 292968.75);

  //laser.turnLasersOff();
}

void executeJson(String json)
{
  StaticJsonDocument<64> doc;
  deserializeJson(doc, json);
  JsonArray rgbxy = doc["D"];

  short red = rgbxy[0];
  short green = rgbxy[1];
  short blue = rgbxy[2];
  short x = rgbxy[3];
  short y = rgbxy[4];

  laser.setLaserPower(red, green, blue);
  laser.sendTo(x, y);
}

void decodeAndExecuteCommands(EthernetClient client)
{
  String json = "";
  while (client.available() > 0)
  {
    char receivedCharacter = client.read();
    switch (receivedCharacter)
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
      json += receivedCharacter;
      break;
    }
  }
}

void handleEthernetServer()
{
  EthernetClient client = server.available();
  if (client)
  {
    while (client.connected())
    {
      decodeAndExecuteCommands(client);
    }

    client.stop();
  }
}

void loop()
{
  //  laser.executeIntervalChecks();
  //  if (laser.emergencyModeActive())
  //  {
  //    laser.executeIntervalChecks();
  //    return;
  //  }
  //
  //  handleEthernetServer();
  laser.sendTo(-4000, -4000);
  delay(2000);
  laser.sendTo(4000, 4000);
  delay(2000);
}
