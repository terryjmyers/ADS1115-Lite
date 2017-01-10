# ADS1115-Lite

ADS1115 lite library - Adapted from adafruit ADS1015/ADS1115 library

This library is stripped down version with bug fixes from the adafruit ADS1015/ADS1115 library in order to save as much space as possible
		1. No explicit "Read ADC" functions exist to save space.  Simply set your mux manually, trigger a conversion manually, and read the value manually.  See the example program
		2. ADS1015 functionality removed to save space.  Note that this library should work for the ADS1015, the only difference is the sample rate constants.
		3. Continuous Conversion removed
			a. If "continuous conversion" is required, perhaps you should rethink your requirements.  Continuous conversion is never REALLY required.
			b. That being said, if you want a continuous conversion simply start a conversion after reading a value by executing "adc.triggerConversion()" immediatly AFTER an "adc.getConversion();".  That's all the chip does anyway...
		4. Comparator functionality removed to save space.  Cmon..you can program that functionality if you REALLY need it, this is a lite library
		5. I2C abstraction removed to save space.  Is anyone using old Arduino IDE versions?
		6. keywords.txt added in order to display proper keyword coloring in the Arduino IDE
		7. All report bug fixes from the adafruit library on github
			a. Most notably the incorrect sample rate used when using the ADS1115.  Yes that's right, if you used the Adafruit library the sample rate was set to the fastest, noisiest sample rate by default with no way to change it!  Opps!
		8. Proper ifndef added to prevent multiple calls
		9. Removed explicit delays and intead poll the Operational Status bit (conversion done)
		 
