#include <Wire.h>
#include <PCF8574.h>
#include <PinChangeInterrupt.h>

#define pinA_enc1 3
#define pinB_enc1 2

#define pinA_enc2 5
#define pinB_enc2 4
#define pinReset_enc2 6

// Global values
const int8_t STEP = 1;

// initialized at startup by reading the legs for encoder 1
volatile bool StateA_enc1, StateB_enc1;
// initialized at startup by reading the legs for encoder 2
volatile bool StateA_enc2, StateB_enc2;
// int8_t is enought for incStreak. If it turns negative it will add only 1

// volatile int8_t incStreak = 0; // used for step increasing during several consecutive scrolls of the second encoder
volatile int8_t incStreak = 0; // used for step increasing during several consecutive scrolls of the second encoder
volatile int64_t DISTANCE = 0; // at startup 0
volatile int64_t incStep; // save previous step to detect when increase the streak
volatile int64_t tempDistance;

volatile int8_t decoder_i;
volatile bool isMinus;
char tempString[9];

// Expanders: PCF8574AP(https://www.nxp.com/docs/en/data-sheet/PCF8574_PCF8574A.pdf)
// Default pins A4-SDA, A5-SCL
// Correct adresses are 0x38 - 0x3F
// --- Lamps 1, 2 ---
// PCF8574 PCF0(0b0100011);
PCF8574 PCF0(0x39); // bin: 0b00111001
// PCF8574 PCF0(0xB9);
// --- Lamps 3, 4 ---
// PCF8574 PCF1(0b0100101);
PCF8574 PCF1(0x3D); // bin: 0b00111101
// PCF8574 PCF1(0xBD);
// --- Lamps 5, 6 ---
// PCF8574 PCF2(0b0100110);
PCF8574 PCF2(0x3F); // bin: 0b00111111
// PCF8574 PCF2(0xBF);
// --- Lamps 7, 8 ---
// PCF8574 PCF3(0b0100111);
PCF8574 PCF3(0x3B); // bin: 0b00111011
// PCF8574 PCF3(0xBB);


void safeDistanceIncrease(int32_t increaseStep){
  // Do not increase or decrease DISTANCE above or below 9999.999
  // because of 8 lamp limit
  // if ((DISTANCE + increaseStep) > 9999.999){
  //   DISTANCE = 9999.999;
  // } else if ((DISTANCE + increaseStep) < -9999.999) {
  //   DISTANCE = -9999.999;
  // } else {
  //   DISTANCE = DISTANCE + increaseStep;
  // }
  DISTANCE = max(min(DISTANCE + increaseStep, 9999999), -9999999);
  // if (abs(DISTANCE + increaseStep) > 9999999){
  //   DISTANCE = 9999999 * (pow(-1, (DISTANCE < 0)));
  // } else {
  //   DISTANCE += increaseStep;
  // }
}


void increaseDistance_enc2(int8_t step){
    // int8_t is enought for incStreak. If it turns negative it will add only 1
    // if inc direction do not changes, increase streak by 1, else reset
    if (incStep == step){
      // set limit incStreak
      incStreak = min(incStreak + 1, 70);
    } else {
      incStreak = 0;
    }
    incStep = step;
    // Changing DISTANCE with new increase step values. 
    // All limits in states and multiplying of step are random numbers and can be adjusted
    if (10 > incStreak){
      safeDistanceIncrease(incStep);
    } else if (20 > incStreak){ // 10, 20, 30 are random nums
      safeDistanceIncrease(incStep * 10);
    } else if (30 > incStreak){
      safeDistanceIncrease(incStep * 100);
    } else if (40 > incStreak){
      safeDistanceIncrease(incStep * 1000);
    } else if (50 > incStreak){
      safeDistanceIncrease(incStep * 10000);
    } else if (60 > incStreak){
      safeDistanceIncrease(incStep * 100000);
    } else {
      // Multilying more then 1000000 require int64_t or long or even more 
      // for incStep and DISTANCE
      safeDistanceIncrease(incStep * 1000000);
    }
}


// Interrupt on channel A of the rotary encoder
void interruptChannelA_enc1() {
  // change the channel state (to avoid digitalRead())
  StateA_enc1 = !StateA_enc1;
  // StateA = digitalRead(3);
  // adjust the coordinate
  if (StateA_enc1 != StateB_enc1){
    safeDistanceIncrease(STEP);
  } else {
    safeDistanceIncrease(-STEP);
  }
  incStreak = 0;
}


