/*
4 channel 4-20mA Data Logger lite
companion initialization script required to initialize EEPROM and files for Serial printing large amounts of text
This is a lite program because it was stripped down to absolute minimal functionality to fit under 32K, so there is no debugging or verbose messaging.



    53B per record
    ~364 Records per second measured
    SD.Open Time = 11ms
    SD.print one line record time = 0.1ms
    Get RTC time = 3.8ms
    SD.Close Time = 15ms






*/

//Includes
  #include <EEPROM.h>
  #include <Time.h>
  #include <ADS1115_lite.h>
  #include <SPI.h>
  #include <SD.h>
  #include <RtcDS3231.h>

    
   // float sdValues[50];
  // int sdValuesPTR;
  
  #define ADC_BITS 32767
  
  //RTC class definition
    RtcDS3231 RTC;
  
  //ADS1115 class definitions
    ADS1115_lite adc0(ADS1115_DEFAULT_ADDRESS);
    ADS1115_lite adc1(ADS1115_ADDRESS_ADDR_VDD);
    
  //ADC Resistor and calibration factors
    struct ADCChan{
      int Raw;
      float Calib; //Calibration factor to multiply things by
      float ActualLo; //Low value for EngUnits scaling
      float ActualHi;//High for EngUnits scaling
      float Current; //Current scaled engineering units from ActualLo and Actual Hi.  This is compared against Actual and Deadband to determine when to update Actual
      float Actual; //scaled engineering units from ActualLo and Actual Hi
      float Deadband; //Swinging door deadband to determine how often to save values
      boolean Change; //Flag set on each scan when the current value has changed outside of its deadband and Actual has been updated.  Check this to log the value.  This is reset at the beggining of each scan
      byte Precision; //Number of decimal places to show
      float mA; //Swinging door deadband to determine how often to save values
    };
    struct ADCChan Ch0;
    struct ADCChan Ch1;
    struct ADCChan Ch2;
    struct ADCChan Ch3;
    
  //SDCard
    String DataFolderLocation = "Data";    
    String DataFileLocation = DataFolderLocation + "/Data0.CSV";
    #define DataMaxFileSize 1048576 //Maximum file size in bytes, set at 1MB or ~ 17,300 records.
    int DataFileNumber; //Create a int that will reference the Data file number on the SD card e.g.: 4 would work on the file on : SDCARD/Data/Data4.CSV
    File file;
    
  //misc
      boolean livereadout; //a tag to store if the user has turned on live readout on or off
    
  //Serial Read tags - delete if not doing Serial Reads, Also delete fucntions SerialMonitor and SerialEvent
    #define SERIAL_BUFFER_SIZE 20
    String sSerialBuffer; //Create a global string that is added to character by character to create a final serial read
    String sLastSerialLine; //Create a global string that is the Last full Serial line read in from sSerialBuffer
    
//SETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUP
void setup() {
  
  
  //Setup Serial 
    //In case the user uses the default setting in Arduino, give them a message to change BAUD rate
      Serial.begin(9600);
      Serial.println("Set Serial baud rate to 115200");
      Serial.flush ();   // wait for send buffer to empty
      delay (2);    // let last character be sent
      Serial.end ();      // close serial
    Serial.begin(250000);
      Serial.println();
      
  //Set serial read buffer size to reserve the space to concatnate all bytes recieved over serial
    sSerialBuffer.reserve(SERIAL_BUFFER_SIZE);
  
  //Read the debug boolean and set the debug flag
    int address = 2; //Temporary EEPROM address pointer
    EEPROM.get(address,DataFileNumber);
    
    address = 10; //Repoint EEPROM pointer                    //EEPROM ADDRESSES
    EEPROM.get(address,Ch0.Calib); address = address + 4;     //10
    EEPROM.get(address,Ch0.ActualLo); address = address + 4;  //14
    EEPROM.get(address,Ch0.ActualHi); address = address + 4;  //18
    EEPROM.get(address,Ch0.Deadband); address = address + 4;  //22

    EEPROM.get(address,Ch1.Calib); address = address + 4;     //26
    EEPROM.get(address,Ch1.ActualLo); address = address + 4;  //30
    EEPROM.get(address,Ch1.ActualHi); address = address + 4;  //34
    EEPROM.get(address,Ch1.Deadband); address = address + 4;  //38
        
    EEPROM.get(address,Ch2.Calib); address = address + 4;     //42
    EEPROM.get(address,Ch2.ActualLo); address = address + 4;  //46
    EEPROM.get(address,Ch2.ActualHi); address = address + 4;  //50
    EEPROM.get(address,Ch2.Deadband); address = address + 4;  //54
        
    EEPROM.get(address,Ch3.Calib); address = address + 4;     //58
    EEPROM.get(address,Ch3.ActualLo); address = address + 4;  //62
    EEPROM.get(address,Ch3.ActualHi); address = address + 4;  //66
    EEPROM.get(address,Ch3.Deadband); address = address + 4;  //70
                                                              //74
  //SDCard 
    DataFileLocation = FileLocation(DataFileNumber);     
     
  //I2C start
    Wire.begin(); 

  //ADS1115 Setup
    adc0.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS);
    adc0.setGain(ADS1115_REG_CONFIG_PGA_0_256V);
    adc1.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS);
    adc1.setGain(ADS1115_REG_CONFIG_PGA_0_256V);
    if (!adc0.testConnection() | !adc1.testConnection()) {EndProgram(F("One of the ADCs has failed"));}
    
   
  //RTC
    RTC.Begin();
    if (!RTC.IsDateTimeValid()) {EndProgram(F("ERROR: RTC failure.  Please set time and/or change battery."));}// Common Cuases: the battery on the device is low or even missing and the power line was disconnected
    
  //SD Card initialization
    if (!SD.begin(10)) {EndProgram(F("Error: SD Card failure.  Insert SD Card"));}
    
  //Printout welcome message
    SerialPrintFile(F("Menu/_01_Wel.txt"));

  //Pin Setup
    pinMode(2, INPUT);
}
//SETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUP




    



//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
void loop() {
  

  SerialMonitor(); //Monitor Serial Inputs
  
  //Get and process inputs then log data
      Data();
      
  //Sleep for some time

   // LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);   //deep powered down sleep

}
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP


