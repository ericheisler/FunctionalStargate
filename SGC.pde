import processing.net.*; 
import ddf.minim.*;

// Modes
int DIALING = 0;
int RECEIVING = 1;

// For connecting to the server
Client myClient; 

// Gate/dial state
int numChevrons = 0;
int nextChevron = 0;
int chevronDistance = 0;
int currentGatePosition = 0;
boolean spinDirection = true; // alternates between CW and CCW

// For playing sounds
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

// State for keeping track of the sounds currently playing
boolean soundPlaying = false;
int rotateTime;
int numLoops;
int nextGlyphDistance;

// Icons
PImage glyphs[];
PImage gate;
PImage blank;

// Various dimensions for drawing the glyphs
int glyphSize = 60;
int screensize = 900;
int glyphsWidth = (4 * (glyphSize + 2) + 2);   // 4 columns (2 pixels for boder and gap)
int glyphsHeight = (10 * (glyphSize + 2) + 2); // 10 rows
int pushLeft = screensize - glyphsWidth - 10;
int pushDown = 20;
int gateWidth = pushLeft - 50;

// Number of frames to animate the dialed glyphs
int numSteps = 50;

// Step distance per frame for animation
float dx;
float dy;

// Animation state
float currentGlyphX;
float currentGlyphY;
int destinationX;
int destinationY;
int frameNum = -1;
int drawingGlyph;

// Location of the large glyph displayed in the middle of the gate
int startCornerX = 25 + (gateWidth / 4);
int startCornerY = 25 + (gateWidth / 4);


//int mode = DIALING;
int mode = RECEIVING;


void setup() {
  
  // initialize sound module
  minim = new Minim(this);

  startup = minim.loadFile("beginRotate.wav");
  chevron = minim.loadFile("chevron.wav");
  rotate = minim.loadFile("rotate.wav");
  wormhole = minim.loadFile("wormhole.wav");

  size(screensize, screensize - 210);
  background(255);
  glyphs = new PImage[39];

  // Load glyphs
  for (int i=0; i < 39; i++)
  {
    glyphs[i] = loadImage("symbols/" + (i + 1) + ".png");

    // Replace any white pixels with alpha transparency
    glyphs[i].loadPixels();
    for (int j = 0; j < glyphs[i].width * glyphs[i].height; j++) 
    { 

      // Some of the glyphs aren't exacltly black/white. Handle the gray pixles
      if (red(glyphs[i].pixels[j]) > 50 && 
        green(glyphs[i].pixels[j]) > 50 && 
        blue(glyphs[i].pixels[j]) > 50)
      {
        glyphs[i].pixels[j] = color(255, 255, 255, 1);
      }
    }
    glyphs[i].updatePixels();
  }

  gate = loadImage("MilkyWayGate.png");
  blank = loadImage("blank.png"); // for clearing old frames

  drawGlyphs();
  image(gate, 25, 25, gateWidth, gateWidth);


  // Change this to a real server's IP or name
   myClient = new Client(this, "server-ip", 2000);
   
   if (mode == DIALING) 
     myClient.write(0); // Tell the server I'm a dialer
   else
     myClient.write(1); // Tell the server I'm a receiver

    // Wait for acknowledgement from server that 2 gates are connected
    while (myClient.available () < 1)
    {
      delay(10);
    }
    if (myClient.read() != 255)
    {
      println("Error: did not receive correct acknowledgement from server");
      exit();
    }
    
    println("Dialer received 2-gate acknowledgement");
  
}

// Draws the glyphs and the borders around them
void drawGlyphs()
{
  stroke(0, 0, 0);
  for (int i=0; i < 39; i++)
  {
    int xoff = glyphIDToXPos(i);
    int yoff = glyphIDToYPos(i);
    rect(xoff, yoff, glyphSize, glyphSize);
    image(glyphs[i], xoff + 1, yoff + 1, glyphSize - 2, glyphSize - 2); 
  }
}

