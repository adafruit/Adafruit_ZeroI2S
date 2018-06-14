#include <Arduino.h>

#include <Adafruit_ZeroI2S.h>
#include <math.h>

/* max volume for 24 bit data */
#define VOLUME ( (1UL << 23) - 1)

/* create a buffer for both the left and right channel data */
#define BUFSIZE 128
int left[BUFSIZE];
int right[BUFSIZE];

Adafruit_ZeroI2S i2s;

void setup()
{
  for(int i=0; i<BUFSIZE; i++){
      /* create a sine wave on the left channel */
        left[i] = sin( (2*PI / (BUFSIZE) ) * i) * VOLUME;

        /* create a cosine wave on the right channel */
        right[i] = cos( (2*PI / (BUFSIZE) ) * i) * VOLUME;
  }

  /* begin I2S on the default pins. 24 bit depth at
   * 44100 samples per second
   */
  i2s.begin(I2S_24_BIT, 44100);
  i2s.enableTx();
}

void loop()
{
  /* write the output buffers
   * note that i2s.write() will block until both channels are written.
   */
  for(int i=0; i<BUFSIZE; i++){
    i2s.write(left[i], right[i]);
  }
}