// Interrupt on channel B of the rotary encoder
void interruptChannelB_enc1() {
  // change the channel state (to avoid digitalRead())
  StateB_enc1 = !StateB_enc1;
  // StateB = digitalRead(2);
  // adjust the coordinate
  if (StateA_enc1 == StateB_enc1){
    safeDistanceIncrease(STEP);
  } else {
    safeDistanceIncrease(-STEP);
  }
  incStreak = 0;
}


// Interrupt on channel A of the rotary encoder
void interruptChannelA_enc2() {
  // change the channel state (to avoid digitalRead())
  StateA_enc2 = !StateA_enc2;
  // StateA = digitalRead(3);
  // adjust the coordinate
  if (StateA_enc2 != StateB_enc2){
    // safeDistanceIncrease(STEP);
    increaseDistance_enc2(STEP);
  } else {
    // safeDistanceIncrease(-STEP);
    increaseDistance_enc2(-STEP);
  }
}


// Interrupt on channel B of the rotary encoder
void interruptChannelB_enc2() {
  // change the channel state (to avoid digitalRead())
  StateB_enc2 = !StateB_enc2;
  // StateB = digitalRead(2);
  // adjust the coordinate
  if (StateA_enc2 == StateB_enc2){
    // safeDistanceIncrease(STEP);
    increaseDistance_enc2(STEP);
  } else {
    // safeDistanceIncrease(-STEP);
    increaseDistance_enc2(-STEP);
  }
}


void interruptResetDISTANCE(){
  DISTANCE = 0;
  incStreak = 0;
}


void setupPcfs() {
  Serial.println("PCFs setupes started");

  // Check for correct address of connected PCF
  if (!PCF0.begin()) {
    Serial.println("PCF0 Chip not responding.");
  }
  if (!PCF0.isConnected()) {
    Serial.println("PCF0 could not initialize... => not connected");
    while (1);
  }
  // Write 0 to all 8 pins for correct work
  PCF0.write8(0);

  if (!PCF1.begin()) {
    Serial.println("PCF1 Chip not responding.");
  }
  if (!PCF1.isConnected()) {
    Serial.println("PCF1 could not initialize... => not connected");
    while (1);
  }
  PCF1.write8(0);

  if (!PCF2.begin()) {
    Serial.println("PCF2 Chip not responding.");
  }
  if (!PCF2.isConnected()) {
    Serial.println("PCF2 could not initialize... => not connected");
    while (1);
  }
  PCF2.write8(0);

  if (!PCF3.begin()) {
    Serial.println("PCF3 Chip not responding.");
  }
  if (!PCF3.isConnected()) {
    Serial.println("PCF3 could not initialize... => not connected");
    while (1);
  }
  PCF3.write8(0);

  Serial.println("PCFs setupes were finished.");
}


void setup() {
  Serial.begin(115200);

  Serial.println("Setup started");

  setupPcfs();

  // Serial.begin(9600);
  // Encoder 1
  pinMode(pinA_enc1, INPUT); // A_enc1
  pinMode(pinB_enc1, INPUT); // B_enc1

  // Encoder 2
  pinMode(pinA_enc2, INPUT); // A_enc2
  pinMode(pinB_enc2, INPUT); // B_enc2
  pinMode(pinReset_enc2, INPUT); // Reset DISTANCE

  // Enable interrupt A_enc1
  attachPCINT(digitalPinToPCINT(pinA_enc1), interruptChannelA_enc1, CHANGE);
  // Enable interrupt B_enc1
  attachPCINT(digitalPinToPCINT(pinB_enc1), interruptChannelB_enc1, CHANGE);

  // Enable interrupt A_enc2
  attachPCINT(digitalPinToPCINT(pinA_enc2), interruptChannelA_enc2, CHANGE);
  // Enable interrupt B_enc2
  attachPCINT(digitalPinToPCINT(pinB_enc2), interruptChannelB_enc2, CHANGE);
  // Enable interrupt reset_enc2
  attachPCINT(digitalPinToPCINT(pinReset_enc2), interruptResetDISTANCE, RISING);

  // Initialization of global variables
  StateA_enc1 = digitalRead(pinA_enc1);
  StateB_enc1 = digitalRead(pinB_enc1);

  // Initialization of global variables
  StateA_enc2 = digitalRead(pinA_enc2);
  StateB_enc2 = digitalRead(pinB_enc2);

  Serial.println("Setup finished");
}