// Returns the Y-coordinate of the pixel corner of the given glyph
int glyphIDToYPos(int id)
{
  return (2 + (id / 4) * (glyphSize + 2) + pushDown);
}

// Returns the X-coordinate of the pixel corner of the given glyph
int glyphIDToXPos(int id)
{
  return (2 + (id % 4) * (glyphSize + 2) + pushLeft);
}

void draw() {
  
  // Check if it's time to stop any playing sounds
  manageSounds();
  
  // If in receiving mode, check if the next glyph has been dialed
  if(mode == RECEIVING && millis() > 3000)
    readGlyph();

  // Don't update anything if there's no animation in progress
  if (frameNum == -1)
  {
    if(numChevrons == 7) // all chevrons locked in
    {
      wormhole.play();
      numChevrons = -1; // reset so it doesn't keep playing the sound
    }
    return;
  }

  // Leave the symbol up while the gate spins  
  if (frameNum == 1)
    delay(millisecondsPerGlyph * nextGlyphDistance);


  // Animate the dialed glyph to shrink down and move back to its position
  
  // Compute the intermediate size for this frame
  float fullSize = float((gateWidth / 2) + 2);
  float fullScale = (fullSize / (glyphSize - 2)) - 1;
  float scale = float((numSteps - frameNum)) / float(numSteps);
  scale *= fullScale;
  int newSize = int((glyphSize - 2) * (scale + 1));

  // Erase the image from last frame
  if (frameNum > 0)
  {   
    image(blank, currentGlyphX - dx, currentGlyphY - dy, newSize, newSize);
  }

  currentGlyphX += dx;
  currentGlyphY += dy;

  // Handle any float conversion errors
  if (frameNum == numSteps)
  {
    newSize = glyphSize - 2;
    currentGlyphX = destinationX;
    currentGlyphY = destinationY;
  }


  // Redraw the gate and glyphs since they may have been drawn over
  image(gate, 25, 25, gateWidth, gateWidth);
  drawGlyphs();

  // Draw the current frame of the animated glyph
  image(glyphs[drawingGlyph], currentGlyphX, currentGlyphY, newSize, newSize);

  frameNum++;

  if (frameNum == numSteps)
  {
    // Make it red once it's been dialed
    glyphs[drawingGlyph].loadPixels();
    for (int i = 0; i < glyphs[drawingGlyph].width * glyphs[drawingGlyph].height; i++) 
    {   
      if (alpha(glyphs[drawingGlyph].pixels[i]) < 1)
        continue;

      // Some of the glyphs aren't exacltly black/white. Handle the gray pixles
      if (red(glyphs[drawingGlyph].pixels[i]) < 50 && 
        green(glyphs[drawingGlyph].pixels[i]) < 50 && 
        blue(glyphs[drawingGlyph].pixels[i]) < 50)
      {
        glyphs[drawingGlyph].pixels[i] = color(255, 0, 0);
      }
    }
    glyphs[drawingGlyph].updatePixels();
  }

  // Set state to not drawing
  if (frameNum > numSteps)
  {
    frameNum = -1;
  }
  
}

// Turn the glyphs in to buttons so we can click them to dial
void mouseClicked()
{ 
  if(mode != DIALING)
    return;

  int row = (mouseY - (pushDown + 2)) / (glyphSize + 4);
  int col = (mouseX - (pushLeft + 2)) / (glyphSize + 4);

  if (row < 0 || row > 9)
    return;
  if (col < 0 || col > 3)
    return;

  // Find which glyph was clicked
  int glyphID = (row * 4) + col;

  // Find the corner pixel of that glyph
  int xpos = glyphIDToXPos(glyphID);
  int ypos = glyphIDToYPos(glyphID);

  // Send dialed glyph to other gate
  // Gate on other end expects 1-based indexing
  sendGlyph(glyphID + 1);

  // Set the state that will animate the glyph and play the sound during draw()
  moveGlyph(glyphID, xpos, ypos, 0, 1);
  nextGlyphDistance = distanceToGlyph(glyphID);
  currentGatePosition = glyphID;
  //println("distance = " + nextGlyphDistance);
  playRotateSound(nextGlyphDistance);
  spinDirection = !spinDirection;
  
  numChevrons++;
  
}


