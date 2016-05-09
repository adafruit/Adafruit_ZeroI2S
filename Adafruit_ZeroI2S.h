// Adafruit Arduino Zero / Feather M0 I2S audio library.
// Author: Tony DiCola
//
// The MIT License (MIT)
//
// Copyright (c) 2016 Adafruit Industries
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef ADAFRUIT_ZEROI2S_H
#define ADAFRUIT_ZEROI2S_H

#include <Arduino.h>
#include <sam.h>
#include <i2s.h>


// By default set SD pin to digital pin 9 (PA07).  You can alternately set this
// to digital pin 12 (PA19) by adjusting the I2S_SD_PIN and I2S_SD_MUX values.
#define I2S_SD_PIN PIN_PA07G_I2S_SD0
#define I2S_SD_MUX MUX_PA07G_I2S_SD0

// Uncomment to enable debug message output.
//#define DEBUG

// Define where debug output is printed (the native USB port on the Zero).
#define DEBUG_PRINTER Serial


class Adafruit_ZeroI2S_TX {
public:
  // Create a new instance of an I2S audio transmitter.
  // Can specify the SAMD21 I2S serializer and clock instance to use (either 0
  // or 1, default is 0), and the generic clock ID to use for driving the I2S
  // hardware (default is GCLK 3).
  Adafruit_ZeroI2S_TX(uint8_t id = 0, gclk_generator gclk = GCLK_GENERATOR_3):
    _id(id),
    _gclk(gclk)
  {}

  // Initialize the I2S audio transmitter.
  bool begin();

  // Configure the transmitter with the specified number of channels, sample rate
  // (in hertz), and number of bits per sample (one of 8, 16, 24, 32).
  bool configure(uint8_t numChannels, uint32_t sampleRateHz, uint8_t bitsPerSample);

  // Write a single sample to the I2S transmitter.  Will wait until the I2S
  // hardware is ready to receive the sample.
  void write(uint32_t data);

  // Return true if the transmitter is in an underflow state (i.e. data isn't
  // sent to it fast enough).
  bool isUnderflow() {
    return i2s_get_status(&_i2s_instance) & I2S_STATUS_TRANSMIT_UNDERRUN(_id);
  }

  // Clear any underflow flag.
  void clearUnderflow() {
    i2s_clear_status(&_i2s_instance, I2S_STATUS_TRANSMIT_UNDERRUN(_id));
  }

private:
  uint8_t _id;
  gclk_generator _gclk;
  struct i2s_module _i2s_instance;
};

#endif
