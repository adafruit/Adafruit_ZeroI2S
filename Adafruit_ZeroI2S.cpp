#include "Adafruit_ZeroI2S.h"
#include "wiring_private.h"

Adafruit_ZeroI2S::Adafruit_ZeroI2S(uint8_t FS_PIN, uint8_t SCK_PIN, uint8_t TX_PIN, uint8_t RX_PIN) : _fs(FS_PIN), _sck(SCK_PIN), _tx(TX_PIN), _rx(RX_PIN) {}

Adafruit_ZeroI2S::Adafruit_ZeroI2S() : _fs(PIN_I2S_FS), _sck(PIN_I2S_SCK), _tx(PIN_I2S_SDO), _rx(PIN_I2S_SDI) {}

bool Adafruit_ZeroI2S::begin(I2SSlotSize width, int fs_freq, int mck_mult)
{
	pinPeripheral(_fs, PIO_I2S);
	pinPeripheral(_sck, PIO_I2S);
	pinPeripheral(_rx, PIO_I2S);
	pinPeripheral(_tx, PIO_I2S);

	I2S->CTRLA.bit.ENABLE = 0;

	//initialize clock control
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_I2S;

	GCLK->PCHCTRL[I2S_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK2_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);
	GCLK->PCHCTRL[I2S_GCLK_ID_1].reg = GCLK_PCHCTRL_GEN_GCLK2_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

	//software reset
	I2S->CTRLA.bit.SWRST = 1;
	while(I2S->SYNCBUSY.bit.SWRST || I2S->SYNCBUSY.bit.ENABLE); //wait for sync

	uint32_t mckFreq = (fs_freq * mck_mult);
	uint32_t sckFreq = fs_freq * I2S_NUM_SLOTS * ( (width + 1) << 3);

	//CLKCTRL[0] is used for the tx channel
	I2S->CLKCTRL[0].reg = I2S_CLKCTRL_MCKSEL_GCLK |
			I2S_CLKCTRL_MCKOUTDIV( (VARIANT_GCLK2_FREQ/mckFreq) - 1) |
			I2S_CLKCTRL_MCKDIV((VARIANT_GCLK2_FREQ/sckFreq) - 1) |
			I2S_CLKCTRL_SCKSEL_MCKDIV |
			I2S_CLKCTRL_MCKEN |
			I2S_CLKCTRL_FSSEL_SCKDIV |
			I2S_CLKCTRL_BITDELAY_I2S |
			I2S_CLKCTRL_FSWIDTH_HALF |
			I2S_CLKCTRL_NBSLOTS(I2S_NUM_SLOTS - 1) |
			I2S_CLKCTRL_SLOTSIZE(width);

	uint8_t wordSize;

	switch(width){
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

	I2S->TXCTRL.reg = I2S_TXCTRL_DMA_SINGLE |
			I2S_TXCTRL_MONO_STEREO |
			I2S_TXCTRL_BITREV_MSBIT |
			I2S_TXCTRL_EXTEND_ZERO |
			I2S_TXCTRL_WORDADJ_RIGHT |
			I2S_TXCTRL_DATASIZE(wordSize) |
			I2S_TXCTRL_TXSAME_ZERO |
			I2S_TXCTRL_TXDEFAULT_ZERO;

	while(I2S->SYNCBUSY.bit.ENABLE); //wait for sync
	I2S->CTRLA.bit.ENABLE = 1;

	I2S->CTRLA.bit.CKEN0 = 1;
	while(I2S->SYNCBUSY.bit.CKEN0);

	I2S->CTRLA.bit.TXEN = 1;
	while(I2S->SYNCBUSY.bit.TXEN);

	return true;
}

void Adafruit_ZeroI2S::enableMCLK()
{
	pinPeripheral(PIN_I2S_MCK, PIO_I2S);
}

void Adafruit_ZeroI2S::disableMCLK()
{
	pinMode(PIN_I2S_MCK, INPUT);
}

bool Adafruit_ZeroI2S::txReady()
{
	return ((!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.TXDATA );
}

void Adafruit_ZeroI2S::write(int32_t left, int32_t right)
{
	while( (!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.TXDATA );
	I2S->INTFLAG.bit.TXUR0 = 1;
	I2S->TXDATA.reg = left;

	while( (!I2S->INTFLAG.bit.TXRDY0) || I2S->SYNCBUSY.bit.TXDATA );
	I2S->INTFLAG.bit.TXUR0 = 1;
	I2S->TXDATA.reg = right;
}