void writeDecoder(int8_t decoder_i, bool p1, bool p2, bool p4, bool p8){
  switch (decoder_i){
    case 0:
    case 1:
      PCF0.write(0 + 4 * (decoder_i % 2), p1); // 1
      PCF0.write(1 + 4 * (decoder_i % 2), p2); // 2
      PCF0.write(2 + 4 * (decoder_i % 2), p4); // 4
      PCF0.write(3 + 4 * (decoder_i % 2), p8); // 8
      break;
    case 2:
    case 3:
      PCF1.write(0 + 4 * (decoder_i % 2), p1); // 1
      PCF1.write(1 + 4 * (decoder_i % 2), p2); // 2
      PCF1.write(2 + 4 * (decoder_i % 2), p4); // 4
      PCF1.write(3 + 4 * (decoder_i % 2), p8); // 8
      break;
    case 4:
    case 5:
      PCF2.write(0 + 4 * (decoder_i % 2), p1); // 1
      PCF2.write(1 + 4 * (decoder_i % 2), p2); // 2
      PCF2.write(2 + 4 * (decoder_i % 2), p4); // 4
      PCF2.write(3 + 4 * (decoder_i % 2), p8); // 8
      break;
    case 6:
    case 7:
      PCF3.write(0 + 4 * (decoder_i % 2), p1); // 1
      PCF3.write(1 + 4 * (decoder_i % 2), p2); // 2
      PCF3.write(2 + 4 * (decoder_i % 2), p4); // 4
      PCF3.write(3 + 4 * (decoder_i % 2), p8); // 8
      break;
  }
}


void loop() {
  tempDistance = DISTANCE;

  // char tempString[9];

  // dtostrf(tempDistance, 9, 3, tempString);
  dtostrf(tempDistance, 8, 0, tempString);

  Serial.print("number:");
  // Serial.println(tempDistance / 1000.0, 3);
  Serial.println(tempString);
  // Serial.println(DISTANCE, 3);

  decoder_i = 1;
  isMinus = false;

  // iterate through all chars of distanse str
  // Start with first character to omit sign of number and show it last
  for (int8_t char_i = 1; char_i < strlen(tempString); char_i++) {
    // 1, 2, 4, 8
    switch (tempString[char_i]){
      case ' ':
      case '0':
        writeDecoder(decoder_i, 0, 0, 0, 0);
        break;
      case '-':
        // Minus may not be the first char(index 0). E.g. |  -12345| or |     -12|
        // So for example in case moving encoder from '-12' to '-8' 
        // it will no clear '1' in '-12' and do not show '0' instead of '-' in '-8'
        // (both empty char and minus(not at index 0) are '0' like above in code)
        isMinus = true;
        writeDecoder(decoder_i, 0, 0, 0, 0);
        break;
      case '1':
        writeDecoder(decoder_i, 1, 0, 0, 1);
        break;
      case '2':
        writeDecoder(decoder_i, 0, 1, 0, 0);
        break;
      case '3':
        writeDecoder(decoder_i, 1, 1, 0, 0);
        break;
      case '4':
        writeDecoder(decoder_i, 0, 0, 1, 0);
        break;
      case '5':
        writeDecoder(decoder_i, 1, 0, 1, 0);
        break;
      case '6':
        writeDecoder(decoder_i, 0, 1, 1, 0);
        break;
      case '7':
        writeDecoder(decoder_i, 1, 1, 1, 0);
        break;
      case '8':
        writeDecoder(decoder_i, 0, 0, 0, 1);
        break;
      case '9':
        writeDecoder(decoder_i, 1, 0, 0, 1);
        break;
      // case '.':
      //   // Do not increase decoder index, because dot is skipped and always showed
      //   decoder_i--;
      //   // break;
    }
    decoder_i++;
  }
  if (isMinus || tempString[0] == '-'){
    PCF0.write(0, LOW); // 1
    PCF0.write(1, HIGH); // 2
  } else {
    PCF0.write(0, HIGH); // 1
    PCF0.write(1, LOW); // 2
  }
  delay(60);
}