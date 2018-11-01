#include <Adafruit_ZeroI2S.h>
#include <Adafruit_ZeroDMA.h>
#include "utility/dma.h"
#include <math.h>

/* max volume for 32 bit data */
#define VOLUME ( (1UL << 31) - 1)

/* create a buffer for both the left and right channel data */
#define BUFSIZE 256
int data[BUFSIZE];

Adafruit_ZeroDMA myDMA;
ZeroDMAstatus    stat; // DMA status codes returned by some functions

Adafruit_ZeroI2S i2s;

void dma_callback(Adafruit_ZeroDMA *dma) {
  /* we don't need to do anything here */
}

void setup()
{
  Serial.begin(115200);
  //while(!Serial);                 // Wait for Serial monitor before continuing

  Serial.println("I2S output via DMA");

  int *ptr = data;

  /*the I2S module will be expecting data interleaved LRLR*/
  for(int i=0; i<BUFSIZE/2; i++){
      /* create a sine wave on the left channel */
        *ptr++ = sin( (2*PI / (BUFSIZE/2) ) * i) * VOLUME;

        /* create a cosine wave on the right channel */
        *ptr++ = cos( (2*PI / (BUFSIZE/2) ) * i) * VOLUME;
  }

  Serial.println("Configuring DMA trigger");
  myDMA.setTrigger(I2S_DMAC_ID_TX_0);
  myDMA.setAction(DMA_TRIGGER_ACTON_BEAT);

  Serial.print("Allocating DMA channel...");
  stat = myDMA.allocate();
  myDMA.printStatus(stat);

  Serial.println("Setting up transfer");
    myDMA.addDescriptor(
      data,                    // move data from here
#if defined(__SAMD51__)
      (void *)(&I2S->TXDATA.reg), // to here (M4)
#else
      (void *)(&I2S->DATA[0].reg), // to here (M0+)
#endif
    BUFSIZE,                      // this many...
      DMA_BEAT_SIZE_WORD,               // bytes/hword/words
      true,                             // increment source addr?
      false);
  myDMA.loop(true);
  Serial.println("Adding callback");
  myDMA.setCallback(dma_callback);

  /* begin I2S on the default pins. 24 bit depth at
   * 44100 samples per second
   */
  i2s.begin(I2S_32_BIT, 44100);
  i2s.enableTx();

  stat = myDMA.startJob();
}

void loop()
{
  Serial.println("do other things here while your DMA runs in the background.");
  delay(2000);
}
