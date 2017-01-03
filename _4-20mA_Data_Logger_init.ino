






/*
4-20mA Data Logger initialization program
Run this once in order to save space
*/
//Includes
 
  #include <Time.h>
  #include <ADS1115_lite.h>
  #include <SPI.h>
  #include <SD.h>
  #include <RtcDateTime.h>
  #include <RtcDS3231.h>
  #include <EEPROM.h>

  String line = "==================================================================";
  int numFailure;
  //ADS1115 class definitions
    ADS1115_lite adc0(ADS1115_DEFAULT_ADDRESS); 
    ADS1115_lite adc1(ADS1115_ADDRESS_ADDR_VDD);
    
  //Serial Read tags - delete if not doing Serial Reads, Also delete fucntions SerialMonitor and SerialEvent
    #define SERIAL_BUFFER_SIZE 20
    String sSerialBuffer; //Create a global string that is added to character by character to create a final serial read
    String sLastSerialLine; //Create a global string that is the Last full Serial line read in from sSerialBuffer
    
  //RTC class definition
    RtcDS3231 RTC;
  
  //SDCard
    String DataFolderLocation = "Data";    
    String DataFileLocation = DataFolderLocation + "/Data0.CSV";
    unsigned long DataMaxFileSize = 10000000; //Maximum file size in bytes, 
    int DataFileNumber; //Create a int that will reference the Data file number on the SD card e.g.: 4 would work on the file on : SDCARD/Data/Data4.CSV


  struct ADCChan{
    uint16_t Raw;
    float Calib; //Calibration factor to multiply things by
    float ActualLo; //Low value for EngUnits scaling
    float ActualHi;//Low value for EngUnits scaling
    float Current; //Current scaled engineering units from ActualLo and Actual Hi.  This is compared against Actual and Deadband to determine when to update Actual
    float Actual; //scaled engineering units from ActualLo and Actual Hi
    float Deadband; //Swinging door deadband to determine how often to save values
    float mA; //Swinging door deadband to determine how often to save values
  };
  struct ADCChan Ch0;
  struct ADCChan Ch1;
  struct ADCChan Ch2;
  struct ADCChan Ch3;
  float DeadbandRange = 500.0; //divide this number into the range to initialize deadband (ActualHi - ActualLo) / Deadband Range).  Suggested value 500-1000

  File file;
  String FileName;
    
//SETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUP
void setup() {
  
  //Setup Serial 
    //In case the user uses the default setting in Arduino, give them a message to change BAUD rate
      Serial.begin(9600);
      Serial.println(F("Set Serial baud rate to 250000"));
      Serial.flush ();   // wait for send buffer to empty
      delay (2);    // let last character be sent
      Serial.end ();      // close serial
      Serial.begin(250000);
      Serial.println("");
      
  //Set serial read buffer size to reserve the space to concatnate all bytes recieved over serial
    sSerialBuffer.reserve(SERIAL_BUFFER_SIZE);


    DrawLine(); //==================================================================

    
  //I2C start
    Wire.begin(); 


    Serial.print(F("Configuring ADC0:"));
    adc0.setGain(ADS1115_REG_CONFIG_PGA_0_256V);
    adc0.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS);
    adc0.setMux(ADS1115_REG_CONFIG_MUX_DIFF_0_1);
    if (!adc0.testConnection()) {
        Serial.println(F("FAILURE!!!"));
        numFailure++;
    } else {
        Serial.println(F("SUCCESS"));
    }

    Serial.print(F("Configuring ADC1:"));
    adc1.setGain(ADS1115_REG_CONFIG_PGA_0_256V);
    adc1.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS);
    adc1.setMux(ADS1115_REG_CONFIG_MUX_DIFF_0_1);
    if (!adc1.testConnection()) {
        Serial.println(F("FAILURE!!!"));
        numFailure++;
    } else {
        Serial.println(F("SUCCESS"));
    }

     
    DrawLine(); //==================================================================
    Serial.println(F("Configuring RTC"));

  //RTC
    RTC.Begin();
    //Get the compiled time to determine if you should set the Date Time
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    Serial.print(F("Checking Connectivity:"));
    if (!RTC.IsDateTimeValid()) {// Common Cuases: the battery on the device is low or even missing and the power line was disconnected
        Serial.println(F("FAILURE!!!"));
        numFailure++;
    } else {
        Serial.println(F("SUCCESS"));
    }
    
    //Set anncillary features on RTC
        Serial.println(F("Enabling RTC, turning off 32Khz pin and square wave pin"));
        if (!RTC.GetIsRunning()) {RTC.SetIsRunning(true);} // RTC was not actively running
        RtcDateTime now = RTC.GetDateTime();
        if (now < compiled) {RTC.SetDateTime(compiled);} //Update RTC with the compiled time
        RTC.Enable32kHzPin(false);
        RTC.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
        
    //print Date time
    Serial.print(F("Current Date/Time, verify if correct: "));
    Serial.println(getDateString());

    DrawLine(); //==================================================================
    
    Serial.print(F("Starting SD Card:"));
  //SD Card initialization
    if (!SD.begin(10)) {
        Serial.println(F("FAILURE!!!"));
        numFailure++;
    } else {
        Serial.println(F("SUCCESS"));
        Serial.println(F("Creating menu files:"));
        SDCreateMenuFiles();
    }
    
    
    DrawLine(); //==================================================================
        Serial.print(F("Initialization complete: "));
    if (numFailure==0) {
        Serial.println(F("SUCCESS"));
    } else {
        Serial.println(F("MORE THAN ONE FAILURE"));
    }
    DrawLine(); //==================================================================
    Serial.println(F("type (e)to write EEPROM.  This is not done automatically to prevent excess writing of the EEPROM"));
    Serial.println(F("The Date/Time will now print forever..."));
    Serial.println();

    
}
//SETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUP




    



