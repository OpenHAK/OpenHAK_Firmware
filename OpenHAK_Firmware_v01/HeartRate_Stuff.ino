#define FADE_LEVEL_PER_SAMPLE 10
#define MAX_FADE_LEVEL 100
#define MIN_FADE_LEVEL 255
#define MIN_IBI 300
#define THRESH_SETTING 0
#define PULSE_SAMPLE_INTERVAL 10

    int FadePin = RED;            // pin to fade on beat

    // Pulse detection output variables.
    int BPM;                // int that holds Beats Per Minute value
    int IBI;                // int that holds the time interval (ms) between beats! Must be seeded!
    boolean Pulse;          // "True" when User's live heartbeat is detected. "False" when not a "live beat".
    boolean QS;             // The start of beat has been detected and not read by the Sketch.
    int FadeLevel;          // brightness of the FadePin, in scaled PWM units. See FADE_SCALE
    int amp;                         // used to hold amplitude of pulse waveform, seeded (sample value)
    unsigned long lastBeatTime;      // used to find IBI. Time (sampleCounter) of the previous detected beat start.
    unsigned long pulseSampleCounter;     // used to determine pulse timing. Milliseconds since we started.
    int P;                           // used to find peak in pulse wave, seeded (sample value)
    int T;                           // used to find trough in pulse wave, seeded (sample value)
    int thresh;                      // used to find instant moment of heart beat, seeded (sample value)
    boolean firstBeat;               // used to seed rate array so we startup with reasonable BPM
//    boolean secondBeat;              // used to seed rate array so we startup with reasonable BPM



boolean captureHR(uint32_t startTime) {
  if (millis() - startTime > HR_TIME) {
#ifdef SERIAL_LOG
    Serial.println("HR capture done");
#endif
		enableMAX30101(false);
    return(false);
  }
	  interruptFlags = MAX_readInterrupts();
    if(interruptFlags > 0){
    serveInterrupts(interruptFlags); // go see what woke us up, and do the work
		if((interruptFlags & PPG_RDY << 8) > 0){
			while (SimbleeBLE.radioActive) {
				;
			}
    	findBeat(LPfilterOutput);
    	if(checkQS()){
				#ifdef SERIAL_LOG
				  printBPM();
        #endif
				arrayBeats[beatCounter] = BPM; // keep track of all the beats we find
		    beatCounter++;
    	}
      if(sampleCounter == 0x00){  // rolls over to 0 at 200
//       MAX30101_writeRegister(TEMP_CONFIG,0x01); // take temperature
      }
		}
   }
  return(true);
}


/*
 *  THIS CODE LIFTED FROM THE PULSESENSOR PLAYGROUND LIBRARY
 */



void resetPulseVariables(){
  QS = false;
  BPM = 0;
  IBI = 750;                  // 750ms per beat = 80 Beats Per Minute (BPM)
  Pulse = false;
  pulseSampleCounter = 0;
  lastBeatTime = 0;
  P = 0;
  T = 0;
  thresh = THRESH_SETTING;     // threshold a little above the trough
  amp = 100;                  // beat amplitude 1/10 of input range.
  firstBeat = true;           // looking for the first beat
//  secondBeat = false;         // not yet looking for the second beat in a row
  FadeLevel = 0; // LED is dark.
  beatCounter = 0;
}


void findBeat(float Signal){  // this takes 120uS max
//  thatTestTime = micros();  // USE TO TIME BEAT FINDER
  pulseSampleCounter += PULSE_SAMPLE_INTERVAL;         // keep track of the time in mS with this variable
  int N = pulseSampleCounter - lastBeatTime;      // monitor the time since the last beat to avoid noise

  // Fade the Fading LED
  FadeLevel = FadeLevel + FADE_LEVEL_PER_SAMPLE;
  FadeLevel = constrain(FadeLevel, MAX_FADE_LEVEL, MIN_FADE_LEVEL);
  FadeHeartbeatLED();

  //  find the peak and trough of the pulse wave
  if (Signal > thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal > P) {                        // P is the peak
      P = Signal;                            // keep track of highest point in pulse wave
    }
  }

  if (Signal < thresh && Signal < T) {       // thresh condition helps avoid noise
    T = Signal;                              // T is the trough
  }                                          // keep track of lowest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // Signal surges DOWN in value every time there is a pulse
  if (N > 250) {                             // avoid high frequency noise
    if ( (Signal < thresh) && (Pulse == false) && (N > (IBI / 5) * 3) ) { // && (amp > 50)) {
      IBI = pulseSampleCounter - lastBeatTime;    // measure time between beats in mS
      if(IBI < MIN_IBI){ return; }
      Pulse = true;                          // set the Pulse flag when we think there is a pulse
      lastBeatTime = pulseSampleCounter;          // keep track of time for next pulse

//      if (secondBeat) {                      // if this is the second beat, if secondBeat == TRUE
//        secondBeat = false;                  // clear secondBeat flag
//      }

      if (firstBeat) {                       // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
//        secondBeat = true;                   // set the second beat flag
        // IBI value is unreliable so discard it
        return;
      }

      BPM = 60000 / IBI; //runningTotal;             // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag (we detected a beat)
      FadeLevel = 0;                          // re-light that LED.
    }
  }

  if (Signal > thresh && Pulse == true) {  // when the values are going down, the beat is over
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                  // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500) {                          // if 2.5 seconds go by without a beat
    resetPulseVariables();
  }
//      thisTestTime = micros(); Serial.println(thisTestTime - thatTestTime);
}

void FadeHeartbeatLED(){
  analogWrite(FadePin, FadeLevel);
}

void printBeatData(){
#ifdef SERIAL_LOG
  Serial.print("BPM: "); Serial.print(BPM); printTab();
  Serial.print("IBI: "); Serial.print(IBI); printTab();
  Serial.print("thresh: "); Serial.print(thresh); printTab();
  Serial.print("amp: "); Serial.print(amp); printTab();
  Serial.println();
#endif
}

void printBPM(){
#ifdef SERIAL_LOG
  Serial.print("BPM: "); Serial.println(BPM);
#endif
}

boolean checkQS(){
  if(QS){
    QS = false;
    return true;
  }
  return false;
}