//==============================================================================================
void SerialPrintFile(String s) {
    //Print an entire file to the Serial stream
    file = SD.open(s);
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
}
//==============================================================================================
String FileLocation(int number) {
  //Pass in the number to append to the file name. Returns the file name
    String f;
    f += DataFolderLocation;
    f += "/Data";
    f += String(number);
    f += ".CSV";
    return f;
}
//==============================================================================================
void Data() {
  //See if data has changed, if so, write to SD
  
  //Get Inputs
      getInputs();

  //print live readouts if enabled
  if (livereadout) {
    PrintChannelLive(&Ch0);
    PrintChannelLive(&Ch1);
    PrintChannelLive(&Ch2);
    PrintChannelLive(&Ch3);
    Serial.println();
  }
      
  //Check to see if any values changed, if not return
      if (!Ch0.Change && !Ch1.Change && !Ch2.Change && !Ch3.Change) {
        return;
      }
  
  //Check to see if the data file exists, if not, try to create it
      if (!SD.exists(DataFileLocation)) { 
        Serial.print(F("creating file: ")); Serial.println(DataFileLocation);
        if (!SD.mkdir(DataFolderLocation)) {
          Serial.println(F("Failed to make folder on SD Card, no data will be saved"));
          return;
        }
        File temp = SD.open(DataFileLocation, FILE_WRITE); //Open file SDCard\Data\Data.CSV
        temp.println(F("Date,Ch0,Ch1,Ch2,Ch3")); //print header
        temp.close();
      }
      
  //Check to see if the file has reached the max file size, if so make a new one
 File dataFile = SD.open(DataFileLocation, FILE_WRITE); //Open file SDCard\Data\Data.CSV
        if (dataFile.size() > DataMaxFileSize) { //Data/DataX.CSV is too big, make another file one.
          dataFile.close(); //Close the file
          DataFileNumber++;
          EEPROM.put(2,DataFileNumber);
          DataFileLocation = FileLocation(DataFileNumber);
          File dataFile = SD.open(DataFileLocation, FILE_WRITE); //Open file SDCard\Data\Data.CSV
        }
  
  //Concatenate the line to print   
      String line;
      line += getDateString();
      line += ",";
      line += String(Ch0.Actual,Ch0.Precision);
      line += ",";
      line += String(Ch1.Actual,Ch1.Precision);
      line += ",";
      line += String(Ch2.Actual,Ch2.Precision);
      line += ",";
      line += String(Ch3.Actual,Ch3.Precision);
      line += ",";
    
  dataFile.println(line);
  dataFile.close(); //Close the file
}
//==============================================================================================
String getDateString() {
  
   if (!RTC.IsDateTimeValid()) {
    Serial.println(F("ERROR: RTC failure.  Please set time and/or change battery."));
    return "Time Error";
   }// Common causes: the battery on the device is low or even missing and the power line was disconnected
       
    RtcDateTime now = RTC.GetDateTime();
    String s;
    byte temp;
    
      s = now.Year();  s += "/";
      
        temp = now.Month();
        if (temp < 10) {s += "0";}
        s += String(temp);  s += "/";
      
        temp = now.Day();
        if (temp < 10) {s += "0";}
        s += String(temp);  s += " ";
      
        temp = now.Hour();
        if (temp < 10) {s += "0";}
        s += String(temp);  s += ":";
      
        temp = now.Minute();
        if (temp < 10) {s += "0";}
        s += String(temp);  s += ":";
      
        temp = now.Second();
        if (temp < 10) {s += "0";}
        s += String(temp);
        
    return s;
}
//==============================================================================================
int ADCProcessing(struct ADCChan *Ch) {
  //Reset the Change bit
      Ch->Change = 0;
  
  //Scale the Raw value(0-32767) to mA (0-25.6)
      Ch->mA = SCP(Ch->Raw, 0.0, ADC_BITS, 0.0, 25.6) * Ch->Calib;
      
  //Scale the Raw value to Eng Units
      Ch->Current = SCP(Ch->mA, 4.0, 20.0, Ch->ActualLo, Ch->ActualHi);    

  //Check to see if it has moved outside of the Actual value
      if ((Ch->Current >= Ch->Actual + Ch->Deadband) || (Ch->Current <= Ch->Actual - Ch->Deadband)) { //If it moved outside of the deadband update the actual
        Ch->Actual = Ch->Current;
        Ch->Change = 1; //Set Change bit true if Actual has been updated
      }

  //Determine number of decimal places that are important
      float ftemp = (Ch->ActualHi - Ch->ActualLo)/ADC_BITS;
      if (ftemp > 0.305185095) {
          Ch->Precision = 0;
      } else if (ftemp >= 0.030518509) {
          Ch->Precision = 1;            
      } else if (ftemp >= 0.003051851) {
          Ch->Precision = 2;            
      } else if (ftemp >= 0.000305185) {
          Ch->Precision = 3;            
      } else if (ftemp >= 0.0000305185) {
          Ch->Precision = 4;            
      } else {
          Ch->Precision = 5;
      }
}
//==============================================================================================
float SCP(float Raw, float RawLo, float RawHi, float ScaleLo, float ScaleHi) {
  return (Raw - RawLo) * (ScaleHi - ScaleLo) / (RawHi - RawLo) + ScaleLo;
}
//==============================================================================================
void getInputs() {
  //Read both channels on both ADCs

 
  //Trigger both channels on both ADCs. Each read takes 128ms, the reads are staggered.  Total measured time is 256.2ms

      adc0.setMux(ADS1115_REG_CONFIG_MUX_DIFF_0_1);   //time = 0
      adc0.TriggerConversion();
      adc1.setMux(ADS1115_REG_CONFIG_MUX_DIFF_0_1);   
      adc1.TriggerConversion();
      
      Ch0.Raw = adc0.getLastConversionResults();       
      adc0.setMux(ADS1115_REG_CONFIG_MUX_DIFF_2_3);
      adc0.TriggerConversion();
      
      Ch2.Raw = adc1.getLastConversionResults();  
      adc1.setMux(ADS1115_REG_CONFIG_MUX_DIFF_2_3);
      adc1.TriggerConversion();
      
      Ch1.Raw = adc0.getLastConversionResults();                    
      Ch3.Raw = adc1.getLastConversionResults();
 
  ADCProcessing(&Ch0);
  ADCProcessing(&Ch1);
  ADCProcessing(&Ch2);
  ADCProcessing(&Ch3);

    //Std Dev Code Below
    /*
    sdValues[sdValuesPTR] = float(Ch0.mA);
   sdValuesPTR++;
   if (sdValuesPTR >= 50) {
      int i;
      float ftemp=0;
      float AVG;
      float VAR;
      float sd;
  
      //Calculate average
        for (i=0;i<=49;i++) {ftemp = ftemp + sdValues[i];}//Add them up
        AVG = ftemp / 50.0;
  
        Serial.print(F("SUM after= "));
        Serial.print(ftemp,4);
        Serial.println();
        Serial.print(F("AVG = "));
        Serial.print(AVG,4);
        Serial.print("mA");
        Serial.println();
        
      //Calculate Varience
        for (i=0;i<=49;i++) { //calculate varience
          VAR = VAR + pow(sdValues[i] - AVG,2);
        }
        
        Serial.print(F("VAR = "));
        Serial.print(VAR,4);
        Serial.print("mA");
        Serial.println();
      //Calculate std dev
        sd = pow(VAR/50 ,0.5);
        Serial.print(F("Std Dev = "));
        Serial.print(sd,6);
        Serial.print("mA");
        Serial.println();
        
    sdValuesPTR=0;
   }
   */
}




