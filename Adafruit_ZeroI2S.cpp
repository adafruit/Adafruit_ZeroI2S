/*!
 * @file Adafruit_ZeroI2S.cpp
 *
 * @mainpage Adafruit I2S peripheral driver for SAMD21 and SAMD51 chips
 *
 * @section intro_sec Introduction
 *
 *  I2S peripheral driver for SAMD21 and SAMD51 chips
 *
 *
 *  Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing
 *  products from Adafruit!
 *
 * @section author Author
 *
 * Written by Dean Miller for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#include "Adafruit_ZeroI2S.h"
#include "wiring_private.h"

#ifndef DEBUG_PRINTLN
#define DEBUG_PRINTLN Serial.println ///< where to print the debug output
#endif

/**************************************************************************/
/*!
    @brief  Class Constructor
        @param FS_PIN frame sync pin
        @param SCK_PIN bit clock pin
        @param TX_PIN data output pin
        @param RX_PIN data input pin
*/
/**************************************************************************/
Adafruit_ZeroI2S::Adafruit_ZeroI2S(uint8_t FS_PIN, uint8_t SCK_PIN,
                                   uint8_t TX_PIN, uint8_t RX_PIN)
    : _fs(FS_PIN), _sck(SCK_PIN), _tx(TX_PIN), _rx(RX_PIN) {}

#if defined(PIN_I2S_SDI) && defined(PIN_I2S_SDO)
/**************************************************************************/
/*!
    @brief  Class Constructor with defaults
*/
/**************************************************************************/
Adafruit_ZeroI2S::Adafruit_ZeroI2S()
    : _fs(PIN_I2S_FS), _sck(PIN_I2S_SCK), _tx(PIN_I2S_SDO), _rx(PIN_I2S_SDI) {}
#else
/**************************************************************************/
/*!
    @brief  Class Constructor with defaults
*/
/**************************************************************************/
Adafruit_ZeroI2S::Adafruit_ZeroI2S()
    : _fs(PIN_I2S_FS), _sck(PIN_I2S_SCK), _tx(PIN_I2S_SD) {
  _rx = -1;
}
#endif

