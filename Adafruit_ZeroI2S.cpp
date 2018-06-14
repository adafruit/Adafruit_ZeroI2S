#include "Adafruit_ZeroI2S.h"
#include "wiring_private.h"

Adafruit_ZeroI2S::Adafruit_ZeroI2S(uint8_t FS_PIN, uint8_t SCK_PIN, uint8_t TX_PIN, uint8_t RX_PIN) : _fs(FS_PIN), _sck(SCK_PIN), _tx(TX_PIN), _rx(RX_PIN) {}

Adafruit_ZeroI2S::Adafruit_ZeroI2S() : _fs(PIN_I2S_FS), _sck(PIN_I2S_SCK), _tx(PIN_I2S_SDO), _rx(PIN_I2S_SDI) {}

bool Adafruit_ZeroI2S::begin(I2SSlotSize width, int fs_freq, int mck_mult)
{
#if defined(__SAMD51__)

	pinPeripheral(_fs, PIO_I2S);
	pinPeripheral(_sck, PIO_I2S);
	pinPeripheral(_rx, PIO_I2S);
	pinPeripheral(_tx, PIO_I2S);

	I2S->CTRLA.bit.ENABLE = 0;

	//initialize clock control
	MCLK->APBDMASK.reg |= MCLK_APBDMASK_I2S;

	GCLK->PCHCTRL[I2S_GCLK_ID_0].reg = GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);
	GCLK->PCHCTRL[I2S_GCLK_ID_1].reg = GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

	//software reset
	I2S->CTRLA.bit.SWRST = 1;
	while(I2S->SYNCBUSY.bit.SWRST || I2S->SYNCBUSY.bit.ENABLE); //wait for sync

	uint32_t mckFreq = (fs_freq * mck_mult);
	uint32_t sckFreq = fs_freq * I2S_NUM_SLOTS * ( (width + 1) << 3);

	//CLKCTRL[0] is used for the tx channel
	I2S->CLKCTRL[0].reg = I2S_CLKCTRL_MCKSEL_GCLK |
			I2S_CLKCTRL_MCKOUTDIV( (VARIANT_GCLK1_FREQ/mckFreq) - 1) |
			I2S_CLKCTRL_MCKDIV((VARIANT_GCLK1_FREQ/sckFreq) - 1) |
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

	I2S->RXCTRL.reg = I2S_RXCTRL_DMA_SINGLE |
			I2S_RXCTRL_MONO_STEREO |
			I2S_RXCTRL_BITREV_MSBIT |
			I2S_RXCTRL_EXTEND_ZERO |
			I2S_RXCTRL_WORDADJ_RIGHT |
			I2S_RXCTRL_DATASIZE(wordSize) |
			I2S_RXCTRL_SLOTADJ_RIGHT |
			I2S_RXCTRL_CLKSEL_CLK0 |
			I2S_RXCTRL_SERMODE_RX;

	while(I2S->SYNCBUSY.bit.ENABLE); //wait for sync
	I2S->CTRLA.bit.ENABLE = 1;

	return true;

