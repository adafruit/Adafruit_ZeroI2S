
/*!
 * @file Adafruit_ZeroI2S.h
 *
 * This is a library for the I2S peripheral on SAMD21 and SAMD51 devices
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Dean Miller for Adafruit Industries.
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#ifndef ADAFRUIT_ZEROI2S_H
#define ADAFRUIT_ZEROI2S_H

#include <Arduino.h>

/**************************************************************************/
/*!
    @brief  available I2S slot sizes
*/
/**************************************************************************/
typedef enum _I2SSlotSize {
  I2S_8_BIT = 0,
  I2S_16_BIT,
  I2S_24_BIT,
  I2S_32_BIT
} I2SSlotSize;

/**************************************************************************/
/*!
    @brief  number of I2S slots to use (stereo)
*/
/**************************************************************************/
#define I2S_NUM_SLOTS 2

/**************************************************************************/
/*!
    @brief  Class that stores state and functions for interacting with I2S
   peripheral on SAMD21 and SAMD51 devices
*/
/**************************************************************************/
class Adafruit_ZeroI2S {
public:
  Adafruit_ZeroI2S(uint8_t FS_PIN, uint8_t SCK_PIN, uint8_t TX_PIN,
                   uint8_t RX_PIN);
  Adafruit_ZeroI2S();
  ~Adafruit_ZeroI2S() {}

  bool begin(I2SSlotSize width, int fs_freq, int mck_mult = 256);

  void enableTx();
  void disableTx();
  void enableRx();
  void disableRx();
  void enableMCLK();
  void disableMCLK();

  bool txReady();
  bool rxReady();
  void write(int32_t left, int32_t right);
  void read(int32_t *left, int32_t *right);

private:
  int8_t _fs, _sck, _tx, _rx;
#ifndef __SAMD51__
  int8_t _i2sserializer, _i2sclock;
#endif
};

#endif
