/*  
11/02/17  This code reads serial data:
It expects 15 strings which describe TEC temperature and laser current
This code also captures data from a camera and performs some motion detection
The code sends a command to the arduino to open a shutter for a given length of time.

Note*  The time element might not yet be read by the Arduino code, but the open or close command should be.
There is a manual shutter button which sends the "open" command to the Arduino
There some of code will automatically turn off the shutter is motion is detected.



This works with the Arduino Sketch called  Simple_motion_detection_developed




        
*/


/* Version 1.0 11 February 2017 
Inital working prototype code
Work needs to be done to handle the shutter open/close commands from processing sketch.
*/

import processing.video.*;
import processing.serial.*;
import controlP5.*;

Capture video;                       // Variable for capture device
Serial serialport;                   // Variable for serial port
ControlP5 Controller;                // Variable for some on screen controllers

String ArduinoString = ("no serial data found");  // This is the actual string coming from Arduino
String [] DataFromArduino = new String[16];        // This is an array to hold data from Arduino, it will come across as one String, and will be split into smaller strings, and converted to floats

//Here are the 15 strings which are sent from the microprocessor.  The first string is the word "start"
String Actemp = "empty";        //2
String MaxActTemp = "empty" ;   //3
String MinActTemp = "empty" ;   //4
String StdevActTemp = "empty" ; //5

String SetTemp = "empty";       //6
String MaxSetTemp = "empty" ;   //7
String MinSetTemp = "empty" ;   //8
String StdevSetTemp = "empty" ; //9

String ActCurrent = "empty";    //10
String MaxCurrent = "empty";    //11
String MinCurrent = "empty";    //12
String StdevCurrent = "empty";  //13

String Count = ("empty");        //14
String ShutterState = ("closed"); //15
String ShutterCountDownTime = "0" ; //16

float[] ImageArray = new float[640*480];  // An array to sum up all of the images and perfom an average
int averaging = 50;                       // Number of frames to average

int CSection;                        // A y coordinate definning a cross section

PImage prevFrame;                    // Previous Frame
PImage currentFrame;                 // Current Frame

PImage averagedFrame;
PImage differenceFrame; 


float threshold = 30;                // How different must a pixel be to be a "motion" pixel, this is set by a slider.
boolean motion ;                     // A boolean which decides if a frame has motion
int motionpixel ;                    // The number of pixels which are counted as motion pixels
int motionpixelthreshold = 1000 ;     // A threshold value, defining how many motion pixels there must be for a frame to be considered a "motion" frame

PFont MotionLabel;                    // Write some feedback to the bottom of the screen

int resetminute;                     // This is time zero for the beginning of a stable image
int stableminutes;                   // Number of minutes frame has been stable for
int resetsecond;
int stableseconds;

int ShutterTime;                    // Number of seconds to open shutter

void setup() {
  size(1280,750);                           // Create a window wide enough to see the video and the difference image
  printArray(Serial.list());                // Print the array of available serial ports
  printArray(Capture.list());
  video = new Capture(this, 640, 480, 30);                // Create a video object
  
  serialport = new Serial(this, Serial.list()[2], 9600);  // Create a serial object, the"2" assumes the arduino is number 2 in the serial array
  serialport.clear();                                     // Clear the serial port
  //serialport.bufferUntil(10);                             // Buffer until caradge return
    
  prevFrame = createImage(video.width,video.height,ALPHA);       // Create an empty image the same size as the video
  currentFrame = createImage(video.width,video.height,ALPHA);    // Create another image the same size as the array
  differenceFrame = createImage(video.width,video.height,ALPHA);
  image(differenceFrame,0,0);
  
  motion = true ;                                           // Set the boolean to true so that a new reference image is defined straight away

 MotionLabel = loadFont("ArialMT-48.vlw");                 // Load the font               
 textFont(MotionLabel, 24);                                // Set the font size
 fill(255,255,255);                                        // Set the font color

Controller = new ControlP5(this);                          // Create a controller object

Controller.addSlider("threshold")                         // This controller sets the threshold for a "motion" pixel
     .setPosition(20,690)
     .setWidth(400)
     .setRange(0,30)
     .setValue(30)
     ;

Controller.addSlider("motionpixelthreshold")              // This controller sets the number of pixels threshold for a "motion" frame
     .setPosition(20,710)
     .setWidth(400)
     .setRange(0,5000)
     .setValue(1000)
     ;

Controller.addSlider("averaging")              // This controller sets the number of frames to calculate for averaging
     .setPosition(20,730)
     .setWidth(400)
     .setRange(1,50)
     .setValue(10)
     ;

Controller.addSlider("CSection")              // This controller sets the number of frames to calculate for averaging
     .setPosition(20,670)
     .setWidth(400)
     .setRange(0,480)
     .setValue(300)
     ;

Controller.addSlider("ShutterTime")              // This controller sets the number of seconds to open the shutter
     .setPosition(20,650)
     .setWidth(400)
     .setRange(0,180)
     .setValue(30)
     ;
     
Controller.addButton("ShutterButton")
     .setValue(0)
     .setPosition(500,650)
     .setSize(100,30)
     ;    

serialport.bufferUntil(10);                               // Don't know why this is here but it is needed.
video.start();

}  // end of setup


