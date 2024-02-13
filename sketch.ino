#include <PCF8574.h>
#include <PinChangeInterrupt.h>
#include <RTClib.h>
#include "AT24CX.h"
// Replace "AT24CX.h" with <AT24CX.h>(add as library)

// Installation, description, connecting -- https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/
// https://github.com/milesburton/Arduino-Temperature-Control-Library/tree/master
#include <OneWire.h>
#include <DallasTemperature.h>



#define pinA_enc1 3
#define pinB_enc1 2

#define pinA_enc2 5
#define pinB_enc2 4
#define pinReset_enc2 6

#define timeInd 7
#define distanceInd 8

#define modeButton 9

#define generalStartSignal 10
#define spindelStartSignal 11

// Data wire is plugged into port 12 on the Arduino
#define ONE_WIRE_BUS 12



// max number uint8_t 2^8 - 1 = 255
// max number uint16_t 2^16 - 1 = 65535
// max number uint32_t 2^32 - 1 = 4294967295
// max number uint64_t 2^64 - 1 = 18446744073709551615
// 
// max numbers int8_t 2^7 - 1 = 127
// max numbers int16_t 2^15 - 1 = 32767
// max numbers int32_t 2^31 - 1 = 2147483647
// max numbers int64_t 2^63 - 1 = 9223372036854775807
// 
// max numbers(in 32x systems) long 2^31 - 1 = 2147483647
// max numbers(in 64x systems) long 2^63 - 1 = 9223372036854775807
const int8_t STEP = 1;
volatile int8_t prevDistanceIncStep; // save previous step to detect when increase the streak
volatile int8_t prevMotorIncStep;
volatile int8_t prevSpindelIncStep;

volatile bool StateA_enc1, StateB_enc1;
volatile bool StateA_enc2, StateB_enc2;

// The distance covered by the ruler
// at startup 0
volatile int64_t distance = 0;
// int8_t is enought for distanceIncStreak. If it turns negative it will add only 1
// volatile int8_t distanceIncStreak = 0; // used for step increasing during several consecutive scrolls of the second encoder
volatile int8_t distanceIncStreak = 0; // used for step increasing during several consecutive scrolls of the second encoder

int8_t decoder_i;
// To store temp values of either distance or work times
// Used int64_t type to include int32_t and below and uint32_t and below
int64_t numberToShow;

// Store current indicators mode
volatile int8_t indicatorsModeNumber = 0;  // int8_t: max values are -128 and 127
// Loop from one state to another:
// indicatorsModeNumber = indicatorsModeNumber + 1 % maxIndicatorsModeNumber
// 0 -> 1 -> 2 -> ... -> 0 -> 1 -> ...
const int8_t maxIndicatorsModeNumber = 5;

// In sec
volatile uint32_t currTime = 0;
volatile uint32_t prevModeButtonInterTime = 0;
// volatile bool isModeChanged;

// max motorSeconds and spindelMotorSeconds will be about 359999999 sec 
// ("99999.59" hours to show; only for 7 lamps)
// In sec
uint32_t motorSeconds;
const uint8_t motorSecondsMemoryAdress = 0;
volatile bool toIncreaseMotorSeconds = false;
volatile uint32_t motorPrevTime;
//
volatile uint8_t motorIncStreak = 0;

// In sec
uint32_t spindelMotorSeconds;
const uint8_t spindelMotorSecondsMemoryAdress = 4;
volatile bool toIncreaseSpindelMotorSeconds = false;
volatile uint32_t spindelPrevTime;
//
volatile uint8_t spindelIncStreak = 0;

//
int32_t temperature;

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

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature SENSORS(&oneWire);
// arrays to hold device addresses
DeviceAddress DSAddress;



void safeDistanceIncrease(int64_t increaseStep){
  // DO NOT PASS increaseStep above increaseStep_type_int_limit - 9999999(for 7 lamps) 
  // as it change distance to negative
  distance = max(min(distance + increaseStep, 9999999), -9999999);
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
  distanceIncStreak = 0;
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
  distanceIncStreak = 0;
}


uint32_t increaseHours(uint32_t timer, int32_t increaseStepSeconds){
  // 359999999 seconds or "99999.59" on display
  // This is done to not do substruction from minuend(uint32_t) that is less then subtrahend
  if (increaseStepSeconds < 0 && -increaseStepSeconds > timer){
    return 0;
  }
  else
  {
    return min(increaseStepSeconds + timer, 359999999);
  }
}