/**************************************************************************/
/*!
    @brief  start up the I2S peripheral
        @param width the width of each I2S frame
        @param fs_freq the frame sync frequency (a.k.a. sample rate)
        @param mck_mult master clock output will be fs_freq * mck_mult for chips
   that have a mclk. This should be a multiple of the width.
        @returns true on success, false on any error
*/
/**************************************************************************/
bool Adafruit_ZeroI2S::begin(I2SSlotSize width, int fs_freq, int mck_mult) {
#if defined(__SAMD51__)

  pinPeripheral(_fs, PIO_I2S);
  pinPeripheral(_sck, PIO_I2S);
  if (_rx != -1)
    pinPeripheral(_rx, PIO_I2S);
  pinPeripheral(_tx, PIO_I2S);

  I2S->CTRLA.bit.ENABLE = 0;

  // initialize clock control
  MCLK->APBDMASK.reg |= MCLK_APBDMASK_I2S;

  // This is the size of an I2S frame: one sample at the given bit
  // width for every I2S slot (in stereo, the "slots" are the left and
  // right channels).
  uint32_t frameSize = I2S_NUM_SLOTS * ((width + 1) << 3);

  // We need to set up two I2S clocks, MCK and SCK, using the SAMD51's
  // available GCLKs and the two clock dividers it provides for this
  // purpose, mckoutdiv and mckdiv (somewhat unhelpfully named).

  // This is the correct frequency we'd ideally like to run MCK at:
  // mck_mult ticks per sample, fs_freq samples per second.
  uint32_t nominalMckFreq = (fs_freq * mck_mult);

  // And we'd ideally like to run SCK at frameSize ticks per sample,
  // fs_freq samples per second.

  // But without an external clock, we'll have to make do with the
  // GCLKs provided by the SAMD51. We can set a divider, mckoutdiv, to
  // get as close as possible to nominalMckFreq as we can using an int
  // divider.
  uint32_t gclkval = GCLK_PCHCTRL_GEN_GCLK1_Val;
  uint32_t gclkFreq = VARIANT_GCLK1_FREQ;
  uint32_t mckoutdiv =
      max((gclkFreq + (nominalMckFreq / 2)) / nominalMckFreq, 1);
  if (mckoutdiv > 64) {
    // 64 is the max, so we'll have to start from a slower GCLK.
    gclkval = GCLK_PCHCTRL_GEN_GCLK4_Val;
    gclkFreq = 12000000;
    mckoutdiv = min((gclkFreq + (nominalMckFreq / 2)) / nominalMckFreq, 64);
  }

  // mckoutdiv divides the GCLK to get our real MCK frequency
  // uint32_t realMckFreq = gclkFreq / mckoutdiv;

  // Note that because our real clock rates are only an approximation
  // of the nominal rate, our real sample rate is only an
  // approximation of fs_freq. Here's how you would calculate the
  // actual rate:
  // float realFsFreq = static_cast<float>(realMckFreq) / mck_mult;

  // mckdiv also divides the GCLK, to get our real SCK frequency. To
  // work well, it needs to divide evenly into the MCK frequency. This
  // is only possible if mck_mult (MCK's ticks per sample) is
  // divisible by frameSize (SCK's ticks per sample), so users should
  // choose mck_mult and width accordingly.

  // Instead of dividing GCLK into mck_mult ticks per sample as
  // mckoutdiv does, mckdiv divides it into frameSize ticks per
  // sample. This is equivalent to directly dividing gclkFreq by
  // (realFsFreq * frameSize), but avoids using the (possibly
  // non-integer) realFsFreq.
  uint32_t mckdiv = (mckoutdiv * mck_mult) / frameSize;

  GCLK->PCHCTRL[I2S_GCLK_ID_0].reg = gclkval | (1 << GCLK_PCHCTRL_CHEN_Pos);
  GCLK->PCHCTRL[I2S_GCLK_ID_1].reg = gclkval | (1 << GCLK_PCHCTRL_CHEN_Pos);

  // software reset
  I2S->CTRLA.bit.SWRST = 1;
  while (I2S->SYNCBUSY.bit.SWRST || I2S->SYNCBUSY.bit.ENABLE)
    ; // wait for sync

  // CLKCTRL[0] is used for the tx channel
  I2S->CLKCTRL[0].reg =
      I2S_CLKCTRL_MCKSEL_GCLK | I2S_CLKCTRL_MCKOUTDIV(mckoutdiv - 1) |
      I2S_CLKCTRL_MCKDIV(mckdiv - 1) | I2S_CLKCTRL_SCKSEL_MCKDIV |
      I2S_CLKCTRL_MCKEN | I2S_CLKCTRL_FSSEL_SCKDIV | I2S_CLKCTRL_BITDELAY_I2S |
      I2S_CLKCTRL_FSWIDTH_HALF | I2S_CLKCTRL_FSOUTINV |
      I2S_CLKCTRL_NBSLOTS(I2S_NUM_SLOTS - 1) | I2S_CLKCTRL_SLOTSIZE(width);

  uint8_t wordSize;

  switch (width) {
  case I2S_8_BIT:
    wordSize = I2S_TXCTRL_DATASIZE_8_Val;
    break;
  case I2S_16_BIT:
    wordSize = I2S_TXCTRL_DATASIZE_16_Val;
    break;
  case I2S_24_BIT:
    wordSize = I2S_TXCTRL_DATASIZE_24_Val;
    break;
  case I2S_32_BIT:
    wordSize = I2S_TXCTRL_DATASIZE_32_Val;
    break;
  }

  I2S->TXCTRL.reg = I2S_TXCTRL_DMA_SINGLE | I2S_TXCTRL_MONO_STEREO |
                    I2S_TXCTRL_BITREV_MSBIT | I2S_TXCTRL_EXTEND_ZERO |
                    I2S_TXCTRL_WORDADJ_RIGHT | I2S_TXCTRL_DATASIZE(wordSize) |
                    I2S_TXCTRL_TXSAME_ZERO | I2S_TXCTRL_TXDEFAULT_ZERO;

  I2S->RXCTRL.reg = I2S_RXCTRL_DMA_SINGLE | I2S_RXCTRL_MONO_STEREO |
                    I2S_RXCTRL_BITREV_MSBIT | I2S_RXCTRL_EXTEND_ZERO |
                    I2S_RXCTRL_WORDADJ_RIGHT | I2S_RXCTRL_DATASIZE(wordSize) |
                    I2S_RXCTRL_SLOTADJ_RIGHT | I2S_RXCTRL_CLKSEL_CLK0 |
                    I2S_RXCTRL_SERMODE_RX;

  while (I2S->SYNCBUSY.bit.ENABLE)
    ; // wait for sync
  I2S->CTRLA.bit.ENABLE = 1;

  return true;

#else // SAMD21
  _i2sserializer = -1;
  _i2sclock = -1;
  uint32_t _clk_pin, _clk_mux, _data_pin, _data_mux, _fs_pin, _fs_mux;

  // Clock pin, can only be one of 3 options
  uint32_t clockport = g_APinDescription[_sck].ulPort;
  uint32_t clockpin = g_APinDescription[_sck].ulPin;
  if ((clockport == 0) && (clockpin == 10)) {
    // PA10
    _i2sclock = 0;
    _clk_pin = PIN_PA10G_I2S_SCK0;
    _clk_mux = MUX_PA10G_I2S_SCK0;
#if defined(PIN_PB11G_I2S_SCK1)
  } else if ((clockport == 1) && (clockpin == 11)) {
    // PB11
    _i2sclock = 1;
    _clk_pin = PIN_PB11G_I2S_SCK1;
    _clk_mux = MUX_PB11G_I2S_SCK1;
#endif
#if defined(PIN_PA20G_I2S_SCK0)
  } else if ((clockport == 0) && (clockpin == 20)) {
    // PA20
    _i2sclock = 0;
    _clk_pin = PIN_PA20G_I2S_SCK0;
    _clk_mux = MUX_PA20G_I2S_SCK0;
#endif
  } else {
    DEBUG_PRINTLN("Clock isnt on a valid pin");
    return false;
  }
  pinPeripheral(_sck, (EPioType)_clk_mux);

  // FS pin, can only be one of 2 options
  uint32_t fsport = g_APinDescription[_fs].ulPort;
  uint32_t fspin = g_APinDescription[_fs].ulPin;
  if ((fsport == 0) && (fspin == 11)) {
    // PA11
    _fs_pin = PIN_PA11G_I2S_FS0;
    _fs_mux = MUX_PA11G_I2S_FS0;
#if defined(PIN_PA21G_I2S_FS0)
  } else if ((fsport == 0) && (fspin == 21)) {
    // PA21
    _fs_pin = PIN_PA21G_I2S_FS0;
    _fs_mux = MUX_PA21G_I2S_FS0;
#endif
  } else {
    DEBUG_PRINTLN("FS isnt on a valid pin");
    return false;
  }
  pinPeripheral(_fs, (EPioType)_fs_mux);

  uint32_t i2sGCLK;
  if (_i2sclock == 0)
    i2sGCLK = I2S_GCLK_ID_0;
  else
    i2sGCLK = I2S_GCLK_ID_1;

  uint32_t divider = fs_freq * 2 * (width + 1) * 8;
  // configure the clock divider
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;
  GCLK->GENDIV.bit.ID = I2S_CLOCK_GENERATOR;
  GCLK->GENDIV.bit.DIV = SystemCoreClock / divider;

  // use the DFLL as the source
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;
  GCLK->GENCTRL.bit.ID = I2S_CLOCK_GENERATOR;
  GCLK->GENCTRL.bit.SRC = GCLK_GENCTRL_SRC_DFLL48M_Val;
  GCLK->GENCTRL.bit.IDC = 1;
  GCLK->GENCTRL.bit.GENEN = 1;

  // enable
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;
  GCLK->CLKCTRL.bit.ID = i2sGCLK;
  GCLK->CLKCTRL.bit.GEN = I2S_CLOCK_GENERATOR;
  GCLK->CLKCTRL.bit.CLKEN = 1;

  while (GCLK->STATUS.bit.SYNCBUSY)
    ;

  // Data pin, can only be one of 3 options
  uint32_t datapin = g_APinDescription[_tx].ulPin;
  uint32_t dataport = g_APinDescription[_tx].ulPort;
  if ((dataport == 0) && (datapin == 7)) {
    // PA07
    _i2sserializer = 0;
    _data_pin = PIN_PA07G_I2S_SD0;
    _data_mux = MUX_PA07G_I2S_SD0;
  } else if ((dataport == 0) && (datapin == 8)) {
    // PA08
    _i2sserializer = 1;
    _data_pin = PIN_PA08G_I2S_SD1;
    _data_mux = MUX_PA08G_I2S_SD1;
  } else if ((dataport == 0) && (datapin == 19)) {
    // PA19
    _i2sserializer = 0;
    _data_pin = PIN_PA19G_I2S_SD0;
    _data_mux = MUX_PA19G_I2S_SD0;
  } else {
    DEBUG_PRINTLN("Data isnt on a valid pin");
    return false;
  }
  pinPeripheral(_tx, (EPioType)_data_mux);

  PM->APBCMASK.reg |= PM_APBCMASK_I2S;

  I2S->CTRLA.bit.ENABLE = 0;
  while (I2S->SYNCBUSY.bit.ENABLE)
    ;

  if (_i2sclock == 0)
    I2S->CTRLA.bit.CKEN0 = 0;
  else
    I2S->CTRLA.bit.CKEN1 = 0;
  while (I2S->SYNCBUSY.bit.CKEN0 || I2S->SYNCBUSY.bit.CKEN1)
    ;

  I2S->CLKCTRL[_i2sclock].reg =
      I2S_CLKCTRL_MCKSEL_GCLK | I2S_CLKCTRL_SCKSEL_MCKDIV |
      I2S_CLKCTRL_FSSEL_SCKDIV | I2S_CLKCTRL_BITDELAY_I2S |
      I2S_CLKCTRL_NBSLOTS(I2S_NUM_SLOTS - 1) | I2S_CLKCTRL_SLOTSIZE(width);

  uint8_t wordSize;
  switch (width) {
  case I2S_8_BIT:
    wordSize = I2S_SERCTRL_DATASIZE_8_Val;
    break;
  case I2S_16_BIT:
    wordSize = I2S_SERCTRL_DATASIZE_16_Val;
    break;
  case I2S_24_BIT:
    wordSize = I2S_SERCTRL_DATASIZE_24_Val;
    break;
  case I2S_32_BIT:
    wordSize = I2S_SERCTRL_DATASIZE_32_Val;
    break;
  default:
    DEBUG_PRINTLN("invalid width!");
    return false;
  }

  if (_i2sserializer == 0)
    I2S->CTRLA.bit.SEREN0 = 0;
  else
    I2S->CTRLA.bit.SEREN1 = 0;
  while (I2S->SYNCBUSY.bit.SEREN0 || I2S->SYNCBUSY.bit.SEREN1)
    ;

  I2S->SERCTRL[_i2sserializer].reg =
      I2S_SERCTRL_DMA_SINGLE | I2S_SERCTRL_MONO_STEREO |
      I2S_SERCTRL_BITREV_MSBIT | I2S_SERCTRL_EXTEND_ZERO |
      I2S_SERCTRL_WORDADJ_RIGHT | I2S_SERCTRL_DATASIZE(wordSize) |
      I2S_SERCTRL_SLOTADJ_RIGHT |
      ((uint32_t)_i2sclock << I2S_SERCTRL_CLKSEL_Pos);

  return true;
#endif
}

