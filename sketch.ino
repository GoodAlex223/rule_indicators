// TODO: 
// 1. use uint where it is need
// 2. show time on indicators

#include <PCF8574.h>
#include <PinChangeInterrupt.h>
#include <RTClib.h>
#include "AT24CX.h"
// Replace "AT24CX.h" with <AT24CX.h>(add as library)

#define pinA_enc1 3
#define pinB_enc1 2

#define pinA_enc2 5
#define pinB_enc2 4
#define pinReset_enc2 6

#define modeButton 7

#define distanceInd 8
#define timeInd 9

// Global values
const int8_t STEP = 1;

// initialized at startup by reading the legs for encoder 1
volatile bool StateA_enc1, StateB_enc1;
// initialized at startup by reading the legs for encoder 2
volatile bool StateA_enc2, StateB_enc2;
// int8_t is enought for incStreak. If it turns negative it will add only 1

// volatile int8_t incStreak = 0; // used for step increasing during several consecutive scrolls of the second encoder
volatile int8_t incStreak = 0; // used for step increasing during several consecutive scrolls of the second encoder
volatile int64_t incStep; // save previous step to detect when increase the streak

volatile int8_t decoder_i;
// volatile bool isMinus;
// char tempString[9];

// max number uint8_t 2^8 - 1 = 255
// max number uint16_t 2^16 - 1 = 65535
// max number uint32_t 2^32 - 1 = 4294967295
// max number uint64_t 2^64 - 1 = 18446744073709551615
// 
// max numbers int8_t 2^7 - 1 = 127
// max numbers int16_t 2^15 - 1 = 32767
// max numbers int32_t 2^31 - 1 = 2147483647
// max numbers int64_t 2^63 - 1 = 9223372036854775807

// In sec
volatile int64_t currTime = 0;
// In sec
volatile int64_t prevTime = 0;
// max workTime will be about 359996400 sec ("99999.00" to show)
// !!! Change workTime to 0 before next start for correct work
// In sec
volatile int64_t workTime = 0;
// In sec
const int64_t writeTimeout = 60;

// In ms. 1000ms = 1sec
const int64_t delay_value = 60;

// Store current indicators mode
volatile int8_t indicatorsModeNumber = 0;  // int8_t: max values are -128 and 127
// Loop from one state to another:
// indicatorsModeNumber = indicatorsModeNumber + 1 % maxIndicatorsModeNumber
// 0 -> 1 -> 0 -> 1...
const int8_t maxIndicatorsModeNumber = 2;

// The distance covered by the ruler
// at startup 0
volatile int64_t distance = 0;
// To store temp values of either distance or workTime
volatile int64_t numberToShow;

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

RTC_DS1307 RTC;

AT24C32 EepromRTC(0x50);


void safeDistanceIncrease(int32_t increaseStep){
  // Do not increase or decrease distance above or below 9999.999
  // because of 8 lamp limit
  // if ((distance + increaseStep) > 9999.999){
  //   distance = 9999.999;
  // } else if ((distance + increaseStep) < -9999.999) {
  //   distance = -9999.999;
  // } else {
  //   distance = distance + increaseStep;
  // }
  distance = max(min(distance + increaseStep, 9999999), -9999999);
  // if (abs(distance + increaseStep) > 9999999){
  //   distance = 9999999 * (pow(-1, (distance < 0)));
  // } else {
  //   distance += increaseStep;
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
    // Changing distance with new increase step values. 
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
      // for incStep and distance
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
  distance = 0;
  incStreak = 0;
}

void changeMode(){
  Serial.println("Mode was changed!");
  // change mode value to next: 0 -> 1 -> 0 -> 1...
  indicatorsModeNumber = (indicatorsModeNumber + 1) % maxIndicatorsModeNumber;
  Serial.println(indicatorsModeNumber);
}

// print function; can be removed
void print_uint64_t(uint64_t num) {
  // Function that can print uint64_t nums
  char rev[128]; 
  char *p = rev+1;

  while (num > 0) {
    // Serial.println('0' + (num % 10));
    *p++ = '0' + (num % 10);
    num /= 10;
  }
  p--;
  /*Print the number which is now in reverse*/
  while (p > rev) {
    Serial.print(*p--);
  }
  Serial.println(*p);
  // Serial.println();
}