void changeIncStreak(uint8_t mode, int8_t step){
  // int8_t is enought for increase streak. If it turns negative it will add only 1
  // if inc direction do not changes, increase streak by 1, else reset
  switch (mode) {
  case 0:
    if (prevDistanceIncStep == step){
      // set limit distanceIncStreak
      distanceIncStreak = min(distanceIncStreak + 1, 70);
    } else {
      distanceIncStreak = 0;
    }
    prevDistanceIncStep = step;
    break;

  case 1:
    if (prevMotorIncStep == step){
      // set limit for increase streak
      motorIncStreak = min(motorIncStreak + 1, 70);
    } else {
      motorIncStreak = 0;
    }
    prevMotorIncStep = step;
    break;

  case 2:
    if (prevSpindelIncStep == step){
      // set limit for increase streak
      spindelIncStreak = min(spindelIncStreak + 1, 70);
    } else {
      spindelIncStreak = 0;
    }
    prevSpindelIncStep = step;
    break;
  }
}


void changeValue(uint8_t mode){
  switch (mode){
  case 0:
    // Changing distance with new increase step values. 
    // All limits in states and multiplying of step are random numbers and can be adjusted
    if (10 > distanceIncStreak){
      safeDistanceIncrease(prevDistanceIncStep);
    } else if (20 > distanceIncStreak){ // 10, 20, 30 are random nums
      safeDistanceIncrease(prevDistanceIncStep * 10);
    } else if (30 > distanceIncStreak){
      safeDistanceIncrease(prevDistanceIncStep * 100);
    } else if (40 > distanceIncStreak){
      safeDistanceIncrease(prevDistanceIncStep * 1000);
    } else if (50 > distanceIncStreak){
      safeDistanceIncrease(prevDistanceIncStep * 10000);
    } else if (60 > distanceIncStreak){
      safeDistanceIncrease(prevDistanceIncStep * 100000);
    } else {
      // Multilying more then 1000000 require int64_t or long or even more 
      // for prevDistanceIncStep and distance
      // DO NOT PASS increaseStep above increaseStep_type_int_limit - 9999999(for 7 lamps) 
      safeDistanceIncrease(prevDistanceIncStep * 1000000);
    }
    break;

  case 1:
    // Changing distance with new increase step values. 
    // All limits in states and multiplying of step are random numbers and can be adjusted
    // 10, 20, 30 are random nums
    // max increaseStepSeconds is 2^31 - 1 = +-2147483647
    if (10 > motorIncStreak){
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep);
    } else if (20 > motorIncStreak){
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep * 10);
    } else if (30 > motorIncStreak){
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep * 100);
    } else if (40 > motorIncStreak){
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep * 1000);
    } else if (50 > motorIncStreak){
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep * 10000);
    } else if (60 > motorIncStreak){
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep * 100000);
    } else {
      // DO NOT PASS increaseStep above increaseStep_type_int_limit - 359999999(for 7 lamps) 
      motorSeconds = increaseHours(motorSeconds, prevMotorIncStep * 1000000);
    }
    break;

  case 2:
    // Changing distance with new increase step values.
    // All limits in states and multiplying of step are random numbers and can be adjusted
    // 10, 20, 30 are random nums
    // max increaseStepSeconds is 2^31 - 1 = +-2147483647
    if (10 > spindelIncStreak){
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep);
    } else if (20 > spindelIncStreak){
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep * 10);
    } else if (30 > spindelIncStreak){
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep * 100);
    } else if (40 > spindelIncStreak){
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep * 1000);
    } else if (50 > spindelIncStreak){
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep * 10000);
    } else if (60 > spindelIncStreak){
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep * 100000);
    } else {
      // DO NOT PASS increaseStep above increaseStep_type_int_limit - 359999999(for 7 lamps) 
      spindelMotorSeconds = increaseHours(spindelMotorSeconds, prevSpindelIncStep * 1000000);
    }
    break;
  }
}


void changeValuesToShow(int8_t step, uint8_t mode){
  changeIncStreak(mode, step);
  changeValue(mode);
}


// Interrupt on channel A of the rotary encoder
void interruptChannelA_enc2() {
  // change the channel state (to avoid digitalRead())
  StateA_enc2 = !StateA_enc2;
  // StateA = digitalRead(3);
  // adjust the coordinate
  switch (indicatorsModeNumber)
  {
  case 0:
    if (StateA_enc2 != StateB_enc2){
      changeValuesToShow(STEP, indicatorsModeNumber);
      // increaseDistance_enc2(STEP);
    } else {
      changeValuesToShow(-STEP, indicatorsModeNumber);
      // increaseDistance_enc2(-STEP);
    }
    break;
  case 1:
  case 2:
    if (StateA_enc2 != StateB_enc2){
      changeValuesToShow(1, indicatorsModeNumber);
    } else {
      changeValuesToShow(-1, indicatorsModeNumber);
    }
    break;
  }
}