/**************************************************************************/
/*!
    @brief  enable data output. Note that on SAMD21 chips either rx or tx can be
   enabled on an Adafruit_ZeroI2S instance, while on SAMD51 the same
   Adafruit_ZeroI2S instance can have both rx and tx channels enabled.
*/
/**************************************************************************/
void Adafruit_ZeroI2S::enableTx() {
#if defined(__SAMD51__)
  I2S->CTRLA.bit.CKEN0 = 1;
  while (I2S->SYNCBUSY.bit.CKEN0)
    ;

  I2S->CTRLA.bit.TXEN = 1;
  while (I2S->SYNCBUSY.bit.TXEN)
    ;
#else
  if (_i2sserializer > -1 && _i2sclock > -1) {
    I2S->CTRLA.bit.ENABLE = 0;
    while (I2S->SYNCBUSY.bit.ENABLE)
      ;

    I2S->SERCTRL[_i2sserializer].bit.SERMODE = I2S_SERCTRL_SERMODE_TX;

    if (_i2sserializer == 0)
      I2S->CTRLA.bit.SEREN0 = 1;
    else
      I2S->CTRLA.bit.SEREN1 = 1;

    if (_i2sclock == 0)
      I2S->CTRLA.bit.CKEN0 = 1;
    else
      I2S->CTRLA.bit.CKEN1 = 1;

    I2S->CTRLA.bit.ENABLE = 1;
    while (I2S->SYNCBUSY.bit.ENABLE || I2S->SYNCBUSY.bit.CKEN0 ||
           I2S->SYNCBUSY.bit.CKEN1 || I2S->SYNCBUSY.bit.SEREN0 ||
           I2S->SYNCBUSY.bit.SEREN1)
      ;
  }
#endif
}

