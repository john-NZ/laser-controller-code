/*
Temperature (TEC) control and Current Control of Wavelength electronics TEC controller and Laser diode driver.

TEC driver is the WTC3243 IC with the WTC3293 evaluation board
Laser Diode driver is WLD3343 with WLD3393 evaluation board

Code was written originally for the Arduino Due, which has digital to Analogue Outputs. The Due runs at 3.3 V
An OpAmp circut was developed (Arduino Due shield) to better utalise the full range of the 3.3 Volts of the Due.
The max Output (in Volts) from the TEC controller is capped at 2 Volts,
So 2 * 1.65 = 3.3 Volts
The OPAmp gain is set to 1.65 


The C++ code was used with a Processing Sketch to see the values and the image from the camera on the screen.
The processing code is called Simple_motion_detection_developed_withAveraging

Hopefully the Processing Sketch can be replaced with a Web based GUI.

Hopefully the code can be adapted to use with the ESP32 board which also has a DAC which also runs at 3.3 V
The ESP board has WIFI so this might make using the Web page interface easier.

Functionality:

1. Reads the Analog input pins and converst these to Temperature and Laser current.
   - Read the Temperature Setpoint
   - Read the Actual Temperature
   - Read the Actual Laser Current
A statistics library was used to evaluate how stable the Temperature and Current reading were.

2. Shutter control:  This meant to be signal sent from a Processing Sketch, after evaluation of the diode stability (using a camera and Michelson interferometer)
It was never implimented.


*/

/* Version 1.0   11 June 2017
First working prototype of the code.
Some functionality could be broken out in to separate functions to simplfy reading of the code
*/


#include <Statistic.h>
Statistic ReadTempStats;
Statistic SetTempStats;
Statistic ReadCurrentStats;

float ReadTempAverage ;
float SetTempAverage ;
float ReadCurrentAverage ;

float ReadTempStdev ;
float SetTempStdev ;
float ReadCurrentStdev ;



String DataString ;    // Construct a string to print to the serial port

// Define which pins are taking which data streams
int ReadTempSetpointPin = 0;   // WE SETT (yellow wire) is sent to A0
int ReadActualTempPin = 1;     // WE ACTT (blue wire) is sent to   A1
int ReadActualCurrentPin = 2;  // WE IMON (green wire is sent to   A2

int shutter = 99;                   // A value which opens or closes a shutter, 99 is shut, 111 is open.
int diagnostics ;                   // a value which defines what is sent to the serial port
String ShutterState = ("Closed");

float TECgain = 1.65 ;        //The gain is set by the Op-Amp The Voltage from the WE board is amplified by the adaptor board
float lasergain = 1.00;    //The gain is set by the Op-Amp The Voltage from the WE (laser) board is amplified by the adaptor board




void setup() {

//Clear the Stataistics  
ReadTempStats.clear(); //explicitly start clean
SetTempStats.clear(); //explicitly start clean
ReadCurrentStats.clear(); //explicitly start clean

//This allows a range of different diagnotics to be printed to the serial port depending on what pin the diagnostic jumper is connected to.  
pinMode(53, INPUT);      // Signal defining what is printed to the serial port (Link to ground to print just three floating point values to serial port);
pinMode(50, INPUT);      // Signal defining what is printed to the serial port (Link to ground, to print full diagnostics to serial port); 
pinMode(51, INPUT);      // Signal defining what is printed to the serial port (Link to ground, to print limited diagnostics to serial port);

//This could be a signal out to a shutter, it has not yet been implimented (but can turn an LED off or on to see if it is working) 
pinMode(52, OUTPUT);     // this is the shutter signal output
delay(1000);
digitalWrite(52,LOW);    // set it to low initailly


Serial.begin(9600);      //open the serial port to get some information and debugging
analogReadResolution(12); // Set the read resolution to 12 bit to get more accuracy
delay(100);           



}

// End of setup up