//==============================================================================================
void SerialMonitor() {
  //Edit this function to process all Serial commands.
  //This function should be called every scan as it will only do something when data with a LF char has been read from the serial port
  //  aka: the full command has been recieved
  
  //Create some temporary variables
      String aSerialStringParse[8]; //Create a small array to store the parsed strings 0-7
      int StringStart=0;
      int i;
  //Make Serial String all lower case:
      sLastSerialLine.toLowerCase()    ;    
  if (sLastSerialLine.equalsIgnoreCase("")==false) { //A full line has been read from the Serial buffer, do something!
    //This should be full of If..Then statemens from all of the different possible commands that can be read and processed over serial

    //Set the String parse 0th element to the number of CSVs recieved, and set the first element to the string
      aSerialStringParse[0]=1;
      aSerialStringParse[1]=sLastSerialLine;

      
    //--Search incoming data for a CSV command and modify the stringParseArray
        int CommaPosition = sLastSerialLine.indexOf(','); 
          if (CommaPosition>0) { //Comma separated values detected, do a string split

            //Perform a String split to a string Array and place the size in [0]
            for (i=1;i<=7;i++) { //Start at 1 and increment only 5 times
              aSerialStringParse[i] = sLastSerialLine.substring(StringStart,CommaPosition); //Get the string
              if (CommaPosition == -1) { //Check if its the last one
                aSerialStringParse[0] = i; //Set the 0th element to the length
                break;
               }
              StringStart = CommaPosition + 1; //Set the start position of the next search based on the end position of this search
              CommaPosition = sLastSerialLine.indexOf(',',StringStart); //Find the next comma
              if (i==7 && CommaPosition !=-1) {
                aSerialStringParse[0] = i; //Set the 0th element to the length
                Serial.println(F("Bad Syntax"));
                break;
              }
      
            }
            
          }

    //SERIAL COMMANDS

    /*
     Commands can be any text that you can trigger off of.  
     It can also be paramaterized via a CSV Format with no more than five(5) CSVs: 'Param1,Param2,Param3,Param4,Param5'
     For Example have the user type in '9,255' for a command to manually set pin 9 PWM to 255.
     */
    //--------Edit the Help menu
     if (aSerialStringParse[0].toInt() == 1) { //Process single string serial commands
          //MAIN MENU
             if (aSerialStringParse[1] == "?" ) {
                SerialPrintFile(F("Menu/_10_Main.txt"));
                Serial.print(F("Time: "));  Serial.println(getDateString()); 
              }
              
              //1. (c)onfig - channel config and calibration"));
                  if (aSerialStringParse[1] == "c") {
                    getInputs();
                    SerialPrintFile(F("Menu/_20_Cfg.txt"));
                    
                       Serial.print(F(" 0\t"));  
                    PrintChannelConfigHor(&Ch0);
                       Serial.print(F(" 1\t"));  
                    PrintChannelConfigHor(&Ch1);
                       Serial.print(F(" 2\t"));  
                    PrintChannelConfigHor(&Ch2);
                       Serial.print(F(" 3\t"));  
                    PrintChannelConfigHor(&Ch3);
                    Serial.println();
                    SerialPrintFile(F("Menu/_21_Cfg.txt"));
                  }
              //2. (f)ile managemant and data
                  if (aSerialStringParse[1] == "f") {SerialPrintFile(F("Menu/_30_File.txt"));} 

                  //File/dir
                      if (aSerialStringParse[1] == "dir") {
                        File DataFolder = SD.open(DataFolderLocation);
                          printDirectory(DataFolder);               
                      } 
          
                  //Deltree
                  
                        if (aSerialStringParse[1]=="deltree") {
                            SD.rmdir(DataFolderLocation);
                            SD.mkdir("Menu");
                        }
                        
               //Channel Config /Live
                  if (sLastSerialLine.equalsIgnoreCase("l")) {
                      if (livereadout) {
                         livereadout=0;
                      } else {
                         livereadout=1;                        
                      }
                  }
                  
              //3. (Future)"));
                 // if (sLastSerialLine.equalsIgnoreCase("t")) {Serial.print(F("Time: "));  Serial.println(getDateString()); } 
                 
      } else if(aSerialStringParse[0].toInt() > 1) {//Process multiple serial commands
                
            //MAIN/Time
                if (aSerialStringParse[1]=="t" && aSerialStringParse[0].toInt() == 7) {
                  if (aSerialStringParse[0].toInt() == 7) {
                    RtcDateTime t = RtcDateTime(aSerialStringParse[2].toInt(),aSerialStringParse[3].toInt(),aSerialStringParse[4].toInt(),aSerialStringParse[5].toInt(),aSerialStringParse[6].toInt(),aSerialStringParse[7].toInt());
                    RTC.SetDateTime(t);
                    Serial.print(F("Time: "));  Serial.println(getDateString()); 
                  } else {
                    Serial.println(F("Bad syntax"));
                  }
                }
            //File Print
                  if (aSerialStringParse[1]=="p") {
                      if(aSerialStringParse[0].toInt() == 2) {
                           File dataFile = SD.open(FileLocation(aSerialStringParse[2].toInt())); //Get the full file name of the selected file, then open it
                          if (dataFile) {
                              while (dataFile.available()) { Serial.write(dataFile.read());} //print entire file to Serial
                              dataFile.close();
                          }
                      } else {
                          Serial.println(F("Bad syntax"));
                      }
                  }
                  
            //Del File
            
                  if (aSerialStringParse[1]=="del") {
                      if(aSerialStringParse[0].toInt() == 2) {
                          SD.remove(FileLocation(aSerialStringParse[2].toInt())); //Get the full file name of the selected file, then delete it
                          Serial.println(F("file deleted"));
                          Serial.println(FileLocation(aSerialStringParse[2].toInt()));
                      } else {
                          Serial.println(F("Bad syntax"));
                      }
                  }
             
            //Channel configuration
                if (aSerialStringParse[1]=="c") {
                    if (aSerialStringParse[0].toInt() == 6 && aSerialStringParse[2].toInt() <=3) {
                        if (aSerialStringParse[5].toFloat()> 0) {
                            int ChNumber = aSerialStringParse[2].toInt();
                            int EEPROMAddress;
                            struct ADCChan  * Ch;
                            if (ChNumber == 0) {
                                Ch = &Ch0;
                                EEPROMAddress = 10;
                            } else if (ChNumber == 1) {   
                                Ch = &Ch1;          
                                EEPROMAddress = 26;     
                            } else if (ChNumber == 2) {
                                Ch = &Ch2;
                                EEPROMAddress = 42;    
                            } else if (ChNumber == 3) { 
                                Ch = &Ch3;       
                                EEPROMAddress = 58;      
                            }
                            //Update Channel cfg
                               Ch->ActualLo = aSerialStringParse[3].toFloat();
                               Ch->ActualHi = aSerialStringParse[4].toFloat();
                               Ch->Deadband = aSerialStringParse[5].toFloat();
                               Ch->Calib    = aSerialStringParse[6].toFloat();
                            //Update EEPROM
                                EEPROM.put(EEPROMAddress,Ch->Calib); EEPROMAddress = EEPROMAddress + 4;
                                EEPROM.put(EEPROMAddress,Ch->ActualLo); EEPROMAddress = EEPROMAddress + 4;
                                EEPROM.put(EEPROMAddress,Ch->ActualHi); EEPROMAddress = EEPROMAddress + 4;
                                EEPROM.put(EEPROMAddress,Ch->Deadband);
                                
                              getInputs(); //Update inputs with new config data
                     
                          Serial.print(F("Ch")); Serial.print(ChNumber); Serial.println(F(" Updated"));
                        } else { 
                          Serial.println(F("Deadband cannot be 0"));
                        }
                    } else {
                      Serial.println(F("Bad syntax"));
                    }
                }
      }
      
      sLastSerialLine = ""; //Clear out buffer, This should ALWAYS be the last line in this if..then
  }




             
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
//==============================================================================================
void EndProgram(String ErrorMessage ) {

  //An unrecoverable error occured
  //Loop forever and display the error message
  while(1) {
    Serial.print(F("Major Error: "));
    Serial.println(ErrorMessage);
    Serial.println(F("Unit is not recorded data.  Cycle power to restart (probably won't help)"));
    delay(5000);
  }
}


//==============================================================================================
void PrintChannelLive(struct ADCChan *Ch) {
  //Print Channel Config Horizontally

     Serial.print(Ch->mA,4); Serial.print("mA/"); Serial.print(Ch->Actual,Ch->Precision); Serial.print(F("\t"));
  }
//==============================================================================================
void PrintChannelConfigHor(struct ADCChan *Ch) {
  //Print Channel Config Horizontally
  Serial.print(Ch->mA,3);
  Serial.print("\t\t");
  Serial.print(Ch->Current,Ch->Precision);
  Serial.print("\t\t");
  Serial.print(Ch->Actual,Ch->Precision);
  Serial.print("\t\t");
  Serial.print(Ch->ActualLo,Ch->Precision);
  Serial.print("\t\t");
  Serial.print(Ch->ActualHi,Ch->Precision);
  Serial.print("\t\t");
  Serial.print(Ch->Deadband,Ch->Precision);
  Serial.print("\t\t");
  Serial.print(Ch->Calib,6);
  Serial.println("");

  }

//==============================================================================================

void printDirectory(File dir) {
  
  Serial.println(F("Name\t\tsize(bytes)\tEst print Time(sec)"));
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) { break;}// no more files
      //Serial.print('\t');
    Serial.print(entry.name());
    Serial.print("\t");
    unsigned long SIZE = entry.size();
    Serial.print(SIZE, DEC);
    Serial.print("\t\t");
    Serial.println(SIZE/20000);
    
    entry.close();
  }
}
