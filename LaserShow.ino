#include "Laser.h"
#include <NativeEthernet.h>
#include <ArduinoJson.h>
#include "Settings.h"

const int redLaserPin = 2;
const int greenLaserPin = 3;
const int blueLaserPin = 4;
String previousMessage = "";
byte mac[6];
unsigned long previousMillis;

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

Laser laser(redLaserPin, greenLaserPin, blueLaserPin);
Settings settings;

EthernetClient client;
IPAddress server;

void blinkLed(int onTimeMs, int offTimeMs) {
  digitalWrite(8, HIGH);
  delay(onTimeMs);
  digitalWrite(8, LOW);
  delay(offTimeMs);
}

void settingsMoment() {
  unsigned long durationSeconds = millis() + 10000;
  while (millis() < durationSeconds) {
    blinkLed(100, 100);
    String jsonSettings = Serial.readString();
    Serial.println("jsonsettings length: " + String(jsonSettings.length()));
    if (jsonSettings.length() == 0) {
      continue;
    }

    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, jsonSettings);
    if (err) {
      continue;
    }

    const char* jsonIp = doc["ip"];
    String ip = String(jsonIp);
    Serial.println("Settings ip is: " + ip);
    settings.writeStringToEEPROM(0, ip);
    Serial.println("Settings received. End of settings");
    return;
  }

  Serial.println("End of settings");
}

void setup()
{
  Serial.begin(115200);
  settingsMoment();

  //Set the MAC address.
  teensyMAC(mac);
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    while (true) { }
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    // implement code
  }

  laser.init();
  laser.setScale(0.5);
  laser.setOffset(2048, 2048);

  laser.turnLasersOff();

  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  tryToConnectToServer();
  digitalWrite(8, HIGH);
}

void tryToConnectToServer() {
  const String ipFromEEPROM = settings.readStringFromEEPROM(0);
  char firstChar = ipFromEEPROM.charAt(0);

  if (firstChar == 255 || ipFromEEPROM.length() == 0) {
    return;
  }

  server.fromString(ipFromEEPROM);

  while (!client.connect(server, 50000)) {
    blinkLed(1000, 100);
  }
}

void executeJson(String json)
{
  StaticJsonDocument<8192> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.println(err.f_str());
    return;
  }

  for (JsonObject item : doc.as<JsonArray>()) {
    int red = item["r"];
    int green = item["g"];
    int blue = item["b"];
    int x = item["x"];
    int y = item["y"];

    laser.sendTo(x, y);
    laser.setLaserPower(red, green, blue);
  }
}

String json = "";

void decodeAndExecuteCommands()
{
  while (client.available()) {
    char receivedCharacter = client.read();
    previousMillis = millis();

    switch (receivedCharacter)
    {
      case '[':
        json = "";
        json += receivedCharacter;
        break;
      case ']':
        json += receivedCharacter;
        executeJson(json);
        client.write('d');
        break;
      default:
        json += receivedCharacter;
        break;
    }
  }

  if (millis() - previousMillis > 2) {
    laser.turnLasersOff();
    previousMillis = millis();
  }
}

void loop()
{
  if (client.connected())
  {
    digitalWrite(8, HIGH);
    decodeAndExecuteCommands();
  }
  else {
    laser.turnLasersOff();
    client.stop();
    tryToConnectToServer();
  }
}
