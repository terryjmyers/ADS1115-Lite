/*
ADS1115_Lite example code
//Minimal code to read ADS1115.  The mux setting must be set every time each channel is read, there is not a separate function call for each possible mux combination.  Only single shot conversion mode is supported, but continuous conversion mode can be implemented easily with code
//]
*/

#include <ADS1115_lite.h>
ADS1115_lite adc(ADS1115_DEFAULT_ADDRESS); //Initializes wire library, sets private configuration variables to ADS1115 default.  The Address parameter is not required.

      unsigned long starttime;
        unsigned long endtime ;
        int16_t Raw;
        
void setup(void) 
{
   //Setup Serial 
    //In case the user uses the default setting in Arduino, give them a message to change BAUD rate
      Serial.begin(9600);
      Serial.println(F("Set Serial baud rate to 250,000"));
      Serial.flush ();   // wait for send buffer to empty
      delay (2);    // let last character be sent
      Serial.end ();      // close serial
      Serial.begin(250000);
      Serial.println();
  
    
    adc.setGain(ADS1115_REG_CONFIG_PGA_0_256V);
    adc.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS); //Set the slowest most accurate sample speed
    
    if (!adc.testConnection()) {
      Serial.println("ADS1115 Connection failed");
      return;
    }
}

void loop(void) {


  
        
    //You must initialize the ADC manually before each read.
    Serial.print("Setting mux to SE mode ch0, 8SPS...");
        adc.setMux(ADS1115_REG_CONFIG_MUX_SINGLE_0); //Set single ended mode

    Serial.println("triggering conversion...");
        starttime = micros();
        adc.triggerConversion(); //Start a conversion.  This immediatly returns
        Raw = adc.getConversion(); //Wait for conversion to finish and then return.
        endtime = micros();
    Serial.print("Conversion complete: "); Serial.print(Raw); Serial.print(",  "); Serial.print(endtime - starttime);  Serial.print("us");
    Serial.println();
    Serial.println("=======================");

    delay(2000); //Delay just because

    Serial.print("Setting mux to differential mode between ch0 and ch1 @128SPS...");
        adc.setMux(ADS1115_REG_CONFIG_MUX_DIFF_0_1); //Set mux...this must be done each time you want to read another channel
        adc.setSampleRate(ADS1115_REG_CONFIG_DR_128SPS); //Change the sample rate just for demonstration
    Serial.println("Triggering conversion");
        starttime = micros();
        adc.triggerConversion(); //Start a conversion.  This immediatly returns
        Raw = adc.getConversion(); //Wait for conversion to finish and then return.
        endtime = micros();
    Serial.print("Conversion complete: "); Serial.print(Raw); Serial.print(",  ");  Serial.print(endtime - starttime);  Serial.print("us");
    Serial.println();
    Serial.println("=======================");


      adc.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS); //Set the slowest speed just to prevent serial monitor flooding for the next step
    Serial.println("Continuous conversion");
    //Demonstrate continuous conversion
    do {
      //just loop forever and update the Raw value continuously
       Raw = GetConversion();

    } while (1);
}
//========================================================================
int16_t GetConversion() {

  //You can make different functions for different mux settings
  adc.setMux(ADS1115_REG_CONFIG_MUX_DIFF_2_3); //change mux just for kicks
  
    if (adc.isConversionDone()) {; //If the conversion is done, do something and trigger another one!
        Raw = adc.getConversion(); //Get conversion value
        adc.triggerConversion(); //immediatly start another one!  See its that easy to do "continuous conversion"
        Serial.print("Conversion complete: "); Serial.print(Raw);
        Serial.println();
    }
  return Raw; //Always return the Raw value.  If it was updated above great!, of not, great!
  
}
