// See LICENSE file for details
// Copyright 2016 Florian Link (at) gmx.de
#include "Laser.h"
#include "math.h"

// if this is enabled, pins need to be 10 and 7 in dac init below, but it is a big speedup!
#define MCP4X_PORT_WRITE 1
#define redLaserPin 4
#define greenLaserPin 5
#define blueLaserPin 6

#include "DAC_MCP4X.h"
MCP4X dac;

void turnLasersOff();

Laser::Laser()
{
  _quality = FROM_FLOAT(1. / (LASER_QUALITY));

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

int occurrencesInArray(short arrayToCheck[], int arrayLength, short valueToCheck)
{
  int total = 0;

  for (int i = 0; i < arrayLength; i++)
    if (arrayToCheck[i] == valueToCheck)
    {
      total++;
    }

  return total;
}

// function to fill array with the same data
void fillArray(short arrayToFill[], int arrayLength, short value)
{
  for (int i = 0; i < arrayLength; i++)
  {
    arrayToFill[i] = value;
  }
}

/**
   @brief limits the laser power to reach below the set maxPowerInAudienceRgb.
*/
void Laser::limitLaserPower()
{
  for (int i = 0; i < 3; i++)
  {
    const short currentLaserPower = _currentLaserPowerRgb[i];
    const short maxLaserPower = _maxPowerInAudienceRgb[i];

    if (currentLaserPower > maxLaserPower)
    {
      short newPowerValue = _currentLaserPowerRgb[i];
      while (newPowerValue > _maxPowerInAudienceRgb)
      {
        newPowerValue = abs(newPowerValue - (newPowerValue / 4));
      }
    }
  }
}

float Laser::getGalvoTemperature()
{
  return analogRead(21) * 0.8;
}

float Laser::getFirstFloorTemperature()
{
  return analogRead(20) * 0.8;
}

float Laser::getSecondFloorTemperature()
{
  return analogRead(19) * 0.8;
}

int Laser::getXGalvoRealPosition()
{
  return analogRead(A16);
}

int Laser::getYGalvoRealPosition()
{
  return analogRead(A17);
}

void Laser::setSolenoid(bool state)
{
  digitalWrite(14, state);
}

/**
 * @brief sets the laser in a safe state
 * 
 */
void Laser::emergencyMode()
{
  setSolenoid(LOW);
  turnLasersOff();
}

bool Laser::temperatureToHigh()
{
  float firstFloorTemp = getFirstFloorTemperature();
  float secondFloorTemp = getSecondFloorTemperature();
  float galvoTemp = getGalvoTemperature();

  const float maxTemp = 50;
  return firstFloorTemp < maxTemp && secondFloorTemp < maxTemp && galvoTemp < maxTemp;
}

void Laser::executeHealthCheck()
{
  testGalvo();
  testTemperatureSensors();
}

void Laser::testTemperatureSensors()
{
  float firstFloorTemp = getFirstFloorTemperature();
  float secondFloorTemp = getSecondFloorTemperature();
  float galvoTemp = getGalvoTemperature();

  int strangeValueThreshold = 99;
  if (firstFloorTemp >= strangeValueThreshold && secondFloorTemp >= strangeValueThreshold && galvoTemp >= strangeValueThreshold)
  {
    _emergencyModeActive = true;
    _emergencyModeActiveReason = "Temperature sensor not working";
  }
}

void Laser::testGalvo()
{
  sendTo(-4000, -4000);
  delayMicroseconds(600);
  int xPosition = getXGalvoRealPosition();
  int yPosition = getYGalvoRealPosition();

  sendTo(4000, 4000);
  delayMicroseconds(600);
  int xPosition2 = getXGalvoRealPosition();
  int yPosition2 = getYGalvoRealPosition();

  if (xPosition == xPosition2 || yPosition == yPosition2)
  {
    _emergencyModeActive = true;
    _emergencyModeActiveReason = "Galvo not moving";
  }
}

void Laser::executeIntervalChecks()
{
  unsigned long currentMillis = millis();
  if (currentMillis - _previousMillis > 100)
  {
    _previousMillis = currentMillis;
    if (temperatureToHigh())
    {
      _emergencyModeActive = true;
    }
  }
  if (currentMillis - _previousMillis > 10)
  {
  }
}

void Laser::audienceScanCheck()
{
  if (_yPos < 0)
  {
    limitLaserPower();
  }
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
  analogWrite(redLaserPin, 0);
  analogWrite(greenLaserPin, 0);
  analogWrite(blueLaserPin, 0);

  _currentLaserPowerRgb[0] = 0;
  _currentLaserPowerRgb[1] = 0;
  _currentLaserPowerRgb[2] = 0;
}

/**
   @brief if so the output of the lasers will be reduced, to prevent the beam from burning in objects
*/
void Laser::preventHotSpotsAndStaticBeams()
{
  unsigned long currentMillis = millis();

  // how much non default values are in arrays
  const int xArrayItemCount = _xGalvoPositionsLength - occurrencesInArray(_xGalvoPositions, _xGalvoPositionsLength, 4001);
  const int yArrayItemCount = _yGalvoPositionsLength - occurrencesInArray(_yGalvoPositions, _yGalvoPositionsLength, 4001);

  _xGalvoPositions[xArrayItemCount + 1] = _xPos;
  _yGalvoPositions[yArrayItemCount + 1] = _yPos;

  if (currentMillis - _previousInterval > _xGalvoPositionsLength || xArrayItemCount >= _xGalvoPositionsLength || yArrayItemCount >= _xGalvoPositionsLength)
  {
    // fill arrays with default values
    fillArray(_xGalvoPositions, _xGalvoPositionsLength, 4001);
    fillArray(_yGalvoPositions, _yGalvoPositionsLength, 4001);
    _previousInterval = currentMillis;
  }

  int occurrencesX = occurrencesInArray(_xPos, _xGalvoPositionsLength, _xGalvoPositions);
  int occurrencesY = occurrencesInArray(_yPos, _yGalvoPositionsLength, _yGalvoPositions);

  int maxOccurrences = 8;

  if (numberIsBetween(occurrencesX, 3, maxOccurrences) &&
      numberIsBetween(occurrencesY, 3, maxOccurrences))
  {
    limitLaserPower();
    return;
  }

  if (occurrencesX > maxOccurrences && occurrencesY > maxOccurrences)
  {
    turnLasersOff();
  }
}

/**
   @brief this function calls other functions to check if the laser is operating in a safe way

*/
void Laser::laserSafetyChecks()
{
  preventHotSpotsAndStaticBeams();
  audienceScanCheck();
}

/**
   @brief sets the power of the lasers and checks if the new values are safe

   @param r the power of the red laser
   @param g the power of the green laser
   @param b the power of the blue laser
*/
void Laser::setLaserPower(short r, short g, short b)
{
  _currentLaserPowerRgb[0] = r;
  _currentLaserPowerRgb[1] = g;
  _currentLaserPowerRgb[2] = b;

  laserSafetyChecks();

  analogWrite(redLaserPin, r);
  analogWrite(greenLaserPin, g);
  analogWrite(blueLaserPin, b);
}

/**
   @brief fix the input value if it goes under or over the min and max values.

   @param input the input variable to fix
   @param min the minimum allowed value
   @param max the maximum allowed value
   @return short the value between or equal to the min or max value
*/
short Laser::fixBoundary(short input, short min, short max)
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
void Laser::sendTo(short xpos, short ypos)
{
  _xPos = fixBoundary(xpos, -4000, 4000);
  _yPos = fixBoundary(ypos, -4000, 4000);

  long xNew = TO_INT(_xPos * _scale) + _offsetX;
  long yNew = TO_INT(_yPos * _scale) + _offsetY;

  sendtoRaw(xNew, yNew);
  _latestGalvoMovement = millis();
  laserSafetyChecks(); // this function is called to check if the new position does not fall in an unsafe zone
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
