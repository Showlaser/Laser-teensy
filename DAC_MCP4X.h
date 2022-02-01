#ifndef _DAC_MCP4X_H
#define _DAC_MCP4X_H

#include <Arduino.h>
#include <inttypes.h>

//
// Params
//
#define MCP4X_PORT_WRITE 1
#define MCP4X_NO_LDAC -1
#ifndef MCP4X_PORT_WRITE
#define MCP4X_PORT_WRITE 0
#endif
#ifndef MCP4X_DEFAULTS
#define MCP4X_DEFAULTS (MCP4X_BUFFERED | MCP4X_GAIN_1X | MCP4X_ACTIVE)
#endif

//
// Constants
//

// Bits
#define MCP4X_WRITE_B (1 << 15)
#define MCP4X_BUFFERED (1 << 14)
#define MCP4X_GAIN_1X (1 << 13)
#define MCP4X_ACTIVE (1 << 12)

// Channels
#define MCP4X_CHAN_A 0
#define MCP4X_CHAN_B 1

// Model identifiers
#define MCP4X_12_BITS 2
#define MCP4X_INTREF (1 << 2)
#define MCP4X_DUAL (1 << 3)

#define MCP4X_4822 (MCP4X_12_BITS | MCP4X_INTREF | MCP4X_DUAL)

class MCP4X
{
public:
	byte init(byte model,
			  unsigned int vrefA = 5000, unsigned int vrefB = 5000,
			  int ss_pin = SS, int ldac_pin = MCP4X_NO_LDAC, boolean autoLatch = 1);
	void begin(boolean beginSPI = 1);

	void configureSPI();

	void setVref(byte chan, unsigned int vref) { vrefs[chan & 1] = vref; }
	void setVref(unsigned int vref) { vrefs[0] = vrefs[1] = vref; }
	void setBuffer(byte chan, boolean buffered);
	void setGain2x(byte chan, boolean gain2x = 1);
	void setAutoLatch(boolean enabled = 1) { autoLatch = enabled; }
	void shutdown(byte chan, boolean off = 1);

	void output(byte _chan, unsigned int _out);
	void output(unsigned int data) { output(MCP4X_CHAN_A, data); };
	/* same as output(), but having A/B makes more sense for dual DACs */
	void outputA(unsigned int data) { output(MCP4X_CHAN_A, data); }
	void outputB(unsigned int data) { output(MCP4X_CHAN_B, data); }
	void output2(unsigned int _out, unsigned int _out2); // For MCP49x2

	//
	// output-like methods that takes a voltage argument
	//
	void setVoltage(byte chan, float v);

	int getGain(byte chan) { return regs[chan & 1] & MCP4X_GAIN_1X ? 1 : 2; }
	float getVoltageMV(byte chan);

	/* Actually change the output, if the LDAC pin isn't inted to ground */
	void latch(void);

	void setAutomaticallyLatchDual(bool latch) { autoLatch = latch; };

private:
	unsigned int vrefs[2];
	unsigned int regs[2];
	boolean dual;
	int ss_pin;
	int LDAC_pin;
	int bitwidth;
	boolean autoLatch; /* call latch() automatically after output2() has been called? */

	void write(unsigned int data);
};

#endif
