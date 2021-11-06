/*
   Microchip MCP4901 / MCP4911 / MCP4921 / MCP4902 / MCP4912 / MCP4922 8/10/12-bit DAC driver

   See header file.
*/

#include <SPI.h>
#include "DAC_MCP4X.h"


byte MCP4X::init(byte model, unsigned int vrefA, unsigned int vrefB,
                 int ss_pin, int ldac_pin, boolean autoLatch) {

  const byte resolutions[] = {8, 10, 12};

  bitwidth = resolutions[model & 2];
  dual = model & MCP4X_DUAL;

  if (model & MCP4X_INTREF)
    vrefA = vrefB = 2048;
  this->vrefs[0] = vrefA;
  this->vrefs[1] = vrefB;

  this->ss_pin = ss_pin;
  this->LDAC_pin = ldac_pin;
  this->autoLatch = autoLatch;

  regs[0] = MCP4X_DEFAULTS;
  regs[1] = MCP4X_WRITE_B | MCP4X_DEFAULTS;

  return 1;
}

void MCP4X::begin(boolean beginSPI) {

  pinMode(ss_pin, OUTPUT); // Ensure that SS is set to SPI master mode
  pinMode(LDAC_pin, OUTPUT);

  digitalWrite(ss_pin, HIGH); // Unselect the device
  digitalWrite(LDAC_pin, HIGH); // Un-latch the output

  if (beginSPI)
    SPI.begin();
  configureSPI();
}

void MCP4X::configureSPI() {
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}

// Sets the gain. These DACs support 1x and 2x gain.
// vout = x/2^n * gain * VREF, where x = the argument to out(), n = number of DAC bits
// Example: with 1x gain, set(100) on a 8-bit (256-step) DAC would give
// an output voltage of 100/256 * VREF, while a gain of 2x would give
// vout = 100/256 * VREF * 2
void MCP4X::setGain2x(byte chan, boolean gain2x) {
  chan &= 0x1;
  if (gain2x)
    regs[chan] &= ~MCP4X_GAIN_1X;
  else
    regs[chan] |= MCP4X_GAIN_1X;
}

// Shuts the DAC down. Shutdown current is about 1/50 (typical) of active mode current.
// My measurements say ~160-180 µA active (unloaded vout), ~3.5 µA shutdown.
// Time to settle on an output value increases from ~4.5 µs to ~10 µs, though (according to the datasheet).
void MCP4X::shutdown(byte chan, boolean off) {

  chan &= 0x1;

  if (off)
    regs[chan] &= ~MCP4X_ACTIVE;
  else
    regs[chan] |= MCP4X_ACTIVE;

  write(regs[chan]);
}

void MCP4X::setVoltage(byte chan, float v) {
  float data;

  chan &= 0x1;

  data = (v * 1000 * (1 << bitwidth)) / ((unsigned long) vrefs[chan] * getGain(chan));

  output(chan, (int)round(data));
}

float MCP4X::getVoltageMV(byte chan) {
  unsigned int data;

  chan &= 1;

  data = (regs[chan] >> (12 - bitwidth)) & ((1 << bitwidth) - 1);
  return ((float) vrefs[chan] * data) / (1 << bitwidth) * getGain(chan);
}

// Called by the output* set of functions.
void MCP4X::output2(unsigned short data_A, unsigned short data_B) {

  this->output(MCP4X_CHAN_A, data_A);
  this->output(MCP4X_CHAN_B, data_B);

  if (autoLatch) {
    this->latch();
  }
}

void MCP4X::output(byte chan, unsigned short data) {
  //	unsigned int out;

  const unsigned short maxval = (1 << bitwidth) - 1;

  chan &= 0x1;


  if (data > maxval)
    data = maxval;
  //	data = constrain(data, 0, (1 << bitwidth) - 1);

  // Truncate the unused bits to fit the 8/10/12 bits the DAC accepts
  //	if (this->bitwidth == 12)
  //		data &= 0xfff;
  //	else if (this->bitwidth == 10)
  //		data &= 0x3ff;
  //	else if (this->bitwidth == 8)
  //		data &= 0xff;
  //	data &= (1 << bitwidth) - 1;

  // clear value bits
  regs[chan] &= 0xF000;

  regs[chan] |= data << (12 - bitwidth);

  write(regs[chan]);
  //	// bit 15: 0 for DAC A, 1 for DAC B. (Always 0 for MCP49x1.)
  //	// bit 14: buffer VREF?
  //	// bit 13: gain bit; 0 for 1x gain, 1 for 2x (thus we NOT the variable)
  //	// bit 12: shutdown bit. 1 for active operation
  //	// bits 11 through 0: data
  //	uint16_t out = (chan << 15) | (this->bufferVref << 14)
  //			| ((!this->gain2x) << 13) | (1 << 12)
  //			| (data << (12 - this->bitwidth));
}

void MCP4X::write(unsigned int data) {

  // Drive chip select low
  digitalWrite(ss_pin, LOW);

  SPI.transfer((data & 0xff00) >> 8);
  SPI.transfer(data & 0xff);

  digitalWrite(ss_pin, HIGH);
}

void MCP4X::latch(void) {

  if (LDAC_pin < 0)
    return;

  digitalWrite(LDAC_pin, LOW);
    digitalWrite(LDAC_pin, HIGH);
}
