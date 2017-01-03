/**************************************************************************/
/*!
Adapted from adafruit ADS1115/ADS1115 library
*/
/**************************************************************************/

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>

/*=========================================================================
    I2C ADDRESS/BITS
    -----------------------------------------------------------------------*/
	#define ADS1115_ADDRESS_ADDR_GND    0x48 // address pin low (GND)
	#define ADS1115_ADDRESS_ADDR_VDD    0x49 // address pin high (VCC)
	#define ADS1115_ADDRESS_ADDR_SDA    0x4A // address pin tied to SDA pin
	#define ADS1115_ADDRESS_ADDR_SCL    0x4B // address pin tied to SCL pin
	#define ADS1115_DEFAULT_ADDRESS     ADS1115_ADDRESS_ADDR_GND
/*=========================================================================*/

/*=========================================================================
    POINTER REGISTERS
    -----------------------------------------------------------------------*/
    #define ADS1115_REG_POINTER_MASK        (0x03)
    #define ADS1115_REG_POINTER_CONVERT     (0x00)
    #define ADS1115_REG_POINTER_CONFIG      (0x01)
/*=========================================================================*/

/*=========================================================================
    CONFIG REGISTER
    -----------------------------------------------------------------------*/
    #define ADS1115_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
    #define ADS1115_REG_CONFIG_OS_BUSY      (0x0000)  // Read: Bit = 0 when conversion is in progress
    #define ADS1115_REG_CONFIG_OS_NOTBUSY   (0x8000)  // Read: Bit = 1 when device is not performing a conversion

    #define ADS1115_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
    #define ADS1115_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
    #define ADS1115_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
    #define ADS1115_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
    #define ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
    #define ADS1115_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
    #define ADS1115_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
    #define ADS1115_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

    #define ADS1115_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
    #define ADS1115_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
    #define ADS1115_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
    #define ADS1115_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
    #define ADS1115_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
    #define ADS1115_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16

	//Not used.  This is a single shot only
    #define ADS1115_REG_CONFIG_MODE_CONTIN  (0x0000)  // Continuous conversion mode
    #define ADS1115_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)

    #define ADS1115_REG_CONFIG_DR_8SPS    (0x0000)  // 8 samples per second
    #define ADS1115_REG_CONFIG_DR_16SPS    (0x0020)  // 16 samples per second
    #define ADS1115_REG_CONFIG_DR_32SPS    (0x0040)  // 32 samples per second
    #define ADS1115_REG_CONFIG_DR_64SPS    (0x0060)  // 64 samples per second
    #define ADS1115_REG_CONFIG_DR_128SPS   (0x0080)  // 128 samples per second (default)
    #define ADS1115_REG_CONFIG_DR_250SPS   (0x00A0)  // 250 samples per second
    #define ADS1115_REG_CONFIG_DR_475SPS   (0x00C0)  // 475 samples per second
    #define ADS1115_REG_CONFIG_DR_860SPS   (0x00E0)  // 860 samples per second

    #define ADS1115_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
    #define ADS1115_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

    #define ADS1115_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
    #define ADS1115_REG_CONFIG_CPOL_ACTVHI  (0x0008)  // ALERT/RDY pin is high when active

    #define ADS1115_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
    #define ADS1115_REG_CONFIG_CLAT_LATCH   (0x0004)  // Latching comparator

    #define ADS1115_REG_CONFIG_CQUE_1CONV   (0x0000)  // Assert ALERT/RDY after one conversions
    #define ADS1115_REG_CONFIG_CQUE_2CONV   (0x0001)  // Assert ALERT/RDY after two conversions
    #define ADS1115_REG_CONFIG_CQUE_4CONV   (0x0002)  // Assert ALERT/RDY after four conversions
    #define ADS1115_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)
/*=========================================================================*/

class ADS1115_lite
{
protected:
   // Instance-specific properties
   uint8_t   m_i2cAddress;
   uint16_t	 m_gain;
   uint16_t	 m_mux;
   uint8_t	 m_rate;

 public:
  ADS1115_lite(uint8_t i2cAddress = ADS1115_DEFAULT_ADDRESS);
  bool 		testConnection(); //returns 1 if connected, 0 if not connected
  void      setGain(uint16_t gain); //Sets Gain
  void      setMux(uint16_t mux); //Sets mux
  void      setSampleRate(uint8_t rate); //Sets sample rate
  
  void   	triggerConversion(void); //Triggers a single conversion with currently configured settings.  Immediately returns
  bool		isConversionDone(); //Returns 1 if conversion is finished, 0 if in the middle of conversion.  
  int16_t   getConversion(); //Waits for isConversionDone, and returns results.

};
