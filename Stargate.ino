/*
The stargate controller. Handles the following:
- the seven chevron LEDs
- the motor for the rotating ring
- IR input from the remote dialer
- serial comm with a corresponding processing sketch

NOTE - this will not run as is on an arduino because it uses the crystal pins as output.
To use an arduino, modify the pin assignment and all the other code associated with them.

*/

// roughly 2336 steps per revolution, 60 steps per symbol
#define fullCircleSteps 2336
#define symbolSteps 60
#define chevronDelay 1300 // the length of the chevron engage audio clip
#define motorDelay 10 // determines the speed of the motor
#define bitthreshold 1500 // for IR protocol
#define startthreshold 10000 // for IR protocol

// pins
const uint8_t e1 = (1<<6); // winding 1
const uint8_t p1[2] = {(1<<7), 5};
const uint8_t e2 = 8; // winding 2
const uint8_t p2[2] = {6, 7};
const uint8_t chevrons[7] = {19, 18, 17, 16, 15, 14, 4};
const uint8_t irInput = 2;

unsigned long delayTime;

// for IR
volatile char irStartReceive;
volatile unsigned long irStartTime;
unsigned long irStopTime, irTimeout;
uint8_t irInputBuffer[8] = {0,0,0,0,0,0,0,0};
uint8_t irBufferIndex;

// for motion
uint8_t engagedChevrons;
uint8_t currentSymbol;
int8_t lastDirection;
int position; // in motor steps
uint8_t state; // 0=+off 1=+- 2=off- 3=-- 4=-off 5=-+ 6=off+ 7=++

ISR(INT0_vect){
	if(!irStartReceive){
		irStartReceive = true;
		irStartTime = micros();
	}
}

void setup(){
  // IR
  irStartReceive = false;
  irBufferIndex = 0;
  pinMode(irInput, INPUT_PULLUP);
  EICRA |= (1<<ISC01);
  EIMSK |= (1<<INT0);
  sei();
  
  // chevron pins
  pinMode(chevrons[0], OUTPUT);
  pinMode(chevrons[1], OUTPUT);
  pinMode(chevrons[2], OUTPUT);
  pinMode(chevrons[3], OUTPUT);
  pinMode(chevrons[4], OUTPUT);
  pinMode(chevrons[5], OUTPUT);
  pinMode(chevrons[6], OUTPUT);
  digitalWrite(chevrons[0], LOW);
  digitalWrite(chevrons[1], LOW);
  digitalWrite(chevrons[2], LOW);
  digitalWrite(chevrons[3], LOW);
  digitalWrite(chevrons[4], LOW);
  digitalWrite(chevrons[5], LOW);
  digitalWrite(chevrons[6], LOW);
  
  // motor pins
  DDRB |= e1;
  pinMode(e2, OUTPUT);
  PORTB &= ~e1;
  digitalWrite(e2, LOW);
  DDRB |= p1[0];
  pinMode(p1[1], OUTPUT);
  pinMode(p2[0], OUTPUT);
  pinMode(p2[1], OUTPUT);
  
  // put motor in state 0 (+off)
  PORTB |= p1[0];
  digitalWrite(p1[1], LOW);
  PORTB |= e1;
  
  position = 0;
  currentSymbol = 1;
  lastDirection = -1;
  engagedChevrons = 0;
  
  // Start serial and wait for ack
  Serial.begin(9600);
  delay(1500); // let the serial pass through(arduino) get ready
  while (Serial.available() <= 0) {
    Serial.print('A');   // send a capital A
    delay(300);
  }
  Serial.read(); // clear the ack
}

void loop(){
  // listen for input from both IR and serial. 
  // If IR comes, enter dialing mode.
  // If serial comes, enter incoming mode.
  if(irStartReceive){
    irReceive();
    dialingMode();
  }
  if(Serial.available() > 0){
    incomingMode();
  }
}

