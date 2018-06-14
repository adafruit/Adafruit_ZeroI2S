# Adafruit Zero I2S Library

I2S audio playback library for the Arduino Zero / Adafruit M0 (SAMD21 processor) -and- Adafruit M4 (SAMD51 processor) boards


Supports:
-   DMA / interrupt support.  Uses the Adafruit ZeroDMA library to set up DMA transfers, see examples!
-   Both Transmit (audio/speaker output) & Receive (audio/mic input) support.

TODO:
-   MCLK output.  Only supports output for BCLK, LRCLK, and data.
