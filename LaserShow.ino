#include "Laser.h"

const int redLaserPin = 2;
const int greenLaserPin = 3;
const int blueLaserPin = 4;
String previousMessage = "";

Laser laser(redLaserPin, greenLaserPin, blueLaserPin);

void setPwmFrequency() {
  analogWriteFrequency(2, 150000);
  analogWriteFrequency(3, 150000);
  analogWriteFrequency(4, 150000);
}

void setup()
{
  setPwmFrequency();
  laser.init();
  laser.setScale(0.5);
  laser.setOffset(2048, 2048);

  laser.turnLasersOff();
  Serial.begin(115200);

  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
}

void drawCircle(double scale, int randomRed, int randomGreen, int randomBlue) {
  laser.sendTo(SIN(0) / scale, COS(0) / scale);
  laser.setLaserPower(randomRed, randomGreen, randomBlue);
  for (int r = 5; r <= 360; r += 5)
  {
    laser.sendTo(SIN(r) / scale, COS(r) / scale);
  }
}

void circle() {
  int randomRed = random(0, 15);
  int randomGreen = random(0, 15);
  int randomBlue = random(0, 15);

  for (int i = 0; i < 5; i++) {
    randomRed = random(0, 15);
    randomGreen = random(0, 15);
    randomBlue = random(0, 15);

    for (double s = 5; s < 24; s += 0.02) {
      double scale = s;
      drawCircle(scale, randomRed, randomGreen, randomBlue);
    }

    randomRed = random(0, 15);
    randomGreen = random(0, 15);
    randomBlue = random(0, 15);

    for (double s = 24; s > 5; s -= 0.02) {
      double scale = s;
      drawCircle(scale, randomRed, randomGreen, randomBlue);
    }

  }

  laser.turnLasersOff();
}

void globe(int count) {
  laser.setScale(1);
  
  int randomRed = random(0, 15);
  int randomGreen = random(0, 15);
  int randomBlue = random(0, 15);

  for (int i = 0; i < count; i++) {
    laser.setLaserPower(randomRed, randomGreen, randomBlue);
    int pos = random(360) / 5 * 5;
    int diff1 = random(35);
    int diff2 = random(35);
    int diff3 = random(35);
    for (int i = 0; i < 2; i++) {
      for (int r = 0; r <= 360; r += 5)
      {
        laser.sendTo(SIN(r) / 16, COS(r) / 16);
        if (r == pos)   {
          laser.sendTo(SIN(r + diff1) / 32, COS(r + diff2) / 32);
          laser.sendTo(SIN(r + diff2) / 64, COS(r + diff3) / 64);
          laser.sendTo(0, 0);
          laser.sendTo(SIN(r + diff3) / 64, COS(r + diff3) / 64);
          laser.sendTo(SIN(r + diff2) / 32, COS(r + diff1) / 32);
          laser.sendTo(SIN(r) / 16, COS(r) / 16);
        }
      }
    }
  }

  laser.setScale(0.5);
}

void loop()
{
  circle();
  globe(2000);
}
