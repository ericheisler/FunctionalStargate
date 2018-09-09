import processing.net.*; 
import processing.serial.*;
import ddf.minim.*;
Client myClient; 
Serial serialPort;
int dataIn; 
int nextChevron;
int chevronDistance;
int numChevrons = 0;
 
Minim minim;
AudioPlayer startup;
AudioPlayer chevron;
AudioPlayer rotate;
AudioPlayer wormhole;

// Amount of time it takes the gate to rotate one unit
int millisecondsPerGlyph = 600;

// Amount of time before the end of one sound to start playing the next
// This helps get rid of the abruptness of switching sounds
int mergeSoundDelay = 1;

// Function to play the rotation sound, and delay until it's done.
 void playRotateSound(int distance) { 

  startup.rewind();
  rotate.rewind();
  chevron.rewind();
  
  // The startup sound makes up some of the rotation time, subtract it out.
  int rotateTime = (distance * millisecondsPerGlyph);/* - (startup.length() - mergeSoundDelay);*/
  
  startup.play();
  // Wait until the startup is almost done playing
  //while(startup.isPlaying() && (startup.position() < (startup.length() - mergeSoundDelay))){delay(1);}
  
  // Play the rotation sound
  int numLoops = 0;
  if(rotateTime > rotate.length())
    numLoops = 1;
  rotate.loop(numLoops);
  
  // Wait until the rotation sound has played long enough.
  // Wait for first loop to finish
  while(rotate.loopCount() > 0) {delay(10);}
  
  // If we had to loop, compute the remaining time before cutting off the 2nd iteration
  rotateTime -= (numLoops * rotate.length());
  
  while(rotate.isPlaying() && (rotate.position() < (rotateTime - mergeSoundDelay))){delay(1);}
  
  // Start playing the chevron sound before pausing the rotation sound, makes it sound smoother.
  chevron.play();
  rotate.pause();
  
} 
 
 
 
void setup() {  
  
  // initialize sound module
  minim = new Minim(this);
  
  startup = minim.loadFile("/Users/Bilbo/Desktop/stargatesounds/beginRotate.wav");
  chevron = minim.loadFile("/Users/Bilbo/Desktop/stargatesounds/chevron2.wav");
  rotate = minim.loadFile("/Users/Bilbo/Desktop/stargatesounds/rotate.wav");
  wormhole = minim.loadFile("/Users/Bilbo/Desktop/stargatesounds/Activate.wav");
  
  // Connect to the local machine at port 5204.
  // This example will not run if you haven't
  // previously started a server on this port.
  
  // List the serial ports so we can hopefully tell which one we want
  //println(Serial.list());
  
  // My arduino is connected to the 6th serial port
  // TODO: Eric, change this to whatever port yours uses
  serialPort = new Serial(this, "COM7", 9600);
  serialPort.clear();
  
  myClient = new Client(this, "lab2-6.eng.utah.edu", 2000); 
  myClient.write(0); // Tell the server I'm a dialer
  
  // Wait for acknowledgement from server that 2 gates are connected
  while(myClient.available() < 1)
   {delay(10);}
  if(myClient.read() != 255)
    {
      println("Error: did not receive correct acknowledgement from server");
      exit();
    }
  
  println("Dialer received 2-gate acknowledgement");
  
} 
 
void draw() { 
  
  // Check if next chevron/distance pair is available
  if(serialPort.available() >= 2)
  {
    numChevrons++;
    
    nextChevron = serialPort.read();
    chevronDistance = serialPort.read();
    
    // TODO: Play a sound/show a symbol based on the chevron and distance
    // Just print them for now
    println("Dialer: Next chevron = " + nextChevron);
    println("Dialer: distance = " + chevronDistance);
    
    // Tell the server which chevron was dialed
    myClient.write(nextChevron);
    
    // Start playing the gate sound
    playRotateSound(chevronDistance);
    
    
    
    
    // Wait for acknowledgement from server (just for testing, this will go away)
    while(myClient.available() < 1)
     {delay(10);}
    int returnedChevron = myClient.read(); 
    if(returnedChevron != nextChevron)
    {
      // Don't exit, just print the error
      println("Error: Server acknowledged different chevron: " + returnedChevron);
    }
    else
    {
      println("Server acknowledges receipt of chevron: " + returnedChevron);
    }
    
    // Force a pause between dials
    delay(2000);
    
  }
  else
  {
    delay(2);
  }
  if(numChevrons == 7)
    {
      println("dialer exiting");
      wormhole.play();
      while(wormhole.isPlaying()) {delay(1);}
      exit();
    }
    
  // change the color of the window for no reason
  //background(nextChevron); 
}
