// See LICENSE file for details
// Copyright 2016 Florian Link (at) gmx.de
#include "Laser.h"
#include "math.h"

// if this is enabled, pins need to be 10 and 7 in dac init below, but it is a big speedup!
#define MCP4X_PORT_WRITE 1

#include "DAC_MCP4X.h"
MCP4X dac;

void turnLasersOff();

Laser::Laser(int redLaserPin, int greenLaserPin, int blueLaserPin)
{
  _quality = FROM_FLOAT(1. / (LASER_QUALITY));
  _redLaserPin = redLaserPin;
  _greenLaserPin = greenLaserPin;
  _blueLaserPin = blueLaserPin;

  _x = 0;
  _y = 0;
  _oldX = 0;
  _oldY = 0;

  _state = 0;

  _scale = 1;
  _offsetX = 0;
  _offsetY = 0;

  _moved = 0;
  _maxMove = -1;
  _laserForceOff = false;
  resetClipArea();

  _enable3D = false;
  _zDist = 1000;
}

void Laser::init()
{
  dac.init(MCP4X_4822, 5000, 5000, 10, 7, 1);
  dac.setGain2x(MCP4X_CHAN_A, 0);
  dac.setGain2x(MCP4X_CHAN_B, 0);
  dac.begin(1);
}

void Laser::sendToDAC(int x, int y)
{
#ifdef LASER_SWAP_XY
  int x1 = y;
  int y1 = x;
#else
  int x1 = x;
  int y1 = y;
#endif
#ifdef LASER_FLIP_X
  x1 = 4095 - x1;
#endif
#ifdef LASER_FLIP_Y
  y1 = 4095 - y1;
#endif
  dac.output2(x1, y1);
}

void Laser::resetClipArea()
{
  _clipXMin = 0;
  _clipYMin = 0;
  _clipXMax = 4095;
  _clipYMax = 4095;
}

void Laser::setClipArea(long x, long y, long x1, long y1)
{
  _clipXMin = x;
  _clipYMin = y;
  _clipXMax = x1;
  _clipYMax = y1;
}

bool Laser::numberIsBetween(int value, int min, int max)
{
  return value >= min && value <= max;
}

/**
   @brief turns off the lasers, can be used in cases of emergencies

*/
void Laser::turnLasersOff()
{
  analogWrite(_redLaserPin, 0);
  analogWrite(_greenLaserPin, 0);
  analogWrite(_blueLaserPin, 0);

  _currentLaserPowerRgb[0] = 0;
  _currentLaserPowerRgb[1] = 0;
  _currentLaserPowerRgb[2] = 0;
}

/**
   @brief sets the power of the lasers and checks if the new values are safe

   @param r the power of the red laser
   @param g the power of the green laser
   @param b the power of the blue laser
*/
void Laser::setLaserPower(int r, int g, int b)
{
  if (_currentLaserPowerRgb[0] == r &&
      _currentLaserPowerRgb[1] == g &&
      _currentLaserPowerRgb[2] == b)
  {
    return;
  }
  if (r > 7) {
    r = 7;
  }
  if (g > 7) {
    g = 7;
  }
  if (b > 7) {
    b = 7;
  }

  r = fixBoundary(r, 0, 255);
  g = fixBoundary(g, 0, 255);
  b = fixBoundary(b, 0, 255);

  _currentLaserPowerRgb[0] = r;
  _currentLaserPowerRgb[1] = g;
  _currentLaserPowerRgb[2] = b;

  analogWrite(_redLaserPin, r);
  analogWrite(_greenLaserPin, g);
  analogWrite(_blueLaserPin, b);
}

/**
   @brief sets the input value to the max or min value if it goes under or over the min and max values.

   @param input the input variable to fix
   @param min the minimum allowed value
   @param max the maximum allowed value
   @return int the value between or equal to the min or max value
*/
int Laser::fixBoundary(int input, int min, int max)
{
  if (input < min)
  {
    input = min;
  }
  if (input > max)
  {
    max = max;
  }
  return input;
}

/**
   @brief sends the mirrors of the galvo to the specified location

   @param xpos the position of the x mirror in the galvo
   @param ypos the position of the y mirror in the galvo
*/
void Laser::sendTo(int xPos, int yPos)
{
  xPos = fixBoundary(xPos, -4000, 4000);
  yPos = fixBoundary(yPos, -4000, 4000);

  int delayMicrosecondsTimeX = (_xPos > xPos ? _xPos - xPos : xPos - _xPos) / 3.5;
  int delayMicrosecondsTimeY = (_yPos > yPos ? _yPos - yPos : yPos - _yPos) / 3.5;
  int delayTime = delayMicrosecondsTimeX > delayMicrosecondsTimeY ? delayMicrosecondsTimeX : delayMicrosecondsTimeY;
  if (delayTime < 25) {
    delayTime = 25;
  }
  if (delayTime > 800) {
    delayTime = 800;
  }

  _xPos = xPos;
  _yPos = yPos;

  long xNew = TO_INT(_xPos * _scale) + _offsetX;
  long yNew = TO_INT(_yPos * _scale) + _offsetY;

  sendtoRaw(xNew, yNew);
  delayMicroseconds(delayTime);
}

void Laser::sendtoRaw(long xNew, long yNew)
{
  // devide into equal parts, using _quality
  long fdiffx = xNew - _x;
  long fdiffy = yNew - _y;
  long diffx = TO_INT(abs(fdiffx) * _quality);
  long diffy = TO_INT(abs(fdiffy) * _quality);

  // use the bigger direction
  if (diffx < diffy)
  {
    diffx = diffy;
  }

  fdiffx = FROM_INT(fdiffx) / diffx;
  fdiffy = FROM_INT(fdiffy) / diffx;
  // interpolate in FIXPT
  FIXPT tmpx = 0;
  FIXPT tmpy = 0;
  for (int i = 0; i < diffx - 1; i++)
  {
    tmpx += fdiffx;
    tmpy += fdiffy;
    sendToDAC(_x + TO_INT(tmpx), _y + TO_INT(tmpy));
  }

  _x = xNew;
  _y = yNew;
  sendToDAC(_x, _y);
}

void Laser::setScale(float scale)
{
  _scale = FROM_FLOAT(scale);
}

void Laser::setOffset(long offsetX, long offsetY)
{
  _offsetX = offsetX;
  _offsetY = offsetY;
}
