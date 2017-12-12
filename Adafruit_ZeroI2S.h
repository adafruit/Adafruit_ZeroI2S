#ifndef ADAFRUIT_ZEROI2S_H
#define ADAFRUIT_ZEROI2S_H

#include <Arduino.h>

typedef enum _I2SSlotSize {
	I2S_8_BIT = 0,
	I2S_16_BIT,
	I2S_24_BIT,
	I2S_32_BIT
} I2SSlotSize;

#define I2S_NUM_SLOTS 2

class Adafruit_ZeroI2S {
public:
	Adafruit_ZeroI2S(uint8_t FS_PIN, uint8_t SCK_PIN, uint8_t TX_PIN, uint8_t RX_PIN);
	Adafruit_ZeroI2S();
	~Adafruit_ZeroI2S() {}

	bool begin(I2SSlotSize width, int fs_freq, int mck_mult = 256);

	void enableMCLK();
	void disableMCLK();

	bool txReady();
	void write(int32_t left, int32_t right);

private:
	uint8_t _fs, _sck, _tx, _rx;
};

#endif
