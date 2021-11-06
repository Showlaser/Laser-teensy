#include "Laser.h"
#include <NativeEthernet.h>

byte mac[6];

void teensyMAC(uint8_t *mac) {
  for (uint8_t by = 0; by < 2; by++) mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  for (uint8_t by = 0; by < 4; by++) mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
}

Laser laser;
IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);
boolean alreadyConnected = false; // whether or not the client was connected previously

void setup()
{
  // Set the MAC address.
  teensyMAC(mac);
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    // implement code
  }

  server.begin();

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

void loop()
{
  // wait for a new client:
  EthernetClient client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    while (client.connected()) {

      while (client.available() > 0) {
        char c = client.read();
      }
    }

    client.stop();
  }
}