// Interrupt on channel B of the rotary encoder
void interruptChannelB_enc2() {
  // change the channel state (to avoid digitalRead())
  StateB_enc2 = !StateB_enc2;
  // StateB = digitalRead(2);
  // adjust the coordinate
  switch (indicatorsModeNumber)
  {
  case 0:
    if (StateA_enc2 == StateB_enc2){
      changeValuesToShow(STEP, indicatorsModeNumber);
      // increaseDistance_enc2(STEP);
    } else {
      changeValuesToShow(-STEP, indicatorsModeNumber);
      // increaseDistance_enc2(-STEP);
    }
    break;
  case 1:
  case 2:
    if (StateA_enc2 == StateB_enc2){
      changeValuesToShow(1, indicatorsModeNumber);
    } else {
      changeValuesToShow(-1, indicatorsModeNumber);
    }
    break;
  }
}


void buttonEnc2(){
  distance = 0;
  distanceIncStreak = 0;
}


void changeMode(){
  // 1 sec delay to avoid button rattle
  // if (isModeChanged == false){
  //   // change mode value to next: 0 -> ... -> maxIndicatorsModeNumber - 1 -> 0 -> ...
  //   indicatorsModeNumber = (indicatorsModeNumber + 1) % maxIndicatorsModeNumber;
  //   isModeChanged = true;
  // }
  if (currTime - prevModeButtonInterTime >= 1){
    // change mode value to next: 0 -> ... -> maxIndicatorsModeNumber - 1 -> 0 -> ...
    indicatorsModeNumber = (indicatorsModeNumber + 1) % maxIndicatorsModeNumber;
    prevModeButtonInterTime = currTime;
  }
}


void changeMotorSecondsIncreasingState(){
  toIncreaseMotorSeconds = !toIncreaseMotorSeconds;
  motorPrevTime = currTime;
}

void changeSpindelMotorSecondsIncreasingState(){
  toIncreaseSpindelMotorSeconds = !toIncreaseSpindelMotorSeconds;
  spindelPrevTime = currTime;
}

// // print function; can be removed
// void print_uint64_t(uint64_t num) {
//   // Function that can print uint64_t nums
//   char rev[128]; 
//   char *p = rev+1;

//   while (num > 0) {
//     // Serial.println('0' + (num % 10));
//     *p++ = '0' + (num % 10);
//     num /= 10;
//   }
//   p--;
//   /*Print the number which is now in reverse*/
//   while (p > rev) {
//     Serial.print(*p--);
//   }
//   Serial.println(*p);
//   // Serial.println();
// }

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
    // Serial.println(*p);
}


void setupTemperatureModules(){
  Serial.println("Temperature modules setupes started.");

  // Setup temperature sensor
  // Start up the library
  // locate devices on the bus
  Serial.print("Locating devices...");
  SENSORS.begin();

  Serial.print("Found ");
  Serial.print(SENSORS.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (SENSORS.isParasitePowerMode()){
    Serial.println("ON");
  } else {
    Serial.println("OFF");
  }
  if (!SENSORS.getAddress(DSAddress, 0)){
    Serial.println("Unable to find address for Device 0");
    while(1);
  }
  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (DSAddress[i] < 16) Serial.print("0");
    Serial.print(DSAddress[i], HEX);
  }
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  SENSORS.setResolution(DSAddress, 9);

  Serial.print("Device 0 Resolution: ");
  Serial.print(SENSORS.getResolution(DSAddress), DEC);
  Serial.println();

  Serial.println("Temperature modules setupes were finished.");
}


