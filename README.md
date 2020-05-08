# Adafruit ZeroI2S [![Build Status](https://github.com/adafruit/Adafruit_ZeroI2S/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/Adafruit_ZeroI2S/actions)[![Documentation](https://github.com/adafruit/ci-arduino/blob/master/assets/doxygen_badge.svg)](http://adafruit.github.io/Adafruit_ZeroI2S/html/index.html)

I2S audio playback library for the Arduino Zero / Adafruit M0 (SAMD21 processor) -and- Adafruit M4 (SAMD51 processor) boards


Supports:
-   DMA / interrupt support.  Uses the Adafruit ZeroDMA library to set up DMA transfers, see examples!
-   Both Transmit (audio/speaker output) & Receive (audio/mic input) support.

TODO:
-   MCLK output.  Only supports output for BCLK, LRCLK, and data.
# Installation
To install, use the Arduino Library Manager and search for "Adafruit ZeroI2S" and install the library.
## Dependencies
 * [Adafruit ZeroDMA](https://github.com/adafruit/Adafruit_ZeroDMA)

# Contributing

Contributions are welcome! Please read our [Code of Conduct](https://github.com/adafruit/Adafruit_ZeroI2S/blob/master/CODE_OF_CONDUCT.md>)
before contributing to help this project stay welcoming.

## Documentation and doxygen
Documentation is produced by doxygen. Contributions should include documentation for any new code added.

Some examples of how to use doxygen can be found in these guide pages:

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen-tips

## Formatting and clang-format
This library uses [`clang-format`](https://releases.llvm.org/download.html) to standardize the formatting of `.cpp` and `.h` files. 
Contributions should be formatted using `clang-format`:

The `-i` flag will make the changes to the file.
```bash
clang-format -i *.cpp *.h
```
If you prefer to make the changes yourself, running `clang-format` without the `-i` flag will print out a formatted version of the file. You can save this to a file and diff it against the original to see the changes.

Note that the formatting output by `clang-format` is what the automated formatting checker will expect. Any diffs from this formatting will result in a failed build until they are addressed. Using the `-i` flag is highly recommended.

### clang-format resources
  * [Binary builds and source available on the LLVM downloads page](https://releases.llvm.org/download.html)
  * [Documentation and IDE integration](https://clang.llvm.org/docs/ClangFormat.html)

## About this Driver
Written by Dean Miller for Adafruit Industries.
MIT license, check license.txt for more information
All text above must be included in any redistribution
