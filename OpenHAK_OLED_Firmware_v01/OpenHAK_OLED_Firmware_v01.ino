

/*
  THIS CODE IS NOT INTENDED FOR NEW DESIGNS
  UNLESS, THAT IS, YOU HAVE AN OpenHAK LAYING AROUND

  WYSIWYG. NO GUARANTEES OR WARANTEES.

  This code targets the OpenHAK BETA hardware.
  Also will target the Biohacking Village DEFCON 27 Badge
	Find the SELECT YOUR VERSION section below to adjust for target

  Made by Joel Murphy and Leif Percifield 2016 and on and on
  www.github.com/OpenHAK

	      Issue with file size due to DFU set default to dual bank
	      To adjust OTA bootloader bank size adjust go here
	      Library/Arduino15/packages/OpenHAK/hardware/Simblee/1.1.4/variants/Simblee/ota_bootloader.h
	      based on advice from https://devzone.nordicsemi.com/f/nordic-q-a/19339/dfu-ota-giving-error-upload-failed-remote-dfu-data-size-exceeds-limit-while-flashing-application


*/

 // SELECT YOUR VERSION
 #define BETA_TESTER 1	// use this for the 2019 beta hardware
 // #define BIO_VILLAGE_BADGE 1	// use this for the BioHacking Village Badge for DEFCON 27
#include "OpenHAK.h"
#include <filters.h>
#include <BMI160Gen.h>
#include "QuickStats.h"
#include <Lazarus.h>
#include <Timezone.h>
#include <ota_bootloader.h>
#include <SimbleeBLE.h>
#include <Wire.h>
#include <OpenHAK_MicroOLED.h>


Lazarus Lazarus;

time_t localTime, utc;
int minutesOffset = 0;
signed char timeZoneOffset = 0;

QuickStats stats; //initialize an instance of stats class
MicroOLED oled(OLED_RESET, DC);    // reset pin, I2C address
String bpmString = "";
String VERSION = "1.0.0";

  //  TESTING
#ifdef SERIAL_LOG
  unsigned long thisTestTime;
  unsigned long thatTestTime;
#endif

uint32_t startTime;

float volts = 0.0;

int LEDpin[] = {RED,GRN,BLU};

volatile boolean MAX_interrupt = false;
short interruptSetting;
short interruptFlags;
boolean getTempFlag = false;
byte tempInteger;
byte tempFraction;
float Celcius;
float Fahrenheit;
byte MAXsampleCounter = 0;
int REDvalue;
int IRvalue;
int GRNvalue;
byte sampleAve;
byte MAX_mode;
byte sampleRange;
byte sampleRate;
byte pulseWidth;
int LEDcurrent;
byte readPointer;
byte writePointer;
byte ovfCounter;
uint8_t arrayBeats[100];
int beatCounter;


long lastTime;
long awakeTime;
int sleepTimeNow;
uint32_t startTime;
#ifndef SERIAL_LOG
int sleepTime = 600; //600 is production
#else
int sleepTime = 60; //600 is production
#endif


uint8_t modeNum = 10;
bool bConnected = false;

// interval between advertisement transmissions ms (range is 20ms to 10.24s) - default 20ms
int bleInterval = 500;  // 675 ms between advertisement transmissions longer time = better battery but slower scan/connect

/* DATA structures
   Sample data every 10 minutes
   Each sample
   Steps since Midnight (uint16_t) 0 - 65535
   Last BPM - rolling average of 30 seconds of caputre (uint8_t) 0 - 255
   Time ()

*/
typedef struct {
  uint32_t epoch;
  uint16_t steps;
  uint8_t hr;
  uint8_t hrDev;
  uint8_t battery;
  uint8_t aux1;
  uint8_t aux2;
  uint8_t aux3;
} Payload;

Payload samples[512];

uint16_t currentSample = 0;