void dialingMode(){
  // The input from the dialer comes one byte at a time.
  // Multi-digit numbers are received MSD first.
  // After a number is received it must be followed by an "enter" key.
  // Numbers can be sent at any time as long as there is room in the buffer.
  // The program will wait until 7 chevrons have been entered.
  // Errors will cause the chevrons to disengage and this will return.
  //
  // When a valid number is received, it is sent to serial
  // and is followed the number of symbols that must be traversed
  
  // we already have at least one byte in the buffer
  uint8_t encoded = 0; // will become 1 when an "enter" byte is received
  uint8_t in = 0; // the number of the next symbol to go to. 1-39
  engagedChevrons = 0; // continue till this is 7
  uint8_t toTraverse = 0;
  
  while(engagedChevrons < 7){
    encoded = 0;
    in = 0;
    while(encoded == 0){
      if(irStartReceive){
         irReceive();
      }
      if(irBufferIndex > 0){
        if(irInputBuffer[irBufferIndex-1] == 22){
          // the number complete command
          encoded = 1;
        }else if(irInputBuffer[irBufferIndex-1] == 21){
          // the shutdown command
          encoded = 1;
          in = 255;
        }else{
          in = in*10 + irInputBuffer[irBufferIndex-1];
        }
        irBufferIndex--;
      }
    }
    if(in > 0 && in < 40){
      // now would be the time to send the byte via serial
      Serial.write(in);
      lastDirection = -lastDirection;
      if(lastDirection > 0){
        // clockwise
        if(currentSymbol >= in){
          toTraverse = currentSymbol - in;
        }else{
          toTraverse = 39 - in + currentSymbol;
        }
      }else{
        // ccw
        if(currentSymbol <= in){
          toTraverse = in - currentSymbol;
        }else{
          toTraverse = 39 + in - currentSymbol;
        }
      }
      Serial.write(toTraverse);
      
      while(currentSymbol != in){
        rotateOneSymbol(lastDirection);
      }
      engagedChevrons++;
      digitalWrite(chevrons[3], HIGH);
      digitalWrite(chevrons[(3+engagedChevrons)%7], HIGH);
      delayTime = millis();
      while(millis() - delayTime < chevronDelay){
        if(irStartReceive){
          irReceive();
        }
      }
      if(engagedChevrons < 7){
        digitalWrite(chevrons[3], LOW);
      }
    }else{
      // there was a dialing error
      engagedChevrons = 0;
      digitalWrite(chevrons[0], LOW);
      digitalWrite(chevrons[1], LOW);
      digitalWrite(chevrons[2], LOW);
      digitalWrite(chevrons[3], LOW);
      digitalWrite(chevrons[4], LOW);
      digitalWrite(chevrons[5], LOW);
      digitalWrite(chevrons[6], LOW);
      irBufferIndex = 0; // effectively clear the IR buffer
      return;
    }
  }
}

void incomingMode(){
  // similar to dialing mode, but via serial
  // receive one byte containing the number ofthe next symbol
  // send one byte containing the number of symbols that must be traversed
  // This will not return until all 7 chevrons are finished or there was an error
  
  uint8_t in = 0; // the number of the next symbol 1-39
  engagedChevrons = 0;
  uint8_t toTraverse = 0;
  
  while(engagedChevrons < 7){
    while(Serial.available() < 1); // wait
    in = Serial.read();
    if(in > 0 && in < 40){
      lastDirection = -lastDirection;
      if(lastDirection > 0){
        // clockwise
        if(currentSymbol >= in){
          toTraverse = currentSymbol - in;
        }else{
          toTraverse = 39 - in + currentSymbol;
        }
      }else{
        // ccw
        if(currentSymbol <= in){
          toTraverse = in - currentSymbol;
        }else{
          toTraverse = 39 + in - currentSymbol;
        }
      }
      Serial.write(toTraverse);
      while(currentSymbol != in){
        rotateOneSymbol(lastDirection);
      }
      engagedChevrons++;
      digitalWrite(chevrons[3], HIGH);
      digitalWrite(chevrons[(3+engagedChevrons)%7], HIGH);
      delayTime = millis();
      while(millis() - delayTime < chevronDelay){
        if(irStartReceive){
          irReceive();
        }
      }
      if(engagedChevrons < 7){
        digitalWrite(chevrons[3], LOW);
      }
    }else{
      // there was an error
      engagedChevrons = 0;
      digitalWrite(chevrons[0], LOW);
      digitalWrite(chevrons[1], LOW);
      digitalWrite(chevrons[2], LOW);
      digitalWrite(chevrons[3], LOW);
      digitalWrite(chevrons[4], LOW);
      digitalWrite(chevrons[5], LOW);
      digitalWrite(chevrons[6], LOW);
      return;
    }
  }
}

void rotateOneSymbol(int8_t dir){
  uint8_t steps = symbolSteps;
  while(steps){
    steps--;
    oneStep(dir);
  }
  if(dir > 0 && currentSymbol == 1){
    currentSymbol = 39;
  }else if(dir < 0 && currentSymbol == 39){
    currentSymbol = 1;
  }else{
    currentSymbol -= dir;
  }
}

