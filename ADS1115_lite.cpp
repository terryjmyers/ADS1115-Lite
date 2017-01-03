/**************************************************************************/
/*!
Adapted from adafruit ADS1015/ADS1115 library

    v1.0 - First release
*/
/**************************************************************************/
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

#include "ADS1115_lite.h"


/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(i2cAddress);
  #if ARDUINO >= 100
	  Wire.write((uint8_t)reg);
	  Wire.write((uint8_t)(value>>8));
	  Wire.write((uint8_t)(value & 0xFF));
  #else
	  Wire.send((uint8_t)reg);
	  Wire.send((uint8_t)(value>>8));
	  Wire.send((uint8_t)(value & 0xFF));
  #endif
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg) {
  Wire.beginTransmission(i2cAddress);
  #if ARDUINO >= 100
	  Wire.write((uint8_t)reg);
  #else
	  Wire.send((uint8_t)reg);
  #endif
  Wire.endTransmission();
  Wire.requestFrom(i2cAddress, (uint8_t)2);
  #if ARDUINO >= 100
    return ((Wire.read() << 8) | Wire.read()); 
  #else
    return ((Wire.receive() << 8) | Wire.receive()); 
  #endif
}

/**************************************************************************/
/*!
    @brief  Instantiates a new ADS1115 class w/appropriate properties
*/
/**************************************************************************/
ADS1115_lite::ADS1115_lite(uint8_t i2cAddress)
{
   Wire.begin();
   m_i2cAddress = i2cAddress;
   m_gain = ADS1115_REG_CONFIG_PGA_6_144V; /* +/- 6.144V range (limited to VDD +0.3V max!) */
   m_mux = ADS1115_REG_CONFIG_MUX_DIFF_0_1; /* to default */
   m_rate = ADS1115_REG_CONFIG_DR_128SPS; /* to default */
}

/**************************************************************************/
/*!
    @brief  Sets up the HW (reads coefficients values, etc.)
*/
/**************************************************************************/
bool ADS1115_lite::testConnection() {
   Wire.beginTransmission(m_i2cAddress);
	  #if ARDUINO >= 100
		  Wire.write(ADS1115_REG_POINTER_CONVERT);
	  #else
		  Wire.send(ADS1115_REG_POINTER_CONVERT);
	  #endif
  Wire.endTransmission();
  Wire.requestFrom(m_i2cAddress, (uint8_t)2);
	if (Wire.available()) {return 1;}
	return 0;
}

/**************************************************************************/
/*!
    @brief  Sets the gain and input voltage range.  Valid numbers:
	 ADS1115_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
	 ADS1115_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
	 ADS1115_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
	 ADS1115_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
	 ADS1115_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
	 ADS1115_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16
*/
/**************************************************************************/
void ADS1115_lite::setGain(uint16_t gain)
{
  m_gain = gain;
}

/**************************************************************************/
/*!
    @brief  Sets the mux.  Valid numbers:
	ADS1115_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
	ADS1115_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
	ADS1115_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
	ADS1115_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
	ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
    ADS1115_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
    ADS1115_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
    ADS1115_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3
*/
/**************************************************************************/
void ADS1115_lite::setMux(uint16_t mux)
{
  m_mux = mux;
}
/**************************************************************************/
/*!
    @brief  Sets the mux.  Valid numbers:
	ADS1115_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
	ADS1115_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
	ADS1115_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
	ADS1115_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
	ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
    ADS1115_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
    ADS1115_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
    ADS1115_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3
*/
/**************************************************************************/
void ADS1115_lite::setSampleRate(uint8_t rate)
{
  m_rate = rate;
}
/**************************************************************************/
/*! 
		Trigger Conversion to Read later
*/
/**************************************************************************/
void ADS1115_lite::triggerConversion() {
  // Start with default values
  uint16_t config = ADS1115_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1115_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1115_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    m_rate   						| // Set Sample Rate 
                    ADS1115_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // OR in the PGA/voltage range
  config |= m_gain;
                    
  // OR in the mux channel
  config |= m_mux;

  // OR in the start conversion bit
  config |= ADS1115_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1115_REG_POINTER_CONFIG, config);
}

/**************************************************************************/
/*!
    @brief  Waits and returns the  last conversion results
*/
/**************************************************************************/
int16_t ADS1115_lite::getConversion()

{  // Wait for the conversion to complete
   do {
      } while(!isConversionDone());
	  
  // Read the conversion results
  return readRegister(m_i2cAddress, ADS1115_REG_POINTER_CONVERT);
}
/**************************************************************************/
/*!
    @brief  returns true if the conversion is done
*/
/**************************************************************************/
bool ADS1115_lite::isConversionDone()
{
	int16_t test =readRegister(m_i2cAddress, ADS1115_REG_POINTER_CONFIG);
  return test>>15 ;
}
