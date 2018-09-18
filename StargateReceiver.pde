import processing.net.*; 
import processing.serial.*;
import ddf.minim.*;

// Communicates with the stargate for receiving
// By D. Kopta

Client myClient; 
Serial serialPort;
 
 int numChevrons = 0;
 int nextChevron = 0;
 int chevronDistance = 0;
 boolean firstContact = false;
 
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
  
  
  // List the serial ports so we can hopefully tell which one we want
  //String[] ports = Serial.list();
  //for(int i=0; i < ports.length; i++)
  //  println(ports[i]);
  
  
  // My arduino is connected to the 6th serial port
  // TODO: Eric, change this to whatever port yours uses
  serialPort = new Serial(this, "COM7", 9600);
  delay(3000);
  serialPort.write('A');
  delay(10);
  serialPort.clear();
  
  
  // Connect to the server
  myClient = new Client(this, "lab2-6.eng.utah.edu", 2000); 
  myClient.write(1); // Tell the server I'm a receiver
  
  // Wait for acknowledgement that 2 gates are connected
  while(myClient.available() < 1)
   {delay(10);}
  if(myClient.read() != 255)
    {
      println("Error: did not receive correct acknowledgement from server");
      exit();
    }
    
    println("Receiver received 2-gate acknowledgement");
  
} 
 
void draw() {
  
  // Wait until the serial business has been sorted out
  //if(!firstContact)
  //  return;
 
 //println("did it");
  // Check if next chevron is available from the server
  if (myClient.available() > 0) 
  { 
   numChevrons++;
   
   // Read the chevron from the server
   nextChevron = myClient.read();
   println("Receiver: received chevron = " + nextChevron);
   
   // Tell the gate which symbol was sent
   serialPort.write(nextChevron);
   
   // The gate will tell us how far it had to move to get to that chevron
   while(serialPort.available() <= 0)
     {delay(10);}
     
   chevronDistance = serialPort.read();
   
   println("Gate reported distance: " + chevronDistance);
   playRotateSound(chevronDistance);
   
   delay(2000);
  }
   
  if(numChevrons == 7)
    {
      println("receiver exiting");
      wormhole.play();
      while(wormhole.isPlaying()) {delay(1);}
      myClient.stop();
      exit();
    }
  // change the color of the window for no reason
  //background(nextChevron);
 
} 

/*
// Use serialEvent just to establish first contact
void serialEvent(Serial myPort) {
  if(firstContact)
    return;
  // read a byte from the serial port:
  int inByte = myPort.read();
  // if this is the first byte received, and it's an A,
  // clear the serial buffer and note that you've
  // had first contact from the microcontroller. 
  if (firstContact == false) {
    if (inByte == 'A') { 
      myPort.clear();          // clear the serial port buffer
      firstContact = true;     // you've had first contact from the microcontroller
      myPort.write('A');       // acknowledge contact
    } 
  }
  /*
 else {
    
      numChevrons++;
      // print the values (for debugging purposes only):
      println("received byte " + inByte + ", numchevrons = " + numChevrons);
  
      chevronDistance = inByte;

      // Send a capital A to request new sensor readings:
      //myPort.write('A');
      // Reset serialCount:
      //serialCount = 0;
    }
    */
//} 