void draw() {
 
FrameAveraging_prevFrame();        // This function is where the images are captured and possibly averaged
FrameAveraging_currentFrame();     // This function is where the images are captured and possibly averaged
DetectMotion();                    // This is the code which detects motion from the images

image(differenceFrame,0,0);        // Draw the difference frame on the screen
image(prevFrame,640,0);            // This draws an image to the screen called prevFrame, it is just a frame grab.

fill(0,0,0);           // Set the color to black then write over the text to re-set 
rect(0,480,640,280);   // Rectangle starts at x, y = 0 , 480  and is 640 long and 280 high
fill(255,0,255);       // This sets the font colour
WriteMotionInfo();     //Write motion info to window
WriteTitles();         // Write the titles to laser state
WriteData();           // Write the actual laser state this assumes data is updated
PlotCrossSection();    // Draw a line plot 
}  // End of draw loop



public void ShutterButton() {   // Responds to Shutter button press and sends the shutter time to arduino
      serialport.write("o"+ShutterTime);
      
      
      serialport.clear();
//println("Button pressed");
}



void serialEvent(Serial p) { 
//  ArduinoString = p.readStringUntil(10);
  ArduinoString = p.readString();
  DataFromArduino = split(ArduinoString, ' ');      // Space is the deliniator
  println(ArduinoString, " string length: ",DataFromArduino.length);

if (DataFromArduino.length == 16) {
SetTemp = (DataFromArduino[1]);
MaxSetTemp = (DataFromArduino[2]);
MinSetTemp = (DataFromArduino[3]);
StdevSetTemp = (DataFromArduino[4]);
  
Actemp = (DataFromArduino[5]);
MaxActTemp = (DataFromArduino[6]);
MinActTemp = (DataFromArduino[7]);
StdevActTemp = (DataFromArduino[8]);

ActCurrent = (DataFromArduino[9]);
MaxCurrent = (DataFromArduino[10]);
MinCurrent = (DataFromArduino[11]);
StdevCurrent  = (DataFromArduino[12]);
                                       // 13 is a count of readings contributing to the average
ShutterState = DataFromArduino[14];    // The Arduino will send a confirmation that the shutter has opened
ShutterCountDownTime = DataFromArduino[15];


}
} // End of Serial Event Function


void PlotCrossSection() {
fill(0,0,0);               // Make a black rectangle to refresh the plot
rect(640,480,640,280);
stroke(255,0,0);           // Give a colour to the plot
//int CSection = 300 ;       // Choose a cross section line and draw it
line(640,CSection,1280,CSection); 
for (int x = 0; x < video.width; x ++ ) {  
int pos = x + CSection*video.width; 
//println("image position = ",pos,"  pixel value = ", red(video.pixels[pos]));
line(640+x,750,640+x,750-(red(currentFrame.pixels[pos])));  
 
}  

//Draw some vertical lines to help with determining stability
stroke(0,0,255);
for (int i = 640; i < 1280; i = i+50) {
  line(i, 750, i, CSection);
}


} // End of Plot Function 



void WriteMotionInfo ()  {
 if (motionpixel > motionpixelthreshold) {motion = true;} else {motion = false;}
 
  if (motion == true) {
  text("Motion detected",10,510); 
  resetminute = minute() ;
  resetsecond = second() ; 
 // serialport.write(99);
 //  delay(100);
 //  serialport.clear();
 }
  
  else {
 text("No motion detected for ",10,510);
 stableminutes = minute() - resetminute;         
 text(stableminutes,255,510);
 text("mins",285,510);
 if (stableminutes < 1) { stableseconds = second() - resetsecond; text(stableseconds,345,510);}
 else {text(second(),345,510);}
 text("seconds",375,510);   
  //  serialport.write(111);
  //  delay(100);
  //  serialport.clear();
 }  
 
  
}  //End WriteMotion Info Function




