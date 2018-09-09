// the remote dialer for the stargate project

// NOTE: DO NOT upload this to your arduino. It uses the crystal pins for I/O
// This is intended for a stand alone micro with internal oscilator.

// The IR output is a 38kHz carrier with a protocol similar to NEC
// Each key press sends one byte.
// Pressing more than one key will result in nothing.
// The shift(fn) key modifies the other keys, but does not send a byte itself.

/* the keys are mapped thus
	A	B	C	D	E
0	7		0	1	4
1	8		,	2	5
2	9		.	3	6
3			la	+	x
4		fn			
5	del		da	ua	-
6	ent		ra	=	div

*/

// pin defs
// A and 0 are crystal pins to be handled seprately
#define Apin (1<<6)
#define ZEROpin (1<<7)
#define Bpin 4
#define Cpin 5
#define Dpin 6
#define Epin 2
#define ONEpin 3
#define TWOpin 7
#define THREEpin 1
#define FOURpin 8
#define FIVEpin 0
#define SIXpin 10
#define LEDpin 9

// for IR protocol
#define headerOn 9000
#define headerOff 4500
#define pulseLength 562
#define zeroBit 562
#define oneBit 1687

// keystates: 0xFF=nothing, msb high=fn pressed, 
// 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10=,, 11=., 12=la, 13=ua, 14=ra, 
// 15=da, 16=+, 17=-, 18=x, 19=div, 20==, 21=del, 22=ent
uint8_t keyState, previousState;

void setup(){
  // pins
  DDRB &= ~Apin;
  PORTB |= Apin;
  DDRB &= ~ZEROpin;
  PORTB |= ZEROpin;
  pinMode(Bpin, INPUT_PULLUP);
  pinMode(Cpin, INPUT_PULLUP);
  pinMode(Dpin, INPUT_PULLUP);
  pinMode(Epin, INPUT_PULLUP);
  pinMode(ONEpin, INPUT_PULLUP);
  pinMode(TWOpin, INPUT_PULLUP);
  pinMode(THREEpin, INPUT_PULLUP);
  pinMode(FOURpin, INPUT_PULLUP);
  pinMode(FIVEpin, INPUT_PULLUP);
  pinMode(SIXpin, INPUT_PULLUP);
  
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, LOW);
  
  // timer for output
  TCCR1A = 0;
  TCCR1B = (1<<WGM12)|(1<<CS10);
  OCR1AH = 0;
  OCR1AL = 104;
  OCR1AH = 0;
  OCR1AL = 104;
  
  keyState = 0xFF;
  previousState = 0xFF;
}

void loop(){
  // if a key is pressed, send the corresponding byte
  // loop delays 50ms for debouncing
  delay(50);
  
  readKeyState();
  
  if(keyState != previousState){
    if(keyState != 0xFF){
      // if fn key is the only one pressed, don't send
      if(keyState != 128){
        // send the byte
        irTransmit(keyState);
      }
    }
    previousState = keyState;
  }
}