/**************************************************************************/
/*!
    @brief  disable data output
*/
/**************************************************************************/
void Adafruit_ZeroI2S::disableTx() {
#if defined(__SAMD51__)
  I2S->CTRLA.bit.TXEN = 0;
  while (I2S->SYNCBUSY.bit.TXEN)
    ;
#else
#endif
}

/**************************************************************************/
/*!
    @brief  enable data input. Note that on SAMD21 chips either rx or tx can be
   enabled on an Adafruit_ZeroI2S instance, while on SAMD51 the same
   Adafruit_ZeroI2S instance can have both rx and tx channels enabled.
*/
/**************************************************************************/
void Adafruit_ZeroI2S::enableRx() {
#if defined(__SAMD51__)
  I2S->CTRLA.bit.CKEN0 = 1;
  while (I2S->SYNCBUSY.bit.CKEN0)
    ;

  I2S->CTRLA.bit.RXEN = 1;
  while (I2S->SYNCBUSY.bit.RXEN)
    ;
#else
  if (_i2sserializer > -1 && _i2sclock > -1) {
    I2S->CTRLA.bit.ENABLE = 0;
    while (I2S->SYNCBUSY.bit.ENABLE)
      ;

    I2S->SERCTRL[_i2sserializer].bit.SERMODE = I2S_SERCTRL_SERMODE_RX;

    if (_i2sserializer == 0)
      I2S->CTRLA.bit.SEREN0 = 1;
    else
      I2S->CTRLA.bit.SEREN1 = 1;

    if (_i2sclock == 0)
      I2S->CTRLA.bit.CKEN0 = 1;
    else
      I2S->CTRLA.bit.CKEN1 = 1;

    I2S->CTRLA.bit.ENABLE = 1;
    while (I2S->SYNCBUSY.bit.ENABLE || I2S->SYNCBUSY.bit.CKEN0 ||
           I2S->SYNCBUSY.bit.CKEN1 || I2S->SYNCBUSY.bit.SEREN0 ||
           I2S->SYNCBUSY.bit.SEREN1)
      ;
  }
#endif
}

