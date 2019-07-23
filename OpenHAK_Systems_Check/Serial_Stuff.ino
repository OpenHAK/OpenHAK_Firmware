

void eventSerial(){
  while(Serial.available()){
    byte inByte = Serial.read();
    uint16_t intSource;
    switch(inByte){
      case 'h':
        printHelpToSerial();
        break;
      case 'b':
        if(DEBUG) Serial.println("start running");
        is_running = true;
        analogWrite(RED,255); analogWrite(BLU,255); analogWrite(GRN,255); 
        enableMAX30102(true);
        thatTestTime = micros();
        break;
      case 's':
        if(DEBUG) Serial.println("stop running");
        is_running = false;
        enableMAX30102(false);
        break;
      case 't':
        MAX30102_writeRegister(TEMP_CONFIG,0x01);
        break;
      case 'i':
        intSource = MAX30102_readShort(STATUS_1);
        if(DEBUG) Serial.print("intSource: 0x"); Serial.println(intSource,HEX);
        break;
      case 'v':
        getBMI_chipID();
        getMAXdeviceInfo();
        break;
      case '?':
        printAllRegisters();
        break;
      case 'f':
        useFilter = false;
        break;
      case 'F':
        useFilter = true;
        break;
      case '1':
        redAmp++; if(redAmp > 50){redAmp = 50;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case '2':
        redAmp--; if(redAmp < 1){redAmp = 0;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case '3':
        irAmp++; if(irAmp > 50){irAmp = 50;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case '4':
        irAmp--; if(irAmp < 1){irAmp = 0;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case 'r':
        LEDvalue[0] = 0; LEDvalue[1] = 255; LEDvalue[2] = 255;
        for(int i=0; i<3; i++){
          analogWrite(LEDpin[i],LEDvalue[i]);        
        }
        colorWheelDegree = 0;
        rainbow = true;
        break;
      case 'R':
        rainbow = false;
        break;
      case 'p':
        getBatteryVoltage();
        break;
      case 'o':
        if(DEBUG){ Serial.print("MAX_INT: "); Serial.println(digitalRead(MAX_INT)); }
        break;
      case 'x':
        MAX_init(sampleAve, mode, sampleRange, sampleRate, pulseWidth, LEDcurrent); 
        break;
      case 'q':
        serviceInterrupts();
        break;
      default:
        if(DEBUG){ Serial.print("Serial Event got: "); Serial.write(inByte); Serial.println(); }
        break;
      
    }
  }
}


//Print out all of the commands so that the user can see what to do
//Added: Chip 2016-09-28
void printHelpToSerial() {
  if(DEBUG) {
    Serial.println(F("Commands:"));
    Serial.println(F("   'h'  Print this help information on available commands"));
    Serial.println(F("   'b'  Start the thing running at the sample rate selected"));
    Serial.println(F("   's'  Stop the thing running"));
    Serial.println(F("   't'  Initiate a temperature conversion. This should work if 'b' is pressed or not"));
    Serial.println(F("   'i'  Query the interrupt flags register. Not really useful"));
    Serial.println(F("   'v'  Verify the device by querying the RevID and PartID registers (hex 6 and hex 15 respectively)"));
    Serial.println(F("   '1'  Increase red LED intensity"));
    Serial.println(F("   '2'  Decrease red LED intensity"));
    Serial.println(F("   '3'  Increase IR LED intensity"));
    Serial.println(F("   '4'  Decrease IR LED intensity"));
    Serial.println(F("   '?'  Print all registers"));
    Serial.println(F("   'F'  Turn on filters"));
    Serial.println(F("   'f'  Turn off filters"));
    Serial.println(F("   'p'  Query battery Voltage"));
    Serial.println(F("   'o'  Read MAX_INT pin value"));
    Serial.println(F("   'x'  Soft Reset MAX30101"));
    Serial.println(F("   'q'  Service Interrupts"));
  }
}


void serialAmps(){
  if(DEBUG){
    Serial.print("PA\t");
    Serial.print(redAmp); printTab(); Serial.print(irAmp); printTab(); Serial.println(grnAmp);
  }
}