// print function; can be removed
void print_int64_t(int64_t num) {
    char rev[128]; 
    char *p = rev+1;

    if (num < 0) {
        Serial.print('-');
        num = -num;
    }

    while (num > 0) {
        *p++ = '0' + (num % 10);
        num /= 10;
    }
    p--;
    /*Print the number which is now in reverse*/
    while (p > rev) {
        Serial.print(*p--);
    }
    Serial.println(*p);
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

  if (!RTC.begin()) {
    Serial.println("RTC module not responsing");
    while (1);
  }

  // Read saved workTime from memory
  // https://adafruit.github.io/RTClib/html/class_date_time.html#ae4629e7b2ffeac4a0c8f8c3f9c545990
  currTime = RTC.now().unixtime();
  prevTime = currTime;
  if (workTime != 0){
    // write value to memory
    // Case when both battery is low and power off
    // Write to memory workTime value. !!!! Change workTime to 0 before next start
    Serial.print("workTime is not 0: ");
    print_int64_t(workTime);
    Serial.println("Write to memory");
    EepromRTC.writeLong(0, workTime);
  } else {
    workTime = EepromRTC.readLong(0);
  }
  Serial.print("Current workTime: ");
  print_int64_t(workTime);
  
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
  pinMode(pinReset_enc2, INPUT); // Reset distance

  pinMode(modeButton, INPUT);

  pinMode(distanceInd, OUTPUT);
  pinMode(timeInd, OUTPUT);

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

  attachPCINT(digitalPinToPCINT(modeButton), changeMode, RISING);

  // Initialization of global variables
  StateA_enc1 = digitalRead(pinA_enc1);
  StateB_enc1 = digitalRead(pinB_enc1);

  // Initialization of global variables
  StateA_enc2 = digitalRead(pinA_enc2);
  StateB_enc2 = digitalRead(pinB_enc2);

  Serial.print("delay_value: ");
  print_int64_t(delay_value);

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


void showDistance(int64_t num, bool toShowDistance){
  decoder_i = 7;
  if (toShowDistance){
    if (num < 0){
      PCF0.write(0, LOW); // 1
      PCF0.write(1, HIGH); // 2
      // while function works only with num > 0
      num = -num;
    } else {
      PCF0.write(0, HIGH); // 1
      PCF0.write(1, LOW); // 2
    }
    digitalWrite(distanceInd, HIGH);
    digitalWrite(timeInd, LOW);
  } else {
    PCF0.write(0, LOW); // 1
    PCF0.write(1, LOW); // 2
    digitalWrite(distanceInd, LOW);
    digitalWrite(timeInd, HIGH);
  }
  // 1, 2, 4, 8
  while (num > 0) {
    // Serial.println('0' + (num % 10));
    switch(num % 10){
      case 0:
        writeDecoder(decoder_i, 0, 0, 0, 0);
        break;
      case 1:
        writeDecoder(decoder_i, 1, 0, 0, 0);
        break;
      case 2:
        writeDecoder(decoder_i, 0, 1, 0, 0);
        break;
      case 3:
        writeDecoder(decoder_i, 1, 1, 0, 0);
        break;
      case 4:
        writeDecoder(decoder_i, 0, 0, 1, 0);
        break;
      case 5:
        writeDecoder(decoder_i, 1, 0, 1, 0);
        break;
      case 6:
        writeDecoder(decoder_i, 0, 1, 1, 0);
        break;
      case 7:
        writeDecoder(decoder_i, 1, 1, 1, 0);
        break;
      case 8:
        writeDecoder(decoder_i, 0, 0, 0, 1);
        break;
      case 9:
        writeDecoder(decoder_i, 1, 0, 0, 1);
        break;
    }
    decoder_i--;
    num /= 10;
  }
  // Write 0 to rest of lamps
  while (decoder_i > 1){
    writeDecoder(decoder_i, 0, 0, 0, 0);
    decoder_i--;
  }
}


void loop() {
  if (indicatorsModeNumber == 0){
    numberToShow = distance;
    showDistance(numberToShow, true);

  } else if (indicatorsModeNumber == 1){
    // In C/C++, when you divide two integers, the result is also an integer. 
    // This means that the fractional part is discarded, and only the integer part is kept. 
    // Both workTime and writeTimer are sec
    int64_t minutes = workTime / 60;
    // (minutes / 60) is hours. 
    // Multiply by 100 to be able to concatenate minutes(1 | 2 -> 12)
    // minutes % 60 is not full hour(minutes remainder)
    numberToShow = (minutes / 60) * 100 + minutes % 60;
    showDistance(numberToShow, false);
    // ms -> s -> min -> hours
    // ms -> hours = ms / 1000 / 60 / 60
  }
  Serial.print("number:");
  print_int64_t(numberToShow);

  currTime = RTC.now().unixtime();

  // Increase worktime by delay_value
  if ((currTime - prevTime) > writeTimeout){
    workTime += currTime - prevTime;
    Serial.print("workTime: ");
    print_int64_t(workTime);
    Serial.println("Write to memory");
    EepromRTC.writeLong(0, workTime);
    prevTime = currTime;
  }

  delay(delay_value);
}