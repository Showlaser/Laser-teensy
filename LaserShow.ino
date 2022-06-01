#include "Laser.h"
#include <NativeEthernet.h>
#include <ArduinoJson.h>
#include "Settings.h"
#include <queue>

const int redLaserPin = 2;
const int greenLaserPin = 3;
const int blueLaserPin = 4;
String previousMessage = "";
unsigned long previousMillis;
bool stopCommandReceived = false;

struct laserMessage {
  int redLaserPower;
  int greenLaserPower;
  int blueLaserPower;
  int x;
  int y;
};

std::queue<laserMessage> laserMessages;
unsigned long timePatternShouldStop;

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

void setPwmFrequency() {
  analogWriteFrequency(2, 150000);
  analogWriteFrequency(3, 150000);
  analogWriteFrequency(4, 150000);
}

void setup()
{
  settingsMoment();
  setPwmFrequency();
  //Set the MAC address.
  byte mac[6];
  teensyMAC(mac);
  Ethernet.begin(mac);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    while (true) { }
  }

  laser.init();
  laser.setScale(0.5);
  laser.setOffset(2048, 2048);

  laser.turnLasersOff();
  Serial.begin(115200);

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

void deserializeJsonString(String jsonString)
{
  StaticJsonDocument<16384> doc;
  DeserializationError err = deserializeJson(doc, jsonString);
  if (err) {
    Serial.println(err.f_str());
    return;
  }

  stopCommandReceived = doc["sp"];
  long durationTime = doc["dms"];
  timePatternShouldStop = millis() + durationTime;
  JsonArray jsonArray = doc["m"].as<JsonArray>();

  for (JsonObject item : jsonArray) {
    laserMessage message;
    message.redLaserPower = item["r"];
    message.greenLaserPower = item["g"];
    message.blueLaserPower = item["b"];
    message.x = item["x"];
    message.y = item["y"];

    laserMessages.push(message);
  }
}

void decodeCommands()
{
  String json = "";
  while (client.available()) {
    char receivedCharacter = client.read();
    previousMillis = millis();

    switch (receivedCharacter)
    {
      case '(':
        clearMessageQueue();
        json = "";
        break;
      case ')':
        clearMessageQueue();
        deserializeJsonString(json);
        json = "";
        break;
      default:
        json += receivedCharacter;
        break;
    }
  }
}

void clearMessageQueue() {
  std::queue<laserMessage> empty;
  std::swap(laserMessages, empty);
}

void executeMessages() {
  bool messagesCannotBeExecuted = timePatternShouldStop < millis() || laserMessages.empty() || stopCommandReceived;
  if (messagesCannotBeExecuted) {
    laser.turnLasersOff();

    if (!laserMessages.empty()) {
      clearMessageQueue();
    }
    return;
  }

  laserMessage message = laserMessages.front();
  laserMessages.pop();
  laserMessages.push(message);

  laser.sendTo(message.x, message.y);
  laser.setLaserPower(message.redLaserPower, message.greenLaserPower, message.blueLaserPower);
}

void loop()
{
  if (client.connected())
  {
    digitalWrite(8, HIGH);
    decodeCommands();
    executeMessages();
  }
  else {
    laser.turnLasersOff();
    client.stop();
    tryToConnectToServer();
  }
}