#else //SAMD21
	uint32_t _clk_pin, _clk_mux, _data_pin, _data_mux;

	// Clock pin, can only be one of 3 options
	uint32_t clockport = g_APinDescription[_sck].ulPort;
	uint32_t clockpin = g_APinDescription[_sck].ulPin;
	if ((clockport == 0) && (clockpin == 10)) {
		// PA10
		_i2sclock = I2S_CLOCK_UNIT_0;
		_clk_pin = PIN_PA10G_I2S_SCK0;
		_clk_mux = MUX_PA10G_I2S_SCK0;
	} else if ((clockport == 1) && (clockpin == 10)) {
		// PB11
		_i2sclock = I2S_CLOCK_UNIT_1;
		_clk_pin = PIN_PB11G_I2S_SCK1;
		_clk_mux = MUX_PB11G_I2S_SCK1;
	} else if ((clockport == 0) && (clockpin == 20)) {
		// PA20
		_i2sclock = I2S_CLOCK_UNIT_0;
		_clk_pin = PIN_PA20G_I2S_SCK0;
		_clk_mux = MUX_PA20G_I2S_SCK0;
	} else {
		DEBUG_PRINTLN("Clock isnt on a valid pin");
		return false;
	}
	pinPeripheral(_sck, (EPioType)_clk_mux);


	// Data pin, can only be one of 3 options
	uint32_t datapin = g_APinDescription[_rx].ulPin;
	uint32_t dataport = g_APinDescription[_rx].ulPort;
	if ((dataport == 0) && (datapin == 7)) {
		// PA07
		_i2sserializer = I2S_SERIALIZER_0;
		_data_pin = PIN_PA07G_I2S_SD0;
		_data_mux = MUX_PA07G_I2S_SD0;
	} else if ((dataport == 0) && (datapin == 8)) {
		// PA08
		_i2sserializer = I2S_SERIALIZER_1;
		_data_pin = PIN_PA08G_I2S_SD1;
		_data_mux = MUX_PA08G_I2S_SD1;
	} else if ((dataport == 0) && (datapin == 19)) {
		// PA19
		_i2sserializer = I2S_SERIALIZER_0;
		_data_pin = PIN_PA19G_I2S_SD0;
		_data_mux = MUX_PA19G_I2S_SD0;
	} else {
		DEBUG_PRINTLN("Data isnt on a valid pin");
		return false;
	}
	pinPeripheral(_rx, (EPioType)_data_mux);

	PM->APBCMASK.reg |= PM_APBCMASK_I2S;

	/* Status check */
	uint32_t ctrla = I2S->CTRLA.reg;
	if (ctrla & I2S_CTRLA_ENABLE) {
		if (ctrla & (I2S_CTRLA_SEREN1 |
			 I2S_CTRLA_SEREN0 | I2S_CTRLA_CKEN1 | I2S_CTRLA_CKEN0)) {
		  //return STATUS_BUSY;
		  return false;
		} else {
		  //return STATUS_ERR_DENIED;
		  return false;
		}
	}

	return true;
#endif
}

void Adafruit_ZeroI2S::enableTx()
{
	I2S->CTRLA.bit.CKEN0 = 1;
	while(I2S->SYNCBUSY.bit.CKEN0);

	I2S->CTRLA.bit.TXEN = 1;
	while(I2S->SYNCBUSY.bit.TXEN);
}

void Adafruit_ZeroI2S::disableTx()
{
	I2S->CTRLA.bit.TXEN = 0;
	while(I2S->SYNCBUSY.bit.TXEN);
}

void Adafruit_ZeroI2S::enableRx(uint8_t clk)
{
	I2S->CTRLA.bit.CKEN0 = 1;
	while(I2S->SYNCBUSY.bit.CKEN0);

	I2S->CTRLA.bit.RXEN = 1;
	while(I2S->SYNCBUSY.bit.RXEN);
}

void Adafruit_ZeroI2S::disableRx()
{
	I2S->CTRLA.bit.RXEN = 0;
	while(I2S->SYNCBUSY.bit.RXEN);
}

void Adafruit_ZeroI2S::enableMCLK()
{
#ifdef PIN_I2S_MCK
	pinPeripheral(PIN_I2S_MCK, PIO_I2S);
#endif
}

void Adafruit_ZeroI2S::disableMCLK()
{
#ifdef PIN_I2S_MCK
	pinMode(PIN_I2S_MCK, INPUT);
#endif
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

void Adafruit_ZeroI2S::read(int32_t *left, int32_t *right)
{
	while( (!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.RXDATA );
	*left = I2S->RXDATA.reg;

	while( (!I2S->INTFLAG.bit.RXRDY0) || I2S->SYNCBUSY.bit.RXDATA );
	*right = I2S->RXDATA.reg;
}