//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
void loop() {
  delay(1000);

  //Wait for Serial data to Update EEPROM
  
      if (sLastSerialLine.equalsIgnoreCase("e")==true) {
        //If anything comes over serial comm send EEPROM data
          Serial.println();
          DrawLine(); //==================================================================
          Serial.println(F("Initializing EEPROM:"));
          UpdateEEPROM();
          DrawLine(); //==================================================================
          Serial.println(); 
        sLastSerialLine = ""; //Clear out buffer, This should ALWAYS be the last line in this if..then
      }

    Serial.print(F("Date and Time is currently: "));
    Serial.println(getDateString());
}
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP

//==============================================================================================
void EndProgram(String ErrorMessage ) {

  //An unrecoverable error occured
  //Loop forever and display the error message
  while(1) {
    Serial.print(F("Firmware encounted non-recoverable error: "));
    Serial.println(ErrorMessage);
    Serial.println(F("Cycle power to restart"));
    delay(5000);
  }
}
void DrawLine() {
  //Menu line separator.  
    Serial.println(line);
}

//==============================================================================================
String getDateString() {
     
    RtcDateTime now = RTC.GetDateTime();
    String CurrentDateString;
    String stemp;
    int temp;
      CurrentDateString = now.Year();  CurrentDateString += "/";
      
        stemp = now.Month();
        temp = stemp.toInt();
        if (temp < 10) {CurrentDateString += "0";}
      CurrentDateString += stemp;  CurrentDateString += "/";
      
        stemp = now.Day();
        temp = stemp.toInt();
        if (temp < 10) {CurrentDateString += "0";}
      CurrentDateString += stemp;  CurrentDateString += " ";
      
        stemp = now.Hour();
        temp = stemp.toInt();
        if (temp < 10) {CurrentDateString += "0";}
      CurrentDateString += now.Hour();  CurrentDateString += ":";
      
        stemp = now.Minute();
        temp = stemp.toInt();
        if (temp < 10) {CurrentDateString += "0";}
      CurrentDateString += stemp;  CurrentDateString += ":";
      
        stemp = now.Second();
        temp = stemp.toInt();
        if (temp < 10) {CurrentDateString += "0";}
      CurrentDateString += stemp;
        
    return CurrentDateString;
}
//=================================================================================================
void SDCreateMenuFiles() {
  //Make the menu directory
    SD.mkdir("Menu");
    
  //Make the Welcome Menu:
   FileName = "Menu/_01_Wel.txt";
    Serial.print(F("Creating/Overwriting File:"));
    Serial.println(FileName);
    SD.remove(FileName);
    file = SD.open(FileName, FILE_WRITE);
      file.println(line);
      file.println(F("4Chan 4-20mA Data Logger"));
      file.println("");  
      file.println(F("Send the text '?' for help (all commands must preceed a LF or CR character"));
      file.println(line);
    file.close();
    
  //Make the Welcome Menu:
    FileName = "Menu/_10_Main.txt";
    Serial.print(F("Creating/Overwriting File:"));
    Serial.println(FileName);
    SD.remove(FileName);
    file = SD.open(FileName, FILE_WRITE);
    
      file.println(line);
      file.println(F("MAIN MENU"));
      file.println(F(""));
      file.println(F("FUNCTIONAL DESCRIPTION:"));
      file.println(F("\tRecords 4 channels of 4-20mA data to a CSV file on attached SD card if any channel changes more than than the channels programmed deadband (up or down)."));
      file.println(F("\tThis is not an industrial appliance, please treat accordingly:.  There is no input protection; don't touch them.  Don't drop, etc."));
      file.println(F("\tPowered by (2) 2100mAh 18650 batteries. ~47mA of power for ~85h/3d8h on battery power. Low battery indicator indicates ~5% charge or ~4h remaining"));
      file.println(F("\tBatteries charge using any standard 5V mobile phone charger capable of at least 1.0A.  ~4h to charge to 80%, ~5h to charge to 100%. Turn off when not in use(obviously!)"));
      file.println(F("\tDo not remove both batteries at once or battery protection circuit will latch off and require resetting"));
      file.println(F("\tUnit always takes samples when powered on.  Powering the unit on is accomplished via the switch which applies battery power to the unit, or"));
      file.println(F("\t   when computer is plugged into the USB data port.  Battery charging can occur when the unit is switched off."));
      file.println(F("\t"));
      file.println();
      file.println(F("COMMANDS:"));
      file.println(F("Note that all commands follow a CSV syntax: {letter} OR {letter},{number},...,{number}.  All commands are accessisible from all 'menus'"));
      file.println(F("\t1. (c)onfig - channel config and calibration"));
      file.println(F("\t2. (f)ile managemant and data - Prints data from storage device"));
      file.println(F("\t3. (t)ime,(YYYY),(MM),(DD),(HH),(MM),(SS) - Update RTC (Real time clock): e.g. t,2017,3,5,00,12,30 to set time to March 5th, 2016 @ 00:12:30"));
      file.println();
      file.println(F("For additional information please contact Terry Myers m:215.262.4148"));
      file.println(line);
      
    file.close();
   
  //Channel Config Screen 1of2
    FileName = "Menu/_20_Cfg.txt";
    Serial.print(F("Creating/Overwriting File:"));
    Serial.println(FileName);
    SD.remove(FileName);
    file = SD.open(FileName, FILE_WRITE);
    
      file.println(line);
      file.println(F("CHANNEL CONFIGURATION"));
      file.println(F(""));
      file.println(F("Current Channel Configuration:"));
      file.println(       F("\t\t\tEngUnits\tEngUnits\tEngUnits\tEngUnits\t\t\tCalibration"));
      file.println(F("Ch\tRaw(mA)\t\tCurrent\t\tActual\t\tLow\t\tHigh\t\tDeadband\tFactor"));
                     //1   4.0923  192.0029        0.0000   100.00000    1.0001
    file.close();


  //Channel Config Screen 2of2
    FileName = "Menu/_21_Cfg.txt";
    Serial.print(F("Creating/Overwriting File:"));
    Serial.println(FileName);
    SD.remove(FileName);
    file = SD.open(FileName, FILE_WRITE);
      file.println(F("COMMANDS:"));
      file.println(F("\t1. (c)hannel onfig,(0-3)Channel,(Low), (High), (Deadband), (Calibration Factor) - configure a channel"));
      file.println(F("\t\te.g. c,2,0,2200,1,1.002, configure channel 2 to have EngUnits from 0 to 2200, a deadband of 1.0, and a calibration factor of 1.002"));
      file.println(F("\t2. (l)ive channel readouts.  Toggle on/off by typing (l)"));
      file.println();


      file.println(F("MEASUREMENT DESCRIPTION:"));
      file.println(F("\tA 4-20mA signal is measured via a 10R shunt resistor with 16 bits of precision (~15 bits of noise free resolution) with a 0.2% accuracy between 23C-35C"));
      file.println(F("\tScaling Schema for each channel:  0-25.6mA(Max) actual is 0-.256V across resistor is converted to: 0-32767 * Calibration Factor -> 0-25.6mA -> EngUnits Low - EngUnits High -> Current"));
      file.println(F("\tAll Current values are compared with their Deadband.  If ANY channel changes more than the Deadband (up or down), the Current value is copied into the Actual value"));
      file.println(F("\t   and a time stamped record is stored on the SD card for ALL channels in a CSV with the format: YYYY/MM/DD HH/MM/SS,Ch0.Actual,Ch1.Actual,Ch2.Actual,Ch3.Actual"));
      file.println(F("\tSuggested Deadband value is 1/500 of the range.  e.g. Lo = 0, Hi = 250, deadband = (250 - 0)/500 = 0.5.  Recommend not more than 1/200 (not enough resolution)and not less than 1/1000 (into noise range)"));
      file.println(F("\tMeasurements are taken on all channels about twice a second"));
      file.println();
      
      file.println(F("CALIBRATION:"));
      file.println(F("\tCalibraton factor is applied to the raw signal to get Raw(mA) displayed and used by this device."));
      file.println(F("\te.g. Current Calibration Factor = 1.01.  If the Raw(mA) displayed (measured by this device) = 4.100, but 4.000 is measured with an external accurate instrument(e.g. loop calibrator),"));
      file.println(F("\t     then change Calibration Factor to 0.98537 = measured / displayed * Current Calibration Factor = 4.0 / 4.1 * 1.01"));
      file.println(line);
      
    file.close();

             

  //File Management
    FileName = "Menu/_30_File.txt";
    Serial.print(F("Creating/Overwriting File:"));
    Serial.println(FileName);
    SD.remove(FileName);
    file = SD.open(FileName, FILE_WRITE);
    
      file.println(line);
      file.println(F("FILE MANAGEMENT AND DATA"));
      file.println();
      file.println(F("COMMANDS:"));
      file.println(F("\t1. (dir) - list directory information"));
      file.println(F("\t2. (del),(File Number) - deletes the file.  e.g. del,3 deletes the file DATA3.CSV"));
     // file.println(F("\t3. (deltree) - deletes all files without a confirmation"));
      file.println(F("\t3. (s)et file number, (File Number) - sets the current working file: e.g. s,5 sets the device to start writing to file DATA5.CSV.  If full it would create a DATA6.CSV, etc."));
      file.println(F("\t4. (p)rint file,(Number) - print the entire contents of the file to the Serial monitor. e.g. p,0 will print file DATA0.CSV"));
      file.println(F("\t    Note that each 1MB (1,048,576bytes) takes ~53sec to print here."));
      file.println(F("\t    You can also of course take the SD card out and put it into your computer directly for MUCH faster data transfer."));
      file.println();
      file.println(line);
      
    file.close();

    
}
          