void oneStep(int8_t dir){
  // make one step in direction dir
  // then delay
  // 0=+off 1=+- 2=off- 3=-- 4=-off 5=-+ 6=off+ 7=++
  if(dir > 0){
    position++;
    if(state == 0){
      digitalWrite(p2[0], LOW);
      digitalWrite(p2[1], HIGH);
      digitalWrite(e2, HIGH);
      state = 1;
    }
    else if(state == 1){
      PORTB &= ~e1;
      state = 2;
    }
    else if(state == 2){
      PORTB &= ~p1[0];
      digitalWrite(p1[1], HIGH);
      PORTB |= e1;
      state = 3;
    }
    else if(state ==3){
      digitalWrite(e2, LOW);
      state = 4;
    }
    else if(state ==4){
      digitalWrite(p2[0], HIGH);
      digitalWrite(p2[1], LOW);
      digitalWrite(e2, HIGH);
      state = 5;
    }
    else if(state ==5){
      PORTB &= ~e1;
      state = 6;
    }
    else if(state ==6){
      PORTB |= p1[0];
      digitalWrite(p1[1], LOW);
      PORTB |= e1;
      state = 7;
    }
    else if(state ==7){
      digitalWrite(e2, LOW);
      state = 0;
    }
  }
  else{
    // 0=+off 1=+- 2=off- 3=-- 4=-off 5=-+ 6=off+ 7=++
    position--;
    if(state ==0){
      digitalWrite(p2[0], HIGH);
      digitalWrite(p2[1], LOW);
      digitalWrite(e2, HIGH);
      state = 7;
    }
    else if(state ==1){
      digitalWrite(e2, LOW);
      state = 0;
    }
    else if(state ==2){
      PORTB |= p1[0];
      digitalWrite(p1[1], LOW);
      PORTB |= e1;
      state = 1;
    }
    else if(state ==3){
      PORTB &= ~e1;
      state = 2;
    }
    else if(state ==4){
      digitalWrite(p2[0], LOW);
      digitalWrite(p2[1], HIGH);
      digitalWrite(e2, HIGH);
      state = 3;
    }
    else if(state ==5){
      digitalWrite(e2, LOW);
      state = 4;
    }
    else if(state ==6){
      PORTB &= ~p1[0];
      digitalWrite(p1[1], HIGH);
      PORTB |= e1;
      state = 5;
    }
    else if(state ==7){
      PORTB &= ~e1;
      state = 6;
    }
  }
  //delay(motorDelay);
  delayTime = millis();
  while(millis() - delayTime < motorDelay){
    if(irStartReceive){
      irReceive();
    }
  }
    
}

// receives IR input
// returns 0 = no input, 1 = received,
// 2 = noise, 3 = no rise(stuck)
// 4 = long header(bad timing), 5 = incomplete byte
uint8_t irReceive(){
	// check for incoming signal
	if(irStartReceive){
		// start time has already been set
		// wait for 400us for ???
		delayMicroseconds(400);
		if((PIND&(1<<2)) > 0){
			// it was noise
			irStartReceive = false;
			return 2;
		}
	
		// check the start interval length
		irTimeout = millis();
		while(((PIND&(1<<2)) == 0)){ // wait for rise
			if(millis()-irTimeout > 200){
				irStartReceive = false;
				return 3;
			}
		}
		irTimeout = millis();
		while(((PIND&(1<<2)) > 0)){ // wait for fall
			if(millis()-irTimeout > 200){
				irStartReceive = false;
				return 4;
			}
		}
		irStartTime = micros();
		
		// receive a byte
		if(!irReceiveByte()){
                  irWaitForStopBit();
		  irStartReceive = false;
		  return 5;
                }
		irWaitForStopBit();
		irStartReceive = false;
		return 1;
    }
	// there was no incoming
	return 0;
}

bool irReceiveByte(){
  uint8_t b = 0;
  uint8_t i = 0;
  while(i<8){
    i++;
    b <<= 1;
    irTimeout = millis();
    while(((PIND&(1<<2)) == 0)){ // wait for rise
      if(millis()-irTimeout > 20){
        return false;
      }
    }
    irTimeout = millis();
    while(((PIND&(1<<2)) > 0)){ // wait for fall
      if(millis()-irTimeout > 20){
        return false;
      }
    }
    irStopTime = micros();
    if(irStopTime-irStartTime > bitthreshold){
      b++;
    }
    irStartTime = irStopTime;
  }
  i = irBufferIndex;
  while(i > 0){
    irInputBuffer[i] = irInputBuffer[i-1];
    i--;
  }
  irInputBuffer[0] = b;
  irBufferIndex = (irBufferIndex+1)%8;
  return true;
}

void irWaitForStopBit(){
  irTimeout = millis();
  while(((PIND&(1<<2)) == 0)){ // wait for rise
    if(millis()-irTimeout > 20){
      return;
    }
  }
}