/* the keys are mapped thus
	A	B	C	D	E
0	7		0	1	4
1	8		,	2	5
2	9		.	3	6
3			la	+	x
4		fn			
5	del		da	ua	-
6	ent		ra	=	div

keystates: 0xFF=nothing, msb high=fn pressed, 
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10=,, 11=., 12=la, 13=ua, 14=ra, 
15=da, 16=+, 17=-, 18=x, 19=div, 20==, 21=del, 22=ent

*/
void readKeyState(){
  // set each letter pin to output low one at a time
  // read number pins, set keyState
  uint8_t keyCount = 0;
  
  // A
  PORTB &= ~Apin;
  DDRB |= Apin;
  delay(1);
  if(!(PINB & ZEROpin)){
    keyCount++;
    keyState = 7;
  }
  if(!digitalRead(ONEpin)){
    keyCount++;
    keyState = 8;
  }
  if(!digitalRead(TWOpin)){
    keyCount++;
    keyState = 9;
  }
  if(!digitalRead(FIVEpin)){
    keyCount++;
    keyState = 21;
  }
  if(!digitalRead(SIXpin)){
    keyCount++;
    keyState = 22;
  }
  DDRB &= ~Apin;
  PORTB |= Apin;
  delay(1);
  // C
  digitalWrite(Cpin, LOW);
  pinMode(Cpin, OUTPUT);
  delay(1);
  if(!(PINB & ZEROpin)){
    keyCount++;
    keyState = 0;
  }
  if(!digitalRead(ONEpin)){
    keyCount++;
    keyState = 10;
  }
  if(!digitalRead(TWOpin)){
    keyCount++;
    keyState = 11;
  }
  if(!digitalRead(THREEpin)){
    keyCount++;
    keyState = 12;
  }
  if(!digitalRead(FIVEpin)){
    keyCount++;
    keyState = 15;
  }
  if(!digitalRead(SIXpin)){
    keyCount++;
    keyState = 14;
  }
  pinMode(Cpin, INPUT_PULLUP);
  delay(1);
  // D
  digitalWrite(Dpin, LOW);
  pinMode(Dpin, OUTPUT);
  delay(1);
  if(!(PINB & ZEROpin)){
    keyCount++;
    keyState = 1;
  }
  if(!digitalRead(ONEpin)){
    keyCount++;
    keyState = 2;
  }
  if(!digitalRead(TWOpin)){
    keyCount++;
    keyState = 3;
  }
  if(!digitalRead(THREEpin)){
    keyCount++;
    keyState = 16;
  }
  if(!digitalRead(FIVEpin)){
    keyCount++;
    keyState = 13;
  }
  if(!digitalRead(SIXpin)){
    keyCount++;
    keyState = 20;
  }
  pinMode(Dpin, INPUT_PULLUP);
  delay(1);
  // E
  digitalWrite(Epin, LOW);
  pinMode(Epin, OUTPUT);
  delay(1);
  if(!(PINB & ZEROpin)){
    keyCount++;
    keyState = 4;
  }
  if(!digitalRead(ONEpin)){
    keyCount++;
    keyState = 5;
  }
  if(!digitalRead(TWOpin)){
    keyCount++;
    keyState = 6;
  }
  if(!digitalRead(THREEpin)){
    keyCount++;
    keyState = 18;
  }
  if(!digitalRead(FIVEpin)){
    keyCount++;
    keyState = 17;
  }
  if(!digitalRead(SIXpin)){
    keyCount++;
    keyState = 19;
  }
  pinMode(Epin, INPUT_PULLUP);
  delay(1);
  
  // B
  digitalWrite(Bpin, LOW);
  pinMode(Bpin, OUTPUT);
  delay(1);
  if(!digitalRead(FOURpin)){
    keyState |= 128;
  }
  pinMode(Bpin, INPUT_PULLUP);
  delay(1);
  
  if(keyCount != 1){
    keyState = 0xFF;
  }
}

void irTransmit(uint8_t output){
  // header
  TCCR1A = (1<<COM1A0);
  delayMicroseconds(headerOn);
  TCCR1A = 0;
  digitalWrite(LEDpin, LOW);
  delayMicroseconds(headerOff);
  
  // data
  for(int8_t i=7; i>=0; i--){
    if(output & (1<<i)){
      TCCR1A = (1<<COM1A0);
      delayMicroseconds(pulseLength);
      TCCR1A = 0;
      digitalWrite(LEDpin, LOW);
      delayMicroseconds(oneBit);
    }else{
      TCCR1A = (1<<COM1A0);
      delayMicroseconds(pulseLength);
      TCCR1A = 0;
      digitalWrite(LEDpin, LOW);
      delayMicroseconds(zeroBit);
    }
  }
  
  // stop bit
  TCCR1A = (1<<COM1A0);
  delayMicroseconds(pulseLength);
  TCCR1A = 0;
  digitalWrite(LEDpin, LOW);
  delay(1);
}