void WriteTitles() {
  text("Set Point Temperature = ",10,545); text("+-",380,545);
  text("Actual Temperature    = ",10,575); text("+-",380,575);
  text("Laser Current         = ",10,605); text("+-",380,605);
  text("Shutter State         = ",10,635); 
  
} // End of Write Titles


void WriteData () {
  text(SetTemp,300,545);                   text(StdevSetTemp,410,545);
  text(Actemp,300,575);                    text(StdevActTemp,410,575);
  text(ActCurrent,300,605);                text(StdevCurrent,410,605);
  text(ShutterState,300,635);              text(ShutterCountDownTime,410,635);
} 



void DetectMotion() {  
  differenceFrame.loadPixels();
  motionpixel = 0 ;
  for (int x = 0; x < video.width*video.height; x ++ ) {
     
      float diff = abs( red(currentFrame.pixels[x]) - red(prevFrame.pixels[x]) );
 
      if (diff > threshold) {                   // If motion, display black
      differenceFrame.pixels[x] = color(0);
      motionpixel = motionpixel + 1;            // And increment motion by 1
      } 
      else {
      differenceFrame.pixels[x] = color(255);  // If not, display white
           }
  }   //end of loop calculating motion, the output is an integer of the number of motion pixels  
 differenceFrame.updatePixels();
} // End of detect motion  




void FrameAveraging_prevFrame() {
  
   if (video.available() == true) {
                                                                           // This loads an image from the camera to which other images will be added to.
    for (int i = 0; i < averaging; i++ ) {
    video.read();                                                          // This grabs a new image from the camera  
    
    prevFrame.copy(video,0,0,640,480,0,0,640,480);                         // This copies this to the PImage variable
    prevFrame.loadPixels();                                                // This loads the pixel array
    
    for (int j = 0; j<640*480; j++) {
    //currentFrame.loadPixels() ;
    ImageArray[j] = (ImageArray[j] + red(currentFrame.pixels[j]));         // This adds the pixel values from the current frame in the buffer.
    }
//println("After ",i," Steps: ","Current Frame ",red(currentFrame.pixels[1000]), " Image Array ",ImageArray[1000]);   
 }       

for (int n = 0; n < 480*640; n++) { 
ImageArray[n] = ImageArray[n]/averaging;     //This does the averaging
prevFrame.pixels[n] = color(int(ImageArray[n]),int(ImageArray[n]),int(ImageArray[n])); // This goes and updates all of the pixels
ImageArray[n] = 0;
}

//println("after division " ,red(prevFrame.pixels[1000]));
prevFrame.updatePixels();  // Update the pixels
println("captured prevFrame, ",averaging," frames were averaged");
} 
else println("no video found for prevFrame");  
} // End of averaging







void FrameAveraging_currentFrame() {
  
   if (video.available() == true) {
                                                                           // This loads an image from the camera to which other images will be added to.
    for (int i = 0; i < averaging; i++ ) {
    video.read();                                                          // This grabs a new image from the camera  
    
    currentFrame.copy(video,0,0,640,480,0,0,640,480);                         // This copies this to the PImage variable
    currentFrame.loadPixels();                                                // This loads the pixel array
    
    for (int j = 0; j<640*480; j++) {
    //currentFrame.loadPixels() ;
    ImageArray[j] = (ImageArray[j] + red(currentFrame.pixels[j]));         // This adds the pixel values from the current frame in the buffer.
    }
//println("After ",i," Steps: ","Current Frame ",red(currentFrame.pixels[1000]), " Image Array ",ImageArray[1000]);   
 }       

for (int n = 0; n < 480*640; n++) { 
ImageArray[n] = ImageArray[n]/averaging;     //This does the averaging
currentFrame.pixels[n] = color(int(ImageArray[n]),int(ImageArray[n]),int(ImageArray[n])); // This goes and updates all of the pixels
ImageArray[n] = 0;
}

//println("after division " ,red(prevFrame.pixels[1000]));
currentFrame.updatePixels(); // Update the pixels 
println("Captured video for currentFrame, ",averaging," frames were averaged");
} 
else println("no video for currentFrame"); 
} // End of averaging