// Set the state for draw() to animate the glyph
void moveGlyph(int glyphID, int x, int y, float angle, float scale)
{
  // offset by 1 to account for border 
  destinationX = x + 1;
  destinationY = y + 1; 


  dx = float((destinationX - startCornerX)) / float(numSteps);
  dy = float((destinationY - startCornerY)) / float(numSteps);

  currentGlyphX = float(startCornerX);
  currentGlyphY = float(startCornerY);

  frameNum = 0;

  drawingGlyph = glyphID;
}

// Start playing the rotation sound
void playRotateSound(int distance) 
{ 

  startup.rewind();
  rotate.rewind();
  chevron.rewind();

  // The rotation duration after the first loop of the wav file
  rotateTime = (distance * millisecondsPerGlyph);

  soundPlaying = true;

  // Play the startup sound
  startup.play();

  // Play the rotation sound
  numLoops = 0;
  if (rotateTime > rotate.length())
  {
    numLoops = 1;
    // We only really care about when we need to stop the sound after the first loop
    rotateTime -= rotate.length();
  }
  rotate.loop(numLoops);
}

// Non-blocking way of stoping the sounds at the right time.
// Called from draw()
void manageSounds()
{
  if (!soundPlaying)
    return;

  // If the rotation sound needs to loop more, just let it continue
  if (rotate.loopCount() > 0)
    return;

  // Otherwise we may need to stop it before its end
  if (rotate.position() >= rotateTime)
  {
    // Start playing the chevron sound before pausing the rotation sound, makes it sound smoother.
    chevron.play();
    rotate.pause();
    soundPlaying = false;
  }
}


// DIALING mode
// Sends a dialed glyph to the other gate via the server
void sendGlyph(int glyphID)
{
// Tell the server which chevron was dialed
    myClient.write(glyphID);
 
    // Wait for acknowledgement from server
    while(myClient.available() < 1)
     {delay(10);}
    int returnedChevron = myClient.read(); 
    if(returnedChevron != glyphID)
    {
      // Don't exit, just print the error
      println("Error: Server acknowledged different chevron: " + returnedChevron);
    }
    else
    {
      println("Server acknowledges receipt of chevron: " + returnedChevron);
    }
}

// RECEIVING mode
// Checks if a glyph has been sent from the other gate
void readGlyph()
{
  // Reuse frameNum as an indicator that the gate is alread spinning, don't read the next glyph yet.
  if(frameNum != -1)
  {
    return;
  } 
  
  delay(1000);
  
  // Check if next glyph is available from the server
  if (myClient.available() > 0) 
  { 
   numChevrons++;
   
   // Read the glyph from the server
   // Translate to 0-based indexing
   int glyphID = myClient.read() - 1;
   //println("Receiver: received chevron = " + glyphID);
   
   // Find the pixel corner of the dialed glyph
   int xpos = glyphIDToXPos(glyphID);
   int ypos = glyphIDToYPos(glyphID);

  // Set the state that will animate the glyph and play the sound during draw()
   moveGlyph(glyphID, xpos, ypos, 0, 1);
   nextGlyphDistance = distanceToGlyph(glyphID);
   currentGatePosition = glyphID;
   //println("distance = " + nextGlyphDistance);
   playRotateSound(nextGlyphDistance);
   
   spinDirection = !spinDirection;
   
  }
}


// Calculate the distance the gate must rotate to put the next glyph at the top
int distanceToGlyph(int glyphID)
{ 
  
  int diff = spinDirection ? (currentGatePosition - glyphID) : (glyphID - currentGatePosition);
  
  if(diff < 0)
    return 39 + diff;
 
 return diff;
 
}


