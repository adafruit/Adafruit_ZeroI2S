# Adafruit Zero I2S Library

I2S audio playback library for the Arduino Zero / Adafruit Feather M0 (SAMD21 processor).

This library depends on the Adafruit ASF Core library from: https://github.com/adafruit/Adafruit_ASFcore

TODO:
-   DMA / interrupt support.  Only supports syncronous output for now.
-   Receive support.  Only supports audio transmit.
-   MCLK output.  Only supports output for BCLK, LRCLK, and data.