uint8_t advdata[14] =
{
  13, // length // 0
  0x09, // complete local name type // 1
  0x4f, // 'O' // 2
  0x70, // 'p' // 3
  0x65, // 'e' // 4
  0x6e, // 'n' // 5
  0x48, // 'H' // 6
  0x41, // 'A' // 7
  0x4b, // 'K' // 8
  0x2D, // '-' // 9
  0x54, // 'T' // 10
  0x41, // 'A' // 11
  0x43, // 'C' // 12
  0x4f, // 'O' // 13
};
char ble_address[4];
// FILTER SEUTP
boolean useFilter = true;
float HPfilterOutput = 0.0;
float LPfilterOutput = 0.0;
//const float HPcutoff_freq = 0.8;  //Cutoff frequency in Hz
//const float LPcutoff_freq = 8.0; // Cutoff frequency in Hz
//const float sampling_time = 0.01; //Sampling time in seconds.
Filter highPass(0.8, 0.01, IIR::ORDER::OD1, IIR::TYPE::HIGHPASS);
Filter lowPass(8.0, 0.01,IIR::ORDER::OD1);


void setup()
{
#ifdef SERIAL_LOG
    Serial.begin(9600);
    delay(100);
    Serial.println("starting...");
    Serial.println("OpenHAK v0.1.0");
    Serial.print("BMI chip ID: 0x"); Serial.println(BMI160.testConnection(),HEX);
    getMAXdeviceInfo(); // prints rev [0x00-0xFF] and device ID [0x15]
    Serial.println("getting battery...");
    getBatteryVoltage();
#endif

  splashOLED();

  ble_address =  String(getDeviceIdLow(), HEX);
  ble_address.toUpperCase();
  advdata[10] = (uint8_t)stringy.charAt(0);
  advdata[11] = (uint8_t)stringy.charAt(1);
  advdata[12] = (uint8_t)stringy.charAt(2);
  advdata[13] = (uint8_t)stringy.charAt(3);
  SimbleeBLE_advdata = advdata;
  SimbleeBLE_advdata_len = sizeof(advdata);
  SimbleeBLE.advertisementData = "OpenHAK";
  // Device Information Service strings
  SimbleeBLE.manufacturerName = "openhak";
  SimbleeBLE.hardwareRevision = "0.3.0";
  SimbleeBLE.softwareRevision = "0.1.0";
  Wire.beginOnPins(SCL_PIN, SDA_PIN);
  // change the advertisement interval?
  SimbleeBLE.advertisementInterval = bleInterval;
  SimbleeBLE.begin();
  for(int i=0; i<3; i++){
    pinMode(LEDpin[i],OUTPUT); analogWrite(LEDpin[i],255); // Enable RGB and turn them off
  }

/*
* Initialize MAX heart rate sensor
* (Sample Average, Mode, ADC Range, Sample Rate, Pulse Width, LED Current)
* Sample Average and Sample Rate are intimately entwined
*/
	sampleAve = SMP_AVE_4;
	MAX_mode = SPO2_MODE;
	sampleRange = ADC_RGE_8192;
	sampleRate = SR_400;
	pulseWidth = PW_411;
	LEDcurrent = 30;
	MAX_init(sampleAve, MAX_mode, sampleRange, sampleRate, pulseWidth, LEDcurrent);
	pinMode(MAX_INT,INPUT); // make input-pullup?

/*
 *  Setup the BMI
 */
  BMI160.begin(0, -1); // use BMI_INT1 for internal interrupt, but we're handling the interrupt so using -1
  BMI160.attachInterrupt(NULL); // use bmi160_intr for internal interrupt callback, but we're handling the interrupt so NULL
  //BMI160.setIntTapEnabled(true);
  //  NEEDS TIGHTER SETTINGS ON THE DOUBLE TAP THRESHOLD
//  BMI160.setIntDoubleTapEnabled(true);
  BMI160.setStepDetectionMode(BMI160_STEP_MODE_NORMAL);
  BMI160.setStepCountEnabled(true);
  pinMode(BMI_INT1, INPUT); // set BMI interrupt pin
  Simblee_pinWake(BMI_INT1, LOW); // use this to wake the MCU if its sleeping



const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
setTime(DEFAULT_TIME);

  //Blink the startup pattern
  digitalWrite(RED, LOW);
  delay(400);digitalWrite(GRN, LOW);digitalWrite(RED, HIGH);
  delay(400);digitalWrite(GRN, HIGH);digitalWrite(BLU, LOW);
  delay(400);digitalWrite(BLU, HIGH);
#ifdef SERIAL_LOG
	delay(400);digitalWrite(BLU, LOW);
  delay(400);digitalWrite(BLU, HIGH);
  delay(400);digitalWrite(BLU, LOW);
  delay(400);digitalWrite(BLU, HIGH);
#endif

}