/**************************************************************************/
/*!
    @brief  disable data input
*/
/**************************************************************************/
void Adafruit_ZeroI2S::disableRx() {
#if defined(__SAMD51__)
  I2S->CTRLA.bit.RXEN = 0;
  while (I2S->SYNCBUSY.bit.RXEN)
    ;
#else
#endif
}

/**************************************************************************/
/*!
    @brief  enable master clock output on devices that have a master clock
   output.
*/
/**************************************************************************/
void Adafruit_ZeroI2S::enableMCLK() {
#ifdef PIN_I2S_MCK
  pinPeripheral(PIN_I2S_MCK, PIO_I2S);
#endif
}

/**************************************************************************/
/*!
    @brief  disable master clock output on devices that have a master clock
   output.
*/
/**************************************************************************/
void Adafruit_ZeroI2S::disableMCLK() {
#ifdef PIN_I2S_MCK
  pinMode(PIN_I2S_MCK, INPUT);
#endif
}

/**************************************************************************/
/*!
    @brief  check if data can be written to the TX data register
        @returns true if data can be written, false otherwise
*/
/**************************************************************************/
bool Adafruit_ZeroI2S::txReady() {
#if defined(__SAMD51__)
  return !((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.TXDATA);
#else
  if (_i2sserializer > -1) {
    if (_i2sserializer == 0) {
      return !((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.DATA0);
    } else {
      return !((!I2S->INTFLAG.bit.TXRDY1) || I2S->SYNCBUSY.bit.DATA1);
    }
  } else
    return false;
#endif
}

/**************************************************************************/
/*!
    @brief  check if data is available to be read from the RX data register
        @returns true if data is available, false otherwise
*/
/**************************************************************************/
bool Adafruit_ZeroI2S::rxReady() {
#if defined(__SAMD51__)
  return !((!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.RXDATA);
#else
  if (_i2sserializer > -1) {
    if (_i2sserializer == 0) {
      return !((!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.DATA0);
    } else {
      return !((!I2S->INTFLAG.bit.RXRDY1) || I2S->SYNCBUSY.bit.DATA1);
    }
  } else
    return false;
#endif
}

/**************************************************************************/
/*!
    @brief perform a blocking write to the I2S peripheral. This function will
   only return once all data has been sent.
        @param left the left channel data
        @param right the right channel data
*/
/**************************************************************************/
void Adafruit_ZeroI2S::write(int32_t left, int32_t right) {
#if defined(__SAMD51__)
  while ((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.TXDATA)
    ;
  I2S->INTFLAG.bit.TXUR0 = 1;
  I2S->TXDATA.reg = left;

  while ((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.TXDATA)
    ;
  I2S->INTFLAG.bit.TXUR0 = 1;
  I2S->TXDATA.reg = right;
#else
  if (_i2sserializer > -1) {
    if (_i2sserializer == 0) {
      while ((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.DATA0)
        ;
      I2S->INTFLAG.bit.TXUR0 = 1;
      I2S->DATA[0].reg = left;

      while ((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.DATA0)
        ;
      I2S->INTFLAG.bit.TXUR0 = 1;
      I2S->DATA[0].reg = right;
    } else {
      while ((!I2S->INTFLAG.bit.TXRDY1) || I2S->SYNCBUSY.bit.DATA1)
        ;
      I2S->INTFLAG.bit.TXUR1 = 1;
      I2S->DATA[1].reg = left;

      while ((!I2S->INTFLAG.bit.TXRDY1) || I2S->SYNCBUSY.bit.DATA1)
        ;
      I2S->INTFLAG.bit.TXUR1 = 1;
      I2S->DATA[1].reg = right;
    }
  }
#endif
}

/**************************************************************************/
/*!
    @brief perform a blocking read to the I2S peripheral. This function will
   only return once all data has been read.
        @param left pointer to where the left channel data will be written
        @param right pointer to where the right channel data will be written
*/
/**************************************************************************/
void Adafruit_ZeroI2S::read(int32_t *left, int32_t *right) {
#if defined(__SAMD51__)
  while ((!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.RXDATA)
    ;
  *left = I2S->RXDATA.reg;

  while ((!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.RXDATA)
    ;
  *right = I2S->RXDATA.reg;
#else
  if (_i2sserializer > -1) {
    if (_i2sserializer == 0) {
      while ((!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.DATA0)
        ;
      *left = I2S->DATA[0].reg;

      while ((!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.DATA0)
        ;
      *right = I2S->DATA[0].reg;
    } else {
      while ((!I2S->INTFLAG.bit.RXRDY1) || I2S->SYNCBUSY.bit.DATA1)
        ;
      *left = I2S->DATA[1].reg;

      while ((!I2S->INTFLAG.bit.RXRDY1) || I2S->SYNCBUSY.bit.DATA1)
        ;
      *right = I2S->DATA[1].reg;
    }
  }
#endif
}