//==============================================================================================
void serialEvent() {
  while (Serial.available()) {

    char inChar = (char)Serial.read();// get the new byte:

    if (inChar == '\n' || inChar == '\r' ) {//if the incoming character is a LF or CR finish the string
      sLastSerialLine = sSerialBuffer; //Transfer the entire string into the last serial line
      sSerialBuffer = ""; // clear the string buffer to prepare it for more data:
      break;
    }

    sSerialBuffer += inChar;// add it to the inputString:
  }
}

//=================================================================================================
void EEPROM_WriteString(int Address_Start, String s) {
  //Write the length of the string to Address_Start, then write the characters of the string starting at Address_Start+1
  //Note that this is a lite function with no error checking.  It should check to ensure you aren't going to go over the max size of the EEPROM
  //a=Address
  //s=String
  EEPROM.write(Address_Start,s.length());
  for (int i=1;i<=s.length()+1;i++) {
      EEPROM.write(Address_Start+i,s[i]);
    }
}

//=================================================================================================
void EEPROM_PrintString(int Address_Start) {
  //Pass in an address where the length of the string is stored.  Address_Start+1 starts the string
  //Note that this is a lite function with no error checking.  It is assumed you are NOT going to go outside of the bounds of the EEPROM size
    int l = EEPROM.read(Address_Start); //Get length
    for (int i=1;i<=l;i++) {
      Serial.print(char(EEPROM.read(i)));
    }
}
//=================================================================================================
void UpdateEEPROM() {
  
  //Read the debug boolean and set the debug flag
    int address = 2; //Temporary EEPROM address pointer
    int DataFileNumber=0;
    EEPROM.put(address,DataFileNumber); address++; //Set the first file to 0
    address = 10; //Repoint EEPROM pointer
    Ch0.Calib = 1.0; //initialize value
    EEPROM.put(address,Ch0.Calib);
    EEPROM.get(address,Ch0.Calib);
    Serial.print(F("Ch0.Calib initialized to:")); Serial.println(Ch0.Calib,6); address = address + 4;
    Ch0.ActualLo = 0.0; //initialize value
    EEPROM.put(address,Ch0.ActualLo);
    EEPROM.get(address,Ch0.ActualLo);
    Serial.print(F("Ch0.ActualLo initialized to:")); Serial.println(Ch0.ActualLo,6); address = address + 4;
    Ch0.ActualHi = 100.0; //initialize value
    EEPROM.put(address,Ch0.ActualHi);
    EEPROM.get(address,Ch0.ActualHi);
    Serial.print(F("Ch0.ActualHi initialized to:")); Serial.println(Ch0.ActualHi,6); address = address + 4;
    Ch0.Deadband = (Ch0.ActualHi - Ch0.ActualLo)/DeadbandRange; //initialize value
    EEPROM.put(address,Ch0.Deadband);
    EEPROM.get(address,Ch0.Deadband);
    Serial.print(F("Ch0.Deadband initialized to:")); Serial.println(Ch0.Deadband,6); address = address + 4;


 
    Ch1.Calib = 1.0; //initialize value
    EEPROM.put(address,Ch1.Calib);
    EEPROM.get(address,Ch1.Calib);
    Serial.print(F("Ch1.Calib initialized to:")); Serial.println(Ch1.Calib,6); address = address + 4;
    Ch1.ActualLo = 0.0; //initialize value
    EEPROM.put(address,Ch1.ActualLo);
    EEPROM.get(address,Ch1.ActualLo);
    Serial.print(F("Ch1.ActualLo initialized to:")); Serial.println(Ch1.ActualLo,6);address = address + 4;
    Ch1.ActualHi = 100.0; //initialize value
    EEPROM.put(address,Ch1.ActualHi);
    EEPROM.get(address,Ch1.ActualHi);
    Serial.print(F("Ch1.ActualHi initialized to:")); Serial.println(Ch1.ActualHi,6);address = address + 4;
    Ch1.Deadband = (Ch1.ActualHi - Ch1.ActualLo)/DeadbandRange; //initialize value
    EEPROM.put(address,Ch1.Deadband);
    EEPROM.get(address,Ch1.Deadband);
    Serial.print(F("Ch1.Deadband initialized to:")); Serial.println(Ch1.Deadband,6); address = address + 4;
      

    Ch2.Calib = 1.0; //initialize value
    EEPROM.put(address,Ch2.Calib);
    EEPROM.get(address,Ch2.Calib);
    Serial.print(F("Ch2.Calib initialized to:")); Serial.println(Ch2.Calib,6);address = address + 4;
    Ch2.ActualLo = 0.0; //initialize value
    EEPROM.put(address,Ch2.ActualLo);
    EEPROM.get(address,Ch2.ActualLo);
    Serial.print(F("Ch2.ActualLo initialized to:")); Serial.println(Ch2.ActualLo,6);address = address + 4;
    Ch2.ActualHi = 100.0; //initialize value
    EEPROM.put(address,Ch2.ActualHi);
    EEPROM.get(address,Ch2.ActualHi);
    Serial.print(F("Ch2.ActualHi initialized to:")); Serial.println(Ch2.ActualHi,6);address = address + 4;
    Ch2.Deadband = (Ch2.ActualHi - Ch2.ActualLo)/DeadbandRange; //initialize value
    EEPROM.put(address,Ch2.Deadband);
    EEPROM.get(address,Ch2.Deadband);
    Serial.print(F("Ch2.Deadband initialized to:")); Serial.println(Ch2.Deadband,6); address = address + 4;
      

    Ch3.Calib = 1.0; //initialize value
    EEPROM.put(address,Ch3.Calib);
    EEPROM.get(address,Ch3.Calib);
    Serial.print(F("Ch3.Calib initialized to:")); Serial.println(Ch3.Calib,6);  address = address + 4;
    Ch3.ActualLo = 0.0; //initialize value
    EEPROM.put(address,Ch3.ActualLo);
    EEPROM.get(address,Ch3.ActualLo);
    Serial.print(F("Ch3.ActualLo initialized to:")); Serial.println(Ch3.ActualLo,6);  address = address + 4;
    Ch3.ActualHi = 100.0; //initialize value
    EEPROM.put(address,Ch3.ActualHi);
    EEPROM.get(address,Ch3.ActualHi);
    Serial.print(F("Ch3.ActualHi initialized to:")); Serial.println(Ch3.ActualHi,6);  address = address + 4;
    Ch3.Deadband = (Ch3.ActualHi - Ch3.ActualLo)/DeadbandRange; //initialize value
    EEPROM.put(address,Ch3.Deadband);
    EEPROM.get(address,Ch3.Deadband);
    Serial.print(F("Ch3.Deadband initialized to:")); Serial.println(Ch3.Deadband,6); address = address + 4;

}

void SerialPrintFile(String s) {
    //Print an entire file to the Serial stream
    file = SD.open(s);
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
}

