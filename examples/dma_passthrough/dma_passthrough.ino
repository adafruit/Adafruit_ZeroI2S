/* This example shows how to pass data through using the
 *  ZeroI2S and ZeroDMA libraries.
 *  
 *  This example is for M4 devices only
 * 
 *  This uses a 24bit i2s slave device capable of both transmitting
 *  receiving data.
 *  
 *  Any data that the microcontroller receives via the I2S bus
 *  will be DMAd back out to the device.
 *  
 *  try this with the AK4556 I2S ADC/DAC
 *  https://www.akm.com/akm/en/file/datasheet/AK4556VT.pdf
 */

#include <Adafruit_ZeroI2S.h>
#include <Adafruit_ZeroDMA.h>
#include "utility/dma.h"
#include <math.h>

#ifndef __SAMD51__
#error "this example is for SAMD51 devices only"
#endif

/* max volume for 24 bit data */
#define VOLUME ( (1UL << 23) - 1)

/* create 2 buffers. One will transmit while the other gets filled */
#define BUFSIZE 128
int ping[BUFSIZE];
int pong[BUFSIZE];

//the buffer that's currently transmitting
int *txBuf;

/* we need one DMA channel for receive (RX), 
* and another for transmit (TX) 
*/
Adafruit_ZeroDMA txDMA;
Adafruit_ZeroDMA rxDMA;

DmacDescriptor *txDesc;
DmacDescriptor *rxDesc;

ZeroDMAstatus    stat; // DMA status codes returned by some functions

Adafruit_ZeroI2S i2s;

void dummy_callback(Adafruit_ZeroDMA *dma) { } //do nothing

void setup()
{
  Serial.begin(115200);
  //while(!Serial);                 // Wait for Serial monitor before continuing

  Serial.println("I2S throughput via DMA");

  Serial.println("Configuring DMA triggers");
  txDMA.setTrigger(I2S_DMAC_ID_TX_0);
  rxDMA.setTrigger(I2S_DMAC_ID_RX_0);
  txDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  rxDMA.setAction(DMA_TRIGGER_ACTON_BEAT);

  Serial.print("Allocating DMA channels...");
  stat = txDMA.allocate();
  txDMA.printStatus(stat);

  stat = rxDMA.allocate();
  rxDMA.printStatus(stat);

  Serial.println("Setting up transfer");
  txDesc = txDMA.addDescriptor(
    ping,                       // move data from here
    (void *)(&I2S->TXDATA.reg),   // to here
    BUFSIZE,                      // this many...
    DMA_BEAT_SIZE_WORD,           // bytes/hword/words
    true,                         // increment source addr?
    false);
  txDesc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  txDesc = txDMA.addDescriptor(
    pong,                       // move data from here
    (void *)(&I2S->TXDATA.reg),   // to here
    BUFSIZE,                      // this many...
    DMA_BEAT_SIZE_WORD,           // bytes/hword/words
    true,                         // increment source addr?
    false);
  txDesc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
  txDMA.loop(true);

  //this will be the initial tx buffer
  txBuf = ping;

  rxDesc = rxDMA.addDescriptor(
    (void *)(&I2S->RXDATA.reg),   // move data from here
    pong,               // to here
    BUFSIZE,                      // this many...
    DMA_BEAT_SIZE_WORD,           // bytes/hword/words
    false,                        // increment source addr?
    true);
  rxDesc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
  
  rxDesc = rxDMA.addDescriptor(
    (void *)(&I2S->RXDATA.reg),   // move data from here
    ping,               // to here
    BUFSIZE,                      // this many...
    DMA_BEAT_SIZE_WORD,           // bytes/hword/words
    false,                        // increment source addr?
    true);
  rxDesc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
  rxDMA.loop(true);

  Serial.println("Adding callbacks");
  txDMA.setCallback(dummy_callback);
  rxDMA.setCallback(dummy_callback);

  /* begin I2S on the default pins. 32 bit depth at
   * 44100 samples per second
   */
  i2s.begin(I2S_32_BIT, 44100);

  /* uncomment this if your I2S device uses the MCLK line */
  i2s.enableMCLK();

  /* enable transmit and receive channels */
  i2s.enableTx();
  i2s.enableRx();

  stat = rxDMA.startJob();
  stat = txDMA.startJob();
}

void loop()
{
  Serial.println("do other things here while your DMA runs in the background.");
  delay(2000);
}