//void bmi160_intr(void)
//{
// NOT USING THE BMI DEFINED ITERRUPT VECTOR
//}

void loop()
{
  if (Lazarus.lazarusArising()) {
#ifdef SERIAL_LOG
    Serial.println("Lazarus has awakened!");
#endif
    analogWrite(GRN, 100);
    delay(600);
    analogWrite(GRN, 255);
  }
  if (Simblee_pinWoke(BMI_INT1))
  {
    byte int_status = BMI160.getIntStatus0();
#ifdef SERIAL_LOG
    Serial.println("TAP has awakened!");
#endif
    Simblee_resetPinWake(BMI_INT1);
    analogWrite(RED,100);
    delay(600);
    analogWrite(RED, 255);
  }

  switch (modeNum) {
    case 0: // modeNum 0 TAKES HEART RATE, BUILDS DATA PACKET AND SENDS IT, THEN SLEEPS
#ifdef SERIAL_LOG
      thisTestTime = millis();
      Serial.println("Enter modeNum 0");
      Serial.print("Time since last here "); Serial.println(thisTestTime - thatTestTime);
      thatTestTime = thisTestTime;
#endif
      updateTime();
      lastTime = millis();
//      utc = now();  // This works to keep time incrementing that we send to the phone
//      localTime = utc + (minutesOffset/60); // This does not work to keep track of time we pring on screen??
      samples[currentSample].epoch = utc;  // Send utc time to the phone. Phone will manage timezone, etc.
      samples[currentSample].steps = BMI160.getStepCount();
      memset(arrayBeats, 0, sizeof(arrayBeats));
#ifdef SERIAL_LOG
      Serial.println("Starting HR capture");
#endif
      printOLED("Measuring Heart Rate",true);
			resetPulseVariables();
      getTempFlag = true;
			enableMAX30101(true);
      startTime = millis();
      while (captureHR(startTime)) { // captureHR will run for 30 seconds. Change?
        ;
      }
//      BUILD THE REST OF THE BLE PACKET
      samples[currentSample].hr = stats.median(arrayBeats, beatCounter);
      samples[currentSample].hrDev = stats.stdev(arrayBeats, beatCounter);
      samples[currentSample].battery = getBatteryVoltage();
      samples[currentSample].aux1 = tempInteger; // ADD MAX TEMP DATA HIGH BYTE
//                samples[currentSample].aux2 = analogRead(PIN_3); // ADD MAX TEMP DATA LOW BYTE
//                samples[currentSample].aux3 = analogRead(PIN_4);

      bpmString = ""; // clear the bpm string
      bpmString += String(samples[currentSample].hr);
      bpmString += " BPM";
      printOLED(bpmString,true);

      if (bConnected) {
        sendSamples(samples[currentSample]);
      }
      if (currentSample < 512) {
        currentSample++;
      } else {
        //TODO: Check and see if this works!
        for (int k = currentSample; k > 0; k--) {
          samples[k] = samples[k - 1];
        }
        currentSample--;
        // for(uint16_t t=0;t<512;t++){
        //   memcpy(&samples[1], &samples[0], (512-1)*sizeof(*samples));
        // }
      }
#ifdef SERIAL_LOG
      Serial.print("Samples Captured: ");
      Serial.println(currentSample);
#endif
      delay(5000);
      digitalWrite(OLED_RESET,LOW);
      awakeTime = millis() - lastTime;
      sleepTimeNow = sleepTime - (HR_TIME / 1000);
      sleepNow(sleepTimeNow);
      break;
    case 1: // modeNum 1 SEEMS TO CAPTURE THE HEART RATE DATA AND NOT DO ANYTHING TO IT
#ifdef SERIAL_LOG
      Serial.println("Enter modeNum 1");
#endif
      resetPulseVariables();
      enableMAX30101(true);
      startTime = millis();
      while (captureHR(startTime)) { // captureHR will run for 30 seconds. Change?
        ;
      }
      break;
    case 2: // modeNum 2 SWITCHES modeNum TO 0 THEN GOES TO SLEEPY
#ifdef SERIAL_LOG
      Serial.println("Enter modeNum 2");
#endif
      modeNum = 0;
      sleepTimeNow = sleepTime - (HR_TIME / 1000);
      sleepNow(sleepTimeNow);
      break;
    case 3: // modeNum 3 TRANSFERS SAMPLES
#ifdef SERIAL_LOG
      Serial.println("Enter modeNum 3");
#endif
      transferSamples();
      break;
    case 10:  // modeNum 10 SLEEPS FOR 10 SECONDS. WAITING FOR CONNECTION
#ifdef SERIAL_LOG
      Serial.println("Enter modeNum 10");
#endif
      printOLED("Sync me :)",false);  // add the advertised hex identifier
	digitalWrite(RED, LOW);
      delay(100);
      sleepNow(10);

      break;
  }
}



