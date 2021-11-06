// See LICENSE file for details
// Copyright 2016 Florian Link (at) gmx.de
#if !(defined(__IMXRT1062__) && defined(ARDUINO_TEENSY41))
  #error This is designed only for Teensy 4.1. Please check your Tools-> Boards
#endif

#define WEBSOCKETS_USE_ETHERNET     true
#define USE_NATIVE_ETHERNET         true

#include <WebSockets2_Generic.h>

using namespace websockets2_generic;

#include "Laser.h"
const byte maxClients = 4;

byte mac[6];
const uint16_t port = 81;
WebsocketsClient clients[maxClients];
WebsocketsServer server;

void teensyMAC(uint8_t *mac) {
  for (uint8_t by = 0; by < 2; by++) mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  for (uint8_t by = 0; by < 4; by++) mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
}

Laser laser;

void setup()
{
  teensyMAC(mac);
  Ethernet.begin(mac);
  server.listen(port);

  laser.init();
  laser.setScale(1);
  laser.setOffset(2048, 2048);

  byte pins[7] = {2, A2, 3, 5, 6, 9, 13};
  for (unsigned int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) pinMode(i, OUTPUT);

  analogWriteFrequency(4, 292968.75);
  analogWriteFrequency(5, 292968.75);
  analogWriteFrequency(6, 292968.75);

  laser.off();
}

String getDataFromMessage(String message) {
  return message.substring(1, message.length());
}

void executeActionsByMessage(String data) {
  int value = getDataFromMessage(data).toInt();
  char firstChar = data.charAt(0);

  switch (firstChar) {
    case 'r':
      
      break;
    case 'g':
      // statements
      break;
    case 'b':
      break;
    case 'x':
      break;
    case 'y':
      break;
    default:
      // statements
      break;
  }
}

void handleMessage(WebsocketsClient &client, WebsocketsMessage message)
{
  auto data = message.data();
}

int8_t getFreeClientIndex()
{
  // If a client in our list is not available, it's connection is closed and we
  // can use it for a new client.
  for (byte i = 0; i < maxClients; i++)
  {
    if (!clients[i].available())
      return i;
  }

  return -1;
}

void listenForClients()
{
  if (server.poll())
  {
    int8_t freeIndex = getFreeClientIndex();

    if (freeIndex >= 0)
    {
      WebsocketsClient newClient = server.accept();
      newClient.onMessage(handleMessage);
      clients[freeIndex] = newClient;
    }
  }
}

void pollClients()
{
  for (byte i = 0; i < maxClients; i++)
  {
    clients[i].poll();
  }
}

void loop()
{
  listenForClients();
  pollClients();
}