void loop() {

// Read the state of some PINs 50, 51, 52, 53 and decide what is printed to the serial port
if (digitalRead(50) == 0) { diagnostics = 2 ; }  else if (digitalRead(51) == 0) { diagnostics = 1 ; } else if (digitalRead(51) == 0) { diagnostics = 3 ; } else { diagnostics = 0 ; }  

// Read some bytes to turn shutter on or off (later these can set the DAC)
if (Serial.available() > 0) {
shutter = Serial.read();
 }
 
//else { Serial.println("no bytes found in serial buffer");}

// 111 is ascii lowercase "o"            
if (shutter == 111) { 
   //Serial.println("shutter is open");                
   digitalWrite(52,HIGH);
   ShutterState = ("Open");
}
else {shutter = 99 ; }

// 99 is ascii lowercase "c"
if (shutter == 99) {
   //Serial.println("Shutter is closed");                 
   digitalWrite(52,LOW);
   ShutterState = ("Closed");
}




// First read the digital value from each pin using the read pin function and assign the avergae to the  digital value
float ReadTemp =    ReadAnalogPin(ReadActualTempPin);
float SetTemp  =    ReadAnalogPin(ReadTempSetpointPin);
float ReadCurrent = ReadAnalogPin(ReadActualCurrentPin);     
  
// Convert the digital values to Voltages, the resolution is 12-bit so max digital value is 4095
float ReadTempVoltage =    (ReadTemp * (3.3 / 4095));
float ReadSetPointVoltage = (SetTemp * (3.3 / 4095));
float ReadCurrentVoltage =   (ReadCurrent * (3.3 / 4095));
 
// Then take these digital values and calculate some temperatures in Celcius by calling the CalculateTemperature function
float ReadActualTemperature = CalculateTemperature (ReadTemp);
float ReadTemperatureSetPoint = CalculateTemperature (SetTemp);

// Calculating Laser Current in mAmps is easier than Temp, so a seperate function is not required
float ReadLaserCurrentmAmps = ((ReadCurrentVoltage * 100) / lasergain ) ;

// Calculate Stdev for these values
ReadTempStats.add(ReadActualTemperature);
SetTempStats.add(ReadTemperatureSetPoint);
ReadCurrentStats.add(ReadLaserCurrentmAmps);

ReadTempAverage = ReadTempStats.average();
SetTempAverage = SetTempStats.average();
ReadCurrentAverage = ReadCurrentStats.average();

ReadTempStdev = ReadTempStats.pop_stdev();
SetTempStdev = SetTempStats.pop_stdev();
ReadCurrentStdev = ReadCurrentStats.pop_stdev();

if (ReadTempStats.count() == 50)
  {
   ReadTempStats.clear();
   delay(1000);
  }

if (SetTempStats.count() == 50)
  {
   SetTempStats.clear();
   delay(1000);
  }

if (ReadCurrentStats.count() == 50)
  {
   ReadCurrentStats.clear();
   delay(1000);
  }




//Then print some values to the serial port
if (diagnostics == 0 )  {
//Serial.print(ReadActualTemperature,3); Serial.print(" "); Serial.print(ReadTemperatureSetPoint,3); Serial.print(" "); Serial.print(ReadLaserCurrentmAmps,3); Serial.print(" "); Serial.println(ShutterState);

// A string is constructed from 14 measurements
// 0. Start 1.SetTempAverage 2.MaxSetTempAverage 3.MinSetTempAverage 4.StdevSetTempAverage 5.ReadTempAverage 6.MaxReadTempAverage 7.MinReadTempAverage 8.StdevReadTempAverage 9.ReadCurrentAverage 10.MaxReadCurrentAverage 11.MinReadCurrentAverage 12.StdevReadCurrentAverage 13.Count 14.Shutter

DataString =("start " + String(SetTempAverage,3) + " " + String(SetTempStats.maximum(),3)+ " " + String(SetTempStats.minimum(),3)+ " " + String(SetTempStdev,3)+ " "+ String(ReadTempAverage,3) + " " + String(ReadTempStats.maximum(),3)+ " " + String(ReadTempStats.minimum(),3)+ " " + String(ReadTempStdev,3)+ " "+ String(ReadCurrentAverage,3) + " " + String(ReadCurrentStats.maximum(),3)+ " " + String(ReadCurrentStats.minimum(),3)+ " " + String(ReadCurrentStdev,3)+ " "+ReadTempStats.count()+ " "+ShutterState); 
Serial.println(DataString);
delay(100);
}


if (diagnostics == 1 )  {

Serial.print("Set Point Temperature is: "); Serial.print(ReadTemperatureSetPoint,3);    Serial.print(" Degrees"); Serial.print("  max is: "),Serial.print(SetTempStats.maximum(),3),Serial.print(" min is: "),Serial.print(SetTempStats.minimum()), Serial.print( "  Average is: "); Serial.print(SetTempAverage,3), Serial.print(" Stdev is: "); Serial.print(SetTempStdev,3); Serial.print(" Count is: "); Serial.println(SetTempStats.count());
Serial.print("Actual Temperature is:    "); Serial.print(ReadActualTemperature,3);      Serial.print(" Degrees"); Serial.print("  max is: "),Serial.print(ReadTempStats.maximum(),3),Serial.print(" min is: "),Serial.print(ReadTempStats.minimum()),Serial.print(" Average is: "); Serial.print(ReadTempAverage,3), Serial.print(" Stdev is: ");  Serial.print(ReadTempStdev,3); Serial.print(" Count is: "); Serial.println(ReadTempStats.count());
Serial.print("Laser Current is:         "); Serial.print(ReadLaserCurrentmAmps,3);      Serial.print(" mAmps  "); Serial.print("  max is: "),Serial.print(ReadCurrentStats.maximum(),3),Serial.print(" min is: "),Serial.print(ReadCurrentStats.minimum()), Serial.print(" Average is: "); Serial.print(ReadCurrentAverage,3), Serial.print(" Stdev is: ");  Serial.print(ReadCurrentStdev,3); Serial.print(" Count is: "); Serial.println(ReadCurrentStats.count());
Serial.println(" ");
// Read the state of some PINs 50, 51, 52, 53 and decide what is printed to the serial port
//Serial.print("PIN 50 is: "); Serial.println(digitalRead(50));
//Serial.print("PIN 51 is: "); Serial.println(digitalRead(51));
//Serial.print("PIN 53 is: "); Serial.println(digitalRead(53));
//Serial.print("diagnostics set to: "); Serial.println(diagnostics);
//Serial.print("Shutter value is "); Serial.println(shutter,DEC);

if (shutter == 99) {
   Serial.println("Shutter is closed");}                 
if (shutter == 111) {
   Serial.println("Shutter is open");} 
delay(500);
}

if (diagnostics == 2 )  {
Serial.print("Temp Set Point      "); Serial.print("  Digital count at A0 is: "); Serial.print(SetTemp);      Serial.print("   Voltage at A0 is: "); Serial.print(ReadSetPointVoltage,3);  Serial.print("   Voltage at SETT is: "); Serial.print((ReadSetPointVoltage)/TECgain,3);   Serial.print("   Set Point Temperature is: "); Serial.print(ReadTemperatureSetPoint,3); Serial.println(" Degrees");
Serial.print("Read Temp           "); Serial.print("  Digital count at A1 is: "); Serial.print(ReadTemp);     Serial.print("   Voltage at A1 is: "); Serial.print(ReadTempVoltage,3);      Serial.print("   Voltage at ACTT is: "); Serial.print((ReadTempVoltage)/TECgain,3);       Serial.print("             Temperature is: "); Serial.print(ReadActualTemperature,3);   Serial.println(" Degrees");
Serial.print("Read Laser Current  "); Serial.print("  Digital count at A2 is: ");  Serial.print(ReadCurrent); Serial.print("   Voltage at A2 is: "); Serial.print(ReadCurrentVoltage,3);   Serial.print("   Voltage at IMON is: "); Serial.print((ReadCurrentVoltage)/lasergain,3);  Serial.print("   Set Point Current is:     "); Serial.print(ReadLaserCurrentmAmps,3);   Serial.println(" mAmps");
Serial.println(" ");
// Read the state of some PINs 50, 51, 52, 53 and decide what is printed to the serial port
Serial.print("PIN 50 is: "); Serial.println(digitalRead(50)); 
Serial.print("PIN 51 is: "); Serial.println(digitalRead(51));
Serial.print("PIN 53 is: "); Serial.println(digitalRead(53));
Serial.print("diagnostics set to: "); Serial.println(diagnostics);
Serial.print("Shutter value is "); Serial.println(shutter,DEC);

if (shutter == 99) {
   Serial.println("Shutter is closed");}                 
if (shutter == 111) {
   Serial.println("Shutter is open");} 
}

}  // End of main loop



// Declare some functions


// Average some readings from Analog pins, this collects 10 readings from an Analog pin and averages to reduce the noise a bit. Just takes in the pin number and outputs a float
float ReadAnalogPin ( int AnalogPin ) {
float ReadpinAverage = 0 ;
for (int i = 0; i < 10 ; i++) 
  {
 int readpin = analogRead(AnalogPin);      
  ReadpinAverage = ReadpinAverage + readpin;
  delay(10);

}
ReadpinAverage = ReadpinAverage/10;
return ReadpinAverage ;
}

// Calculate Temperature
float CalculateTemperature ( float DigitalTemp ) {    

  float a = 0.003354017;          
  float b = 0.000256172;
  float c = 0.000002140094300;
  float d = -0.000000072405219000; 

float CalculatedVoltage = (DigitalTemp * (3.3 / 4095)/ TECgain);
float CalculatedResistance = (CalculatedVoltage/0.0001); 
float bTerm = b*log(CalculatedResistance/10000);
float cTerm = c* pow(log((CalculatedResistance/10000)),2);
float dTerm = d* pow(log((CalculatedResistance/10000)),3);
float CalculatedTemperature = 1/(a + bTerm + cTerm + dTerm) -273.15;

return CalculatedTemperature ;
}