void setupRTCModule(){
  Serial.println("RTC setupes started.");

  // Setup real time clock
  RTC.begin();
  if (!RTC.isrunning()) {
    Serial.println("RTC module not responsing");
    while (1);
  }
  // https://forum.arduino.cc/t/why-my-rtc-module-is-not-keeping-time/1000098
  // Run only once to setup RTC
  // RTC.adjust(DateTime(__DATE__, __TIME__));

  // https://adafruit.github.io/RTClib/html/class_date_time.html#ae4629e7b2ffeac4a0c8f8c3f9c545990
  // Returns uint32_t seconds. Its ok for int64_t
  currTime = RTC.now().unixtime();

  // Read saved motorSeconds from memory
  motorSeconds = EepromRTC.readLong(motorSecondsMemoryAdress);
  spindelMotorSeconds = EepromRTC.readLong(spindelMotorSecondsMemoryAdress);
  // 4294967295 is limit for uint32_t and for page of memory module 24c32
  // (max page size is 4 bytes(4 * 8 = 32))
  // 359999999 is 99999 hours("99999.59" on display)
  if (motorSeconds > 359999999){
    motorSeconds = 0;
  }
  if (spindelMotorSeconds > 359999999){
    spindelMotorSeconds = 0;
  }
  Serial.print("Get time from RTC -- currTime: ");
  Serial.println(currTime);
  Serial.print("Read from memory -- motorSeconds: ");
  Serial.println(motorSeconds);
  Serial.print("Read from memory -- spindelMotorSeconds: ");
  Serial.println(spindelMotorSeconds);

  Serial.println("RTC setupes were finished.");
}