void sleepNow(long timeNow) {
  analogWrite(RED, 255);
  analogWrite(GRN, 255);  // shut down LEDs
  analogWrite(BLU, 255);
  digitalWrite(OLED_RESET,LOW);
  enableMAX30101(false);  // shut down MAX
  long sleepTimeNow = SLEEP_TIME - HR_TIME;
  Simblee_ULPDelay(SECONDS(timeNow));
}



String digitalClockDisplay() {
  // digital clock display of the time
  String dataString = "";
  dataString += year(localTime);
  dataString += "-";
  dataString += month(localTime);
  dataString += "-";
  dataString += day(localTime);
  dataString += " ";
  dataString += hour(localTime);
  dataString += ":";
  //Serial.print(hour());
  if (minute(localTime) < 10) {
    dataString += "0";
  }
  dataString += minute(localTime);
  dataString += ":";
  //Serial.print(hour());
  if (second(localTime) < 10) {
    dataString += "0";
  }
  dataString += second(localTime);
  return dataString;
}

uint8_t getBatteryVoltage() {
    int thisCount, lastCount;
    byte returnVal;
    for(int i=0; i<100; i++){
      lastCount = analogRead(V_SENSE);
      delay(10);
      thisCount = analogRead(V_SENSE);
#ifdef SERIAL_LOG
	      Serial.print(i); Serial.print("\t"); Serial.print(lastCount); Serial.print("\t"); Serial.println(thisCount);
#endif
      if(thisCount >= lastCount){ break; }
      delay(10);
    }
    volts = float(thisCount) * (3.0 / 1023.0);
    volts *= 2.0;
#ifdef SERIAL_LOG
      Serial.print(thisCount); Serial.print("\t"); Serial.println(volts,3);
#endif
    returnVal = byte(volts/BATT_VOLT_CONST); // convert to byte for OTA data transfer
    return returnVal;
}

void updateTime(){
    TimeChangeRule localCR = {"TCR", First, Sun, Nov, 2, minutesOffset};
    Timezone localZone(localCR, localCR);
    utc = now();    //current time from the Time Library
    localTime = localZone.toLocal(utc);
}