void setupPCFsExpanders(){
  Serial.println("PCFs setupes started.");
  // Setup all PCFs
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


void setupModules() {
  Serial.println("Modules setupes started");
  setupPCFsExpanders();
  setupRTCModule();
  setupTemperatureModules();
  Serial.println("Modules setupes were finished.");
}


void setup() {
  Serial.begin(115200);

  Serial.println("Setup started");

  setupModules();

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

  pinMode(generalStartSignal, INPUT);
  pinMode(spindelStartSignal, INPUT);

  // Enable interrupt A_enc1
  attachPCINT(digitalPinToPCINT(pinA_enc1), interruptChannelA_enc1, CHANGE);
  // Enable interrupt B_enc1
  attachPCINT(digitalPinToPCINT(pinB_enc1), interruptChannelB_enc1, CHANGE);

  // Enable interrupt A_enc2
  attachPCINT(digitalPinToPCINT(pinA_enc2), interruptChannelA_enc2, CHANGE);
  // Enable interrupt B_enc2
  attachPCINT(digitalPinToPCINT(pinB_enc2), interruptChannelB_enc2, CHANGE);
  // Enable interrupt reset_enc2
  attachPCINT(digitalPinToPCINT(pinReset_enc2), buttonEnc2, RISING);

  attachPCINT(digitalPinToPCINT(modeButton), changeMode, RISING);

  attachPCINT(digitalPinToPCINT(generalStartSignal), changeMotorSecondsIncreasingState, CHANGE);
  attachPCINT(digitalPinToPCINT(spindelStartSignal), changeSpindelMotorSecondsIncreasingState, CHANGE);

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


void showNumber(int64_t num, int8_t mode){
  switch (mode)
  {
  case 0:
    if (num > 0){
      PCF0.write(0, HIGH); // 1
      PCF0.write(1, LOW); // 2
      PCF0.write(2, LOW); // 4
      PCF0.write(3, LOW); // 8
    } else {
      PCF0.write(0, LOW); // 1
      PCF0.write(1, HIGH); // 2
      PCF0.write(2, LOW); // 4
      PCF0.write(3, LOW); // 8
      // while function works only with num > 0
      num = -num;
    }
    digitalWrite(distanceInd, HIGH);
    digitalWrite(timeInd, LOW);
    break;
  case 1:
    PCF0.write(0, LOW); // 1
    PCF0.write(1, LOW); // 2
    PCF0.write(2, HIGH); // 4
    PCF0.write(3, LOW); // 8
    digitalWrite(distanceInd, LOW);
    digitalWrite(timeInd, HIGH);
    break;
  case 2:
    PCF0.write(0, LOW); // 1
    PCF0.write(1, LOW); // 2
    PCF0.write(2, LOW); // 4
    PCF0.write(3, HIGH); // 8
    digitalWrite(distanceInd, LOW);
    digitalWrite(timeInd, HIGH);
    break;
  case 4:
    if (num > 0){
      PCF0.write(0, HIGH); // 1
      PCF0.write(1, LOW); // 2
      PCF0.write(2, LOW); // 4
      PCF0.write(3, LOW); // 8
    } else {
      PCF0.write(0, LOW); // 1
      PCF0.write(1, HIGH); // 2
      PCF0.write(2, LOW); // 4
      PCF0.write(3, LOW); // 8
      // while function works only with num > 0
      num = -num;
    }
    digitalWrite(distanceInd, HIGH);
    digitalWrite(timeInd, LOW);
    break;
  default:
    PCF0.write(0, LOW); // 1
    PCF0.write(1, LOW); // 2
    PCF0.write(2, LOW); // 4
    PCF0.write(3, LOW); // 8
    digitalWrite(distanceInd, LOW);
    digitalWrite(timeInd, HIGH);
    break;
  }
  // Function that shows last 7 numbers and sign(in case)
  decoder_i = 7;
  // 1, 2, 4, 8
  // decoder_i = 0 is decoder for sign lamp
  // if num == 0 then num % 0 is 0 that is correct in any case
  while (decoder_i > 0) {
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
}


void loop() {
  // Do not increase work times by delay_value because interruptions additionally take some time
  // so time increases not only after delay
  currTime = RTC.now().unixtime();

  SENSORS.requestTemperatures();
  // Save only int part and 3 digits after dot as integer for correct displaying
  temperature = SENSORS.getTempC(DSAddress) * 1000;
  if (temperature > 50000){
    indicatorsModeNumber = 4;
  }

  switch (indicatorsModeNumber)
  {
  case 0:
    numberToShow = distance;
    break;
  case 1:
    // In C/C++, when you divide two integers, the result is also an integer. 
    // This means that the fractional part is discarded, and only the integer part is kept. 
    // Both motorSeconds and writeTimer are sec
    // (motorSeconds / 3600) is hours.
    // Multiply by 100 to be able to concatenate minutes(1 | 22 -> 122)
    // minutes % 60 is not full hour(minutes remainder)
    numberToShow = (motorSeconds / 3600) * 100 + (motorSeconds / 60) % 60;
    break;
  case 2:
    numberToShow = (spindelMotorSeconds / 3600) * 100 + (spindelMotorSeconds / 60) % 60;
    break;
  case 3:
    // hours|minutes|seconds -> 11|23|45 -> 112345
    // currTime / 60 / 60 % 24 - rest of hours
    // currTime / 60 % 60      - rest of minutes
    // currTime % 3600         - rest of seconds
    // currTime % 60 % 60 and currTime % 3600 have different results
    // (((currTime / 3600) % 24) +2) and ((currTime / 3600) % 24 +2) have different results
    numberToShow = ((currTime / 3600) % 24) * 10000 + ((currTime / 60) % 60) * 100 + currTime % 60 % 60;
    break;
  case 4:
    numberToShow = temperature;
    break;
  }

  showNumber(numberToShow, indicatorsModeNumber);

  Serial.print("mms:");
  print_int64_t(distance);
  Serial.print(" - time(sec):");
  Serial.print(currTime);
  Serial.print(" - mode state:");
  Serial.print(indicatorsModeNumber);
  Serial.print(" - motor state:");
  Serial.print(toIncreaseMotorSeconds);
  Serial.print(" - motorSeconds:");
  Serial.print(motorSeconds);
  Serial.print(" - spindel state:");
  Serial.print(toIncreaseSpindelMotorSeconds);
  Serial.print(" - spindel(sec):");
  Serial.print(spindelMotorSeconds);

  // currTime and *PrevTime are seconds
  // 60 is seconds
  // Wokwi timer while simulation is not correct, use other
  if (toIncreaseMotorSeconds){
    if ((currTime - motorPrevTime) > 60){
        motorSeconds = min(motorSeconds + currTime - motorPrevTime, 359999999);
        EepromRTC.writeLong(motorSecondsMemoryAdress, motorSeconds);
        motorPrevTime = currTime;
        Serial.print(" - WROTE TO MEMORY: motorSeconds");
      }
  }
  if (toIncreaseSpindelMotorSeconds){
    if ((currTime - spindelPrevTime) > 60){
      spindelMotorSeconds = min(spindelMotorSeconds + currTime - spindelPrevTime, 359999999);
      EepromRTC.writeLong(spindelMotorSecondsMemoryAdress, spindelMotorSeconds);
      spindelPrevTime = currTime;
      Serial.print(" - WROTE TO MEMORY: spindelMotorSeconds");
    }
  }
  Serial.println();
  // if (butt == 1){
  //   enablePCINT(digitalPinToPCINT(modeButton));
  // }
  // butt = 0;
  // delay value must be not greater then 1000ms(1 sec) for fast work
  switch (indicatorsModeNumber)
  {
  case 0:
    delay(500);
    break;
  default:
    delay(1000);
    break;
  }
  // isModeChanged = false;
}
// TODO:
// - Load AT24CX library from https://github.com/cyberp/AT24Cx/tree/master and replace "AT24CX.h" with <AT24CX.h>
// - Use rotator to check if the number on indicators is correct because of new showing system
// - Press the button to check if modes are changed properly
// - Turn on device and wait a 1-2 minutes and see if time is written to memory(output in console)
// - Turn off and turn on device to see if time was read from memory