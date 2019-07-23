/*
  developing a library to control the MAX30102 Sp02 Sensor
*/


void getBMI_chipID(){ // should return 0xD1
  byte chipID = BMI160_readRegister(CHIP_ID);
  if(DEBUG){ Serial.print("BMI chip ID: 0x"); Serial.println(chipID,HEX); }
}

// reads one register from the MAX30102
byte BMI160_readRegister(byte reg){
  byte inByte;
  Wire.beginTransmission(BMI_ADD);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(BMI_ADD,1);
  while(Wire.available()){
    inByte = Wire.read();
  }
  Wire.endTransmission(true);
 return inByte;
}

short BMI160_read16bit(byte startReg){
  byte inByte[2];
  short shorty;
  int byteCounter = 0;
  Wire.beginTransmission(BMI_ADD);
  Wire.write(startReg);
  Wire.endTransmission(false);
  Wire.requestFrom(BMI_ADD,2);
  while(Wire.available()){
    inByte[byteCounter] = Wire.read();
    byteCounter++;
  }
  Wire.endTransmission(true);
  shorty = (inByte[0]<<8) | inByte[1];
 return shorty;
}

void BMI160_writeRegister(byte reg, byte setting){
  Wire.beginTransmission(BMI_ADD);
  Wire.write(reg);
  Wire.write(setting);
  Wire.endTransmission(true);
}

//************************************************
/*
 * Initialize MAX heart rate sensor
 * Sample Average
   * SMP_AVE_1, SMP_AVE_2, SMP_AVE_4, SMP_AVE_8, SMP_AVE_16, SMP_AVE_32
 * Mode
   * HR_MODE, SPO2_MODE, MULTI_MODE
 * ADC Range
   * ADC_RGE_2048, ADC_RGE_4096, ADC_RGE_8192, ADC_RGE_16348
 * Sample Rate
   * SR_50 0x00, SR_100, SR_200, SR_400, SR_800, SR_1000, SR_1600, SR_3200
 * Pulse Width
   * PW_69, PW_118, PW_215, PW_411
 * LED Current
   * 0 - 50mA
 * 
 *  Sample Average effects Sample Rate
   *  sa of 4 and sr of 400 = actual sample rate of 100
 *  Pulse Width effects resolution
   *  69 = 15bit, 118 = 16bit, 215 = 17bit, 411 = 18bit
 */
void MAX_init(byte sa, byte m, byte adcr, byte sr, byte pw, int a){
  byte setting;
  // reset the MAX30102
  setting = RESET;
  MAX30102_writeRegister(MODE_CONFIG,setting);
  delay(50);
  // set fifo configuration
  setting = (sa | ROLLOVER_EN | 0x0F) & 0xFF;
  MAX30102_writeRegister(FIFO_CONFIG,setting);
  // set mode configuration put device in shutdown to set registers
  // 0x82 = shutdown, Heart Rate mode
  // 0x83 = shutdown, Sp02 mode
  setting = (SHUTDOWN | m) & 0xFF;
  MAX30102_writeRegister(MODE_CONFIG,setting);
  // set Sp02 configuration
  setting = (adcr | sr | pw) & 0xFF;
  MAX30102_writeRegister(SPO2_CONFIG,setting);
  // set LED pulse amplitude (current in mA)
  setLEDamplitude(a, a, a);
  // enable interrupts
  short interruptSettings = ( (PPG_RDY<<8) | (ALC_OVF<<8) | TEMP_RDY ) & 0xFFFF; // (A_FULL<<8) |
  MAX_setInterrupts(interruptSettings);
}

void enableMAX30102(boolean activate){
  byte setting = mode;
  zeroFIFOpointers();
  if(!activate){ setting |= 0x80; }
  MAX30102_writeRegister(MODE_CONFIG,setting);
}

void zeroFIFOpointers(){
  MAX30102_writeRegister(FIFO_WRITE,0x00);
  MAX30102_writeRegister(OVF_COUNTER,0x00);
  MAX30102_writeRegister(FIFO_READ,0x00);
}

// report RevID and PartID for verification
void getMAXdeviceInfo(){
  byte revID = MAX30102_readRegister(REV_ID);
  byte partID = MAX30102_readRegister(PART_ID);
  if(DEBUG){ Serial.print("MAX rev: 0x");
    Serial.println(revID,HEX);
    Serial.print("MAX part ID: 0x");
    Serial.println(partID,HEX); 
  }
}

// read interrupt flags and do the work to service them
void serviceInterrupts(){
    MAX_interrupt = false;  // reset this software flag
    interruptFlags = MAX_readInterrupts();  // read interrupt registers
    if((interruptFlags & (A_FULL<<8)) > 0){ // FIFO Almost Full
      if(DEBUG){ Serial.println("A_FULL"); }
      // do something?
    }
    if((interruptFlags & (PPG_RDY<<8)) > 0){ // PPG data ready
//      Serial.println("PPG_RDY");
//      readPointers();
      readPPG();  // read the light sensor data that is available
      serialPPG(); // send the RED and/or IR data
    }
    if((interruptFlags & (ALC_OVF<<8)) > 0){ // Ambient Light Cancellation Overflow
      if(DEBUG){ Serial.println("ALC_OVF"); }
      // do something?
    }
    if((interruptFlags & (TEMP_RDY)) > 0){  // Temperature Conversion Available
//      Serial.println("TEMP_RDY");
      readTemp();
      if(DEBUG){ printTemp(); }
    }
    if((interruptFlags &(PWR_RDY<<8)) > 0){
      if(DEBUG){ Serial.println("power up"); }
    }
}

void serveInterrupts(uint16_t flags){
    MAX_interrupt = false;  // reset this software flag
    if((flags & (A_FULL<<8)) > 0){ // FIFO Almost Full
      if(DEBUG){ Serial.println("A_FULL"); }
      // do something?
    }
    if((flags & (PPG_RDY<<8)) > 0){ // PPG data ready
//      Serial.println("PPG_RDY");
//      readPointers();
      readPPG();  // read the light sensor data that is available
      serialPPG(); // send the RED and/or IR data
    }
    if((flags & (ALC_OVF<<8)) > 0){ // Ambient Light Cancellation Overflow
      if(DEBUG){ Serial.println("ALC_OVF"); }
      // do something?
    }
    if((flags & (TEMP_RDY)) > 0){  // Temperature Conversion Available
//      Serial.println("TEMP_RDY");
      readTemp();
      if(DEBUG){ printTemp(); }
    }
    if((flags &(PWR_RDY<<8)) > 0){
      if(DEBUG){ Serial.println("power up"); }
    }
}

int readPointers(){
  int diff;
  readPointer = MAX30102_readRegister(FIFO_READ);
  writePointer = MAX30102_readRegister(FIFO_WRITE);
  if(writePointer > readPointer){
    diff = int(writePointer - readPointer);
  }else if(readPointer > writePointer){
    diff = int((32 - readPointer) + writePointer);
  }
  if(DEBUG){
    Serial.print("point"); printSpace();
    Serial.print(writePointer,DEC); printSpace();
    Serial.print(readPointer,DEC); printSpace();
    Serial.println(diff);
  }
  return diff;
}

//  read die temperature to compansate for RED LED
byte tempInteger;
byte tempFraction;
void readTemp(){
  tempInteger = MAX30102_readRegister(TEMP_INT);
  tempFraction = MAX30102_readRegister(TEMP_FRAC);
  Celcius = float(tempInteger);
  Celcius += (float(tempFraction)/16);
  Fahrenheit = Celcius*1.8 + 32;
}

void printTemp(){
  if (DEBUG) {
    printTab(); // formatting...
    Serial.print(Celcius,3); Serial.println("*C");
  }
}


void readPPG(){
//  sampleTimeTest();
  sampleCounter++;
  if(sampleCounter > 200){ sampleCounter = 0; }
  // use the FIFO read and write pointers?
  readFIFOdata();

}

// send PPG value(s) via Serial port
void serialPPG(){
  if (DEBUG) {
//    Serial.println();  // formatting...
    Serial.print(sampleCounter,DEC); printTab();
    Serial.print(REDvalue); printTab();
    Serial.print(IRvalue);
  } else {
    if(useFilter){

    } else {
      Serial.print(REDvalue + IRvalue);
//      printSpace();
//      Serial.print(IRvalue);
    }
    Serial.println();
  }
}

// read in the FIFO data three bytes per ADC result
void readFIFOdata(){
  int bytesToGet;
  switch(mode){
    case HR_MODE:
      bytesToGet = 3; break;
    case SPO2_MODE:
      bytesToGet = 6; break;
    case MULTI_MODE:
      bytesToGet = 9; break;
    default:
      if(DEBUG){ Serial.println("mode not defined"); }
      return;
      break;
  }
  byte dataByte[bytesToGet];
  int byteCounter = 0;
  Wire.beginTransmission(MAX_ADD);
  Wire.write(FIFO_DATA);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,bytesToGet);
  while(Wire.available()){
    dataByte[byteCounter] = Wire.read();
    byteCounter++;
  }
  Wire.endTransmission(true);
  REDvalue = 0L; 
  REDvalue = (dataByte[0] & 0xFF); REDvalue <<= 8;
  REDvalue |= dataByte[1]; REDvalue <<= 8;
  REDvalue |= dataByte[2];
  REDvalue &= 0x0003FFFF;
  if(bytesToGet > 3){
    IRvalue = 0L;
    IRvalue = (dataByte[3] & 0xFF); IRvalue <<= 8;
    IRvalue |= dataByte[4]; IRvalue <<= 8;
    IRvalue |= dataByte[5];
    IRvalue &= 0x0003FFFF;
  }
  if(bytesToGet > 6){
    GRNvalue = 0L;
    GRNvalue = (dataByte[6] & 0xFF); GRNvalue <<= 8;
    GRNvalue |= dataByte[7]; GRNvalue <<= 8;
    GRNvalue |= dataByte[8];
    GRNvalue &= 0x0003FFFF;
  }
  switch(pulseWidth){
    case PW_69:
      REDvalue >>= 3;
      IRvalue >>= 3;
      GRNvalue >>= 3;
      break;
    case PW_118:
      REDvalue >>= 2;
      IRvalue >>= 2;
      GRNvalue >>= 2;
      break;
    case PW_215:
      REDvalue >>= 1;
      IRvalue >>= 1;
      GRNvalue >>= 1;
      break;
    default:
      break;
  }
  if(useFilter){

  }
}

// set the current amplitude for the LEDs
// currently uses the same setting for both RED and IR
// should be able to adjust each dynamically...
void setLEDamplitude(int Ir, int Iir, int Ig){
  Ir *= 1000; Iir *= 1000; Ig *= 1000;
  Ir /= 196; Iir /= 196; Ig /= 196;
  byte currentR = Ir & 0xFF;
  byte currentIR = Iir & 0xFF;
  byte currentG = Ig & 0xFF;
  // RED LED will always be on in all modes
  MAX30102_writeRegister(RED_PA,currentR);
  if(mode == SPO2_MODE){
    MAX30102_writeRegister(IR_PA,currentIR);
  }else{
    MAX30102_writeRegister(IR_PA,0x00);
  }
  if(mode == MULTI_MODE){
    MAX30102_writeRegister(GRN_PA,currentG);
  }else{
    MAX30102_writeRegister(GRN_PA,0x00);
  }
}

// measures time between samples for verificaion purposes
void sampleTimeTest(){
  thisTestTime = micros();
  if(DEBUG){ Serial.print("S\t"); Serial.println(thisTestTime - thatTestTime); }
  thatTestTime = thisTestTime;
}

// set the desired interrupt flags
void MAX_setInterrupts(uint16_t setting){
  byte highSetting = (setting >> 8) & 0xFF;
  byte lowSetting = setting & 0xFF;
  Wire.beginTransmission(MAX_ADD);
  Wire.write(ENABLE_1);
  Wire.write(highSetting);
  Wire.write(lowSetting);
  Wire.endTransmission(true);
}



// reads the interrupt status registers
// returns a 16 bit value
uint16_t MAX_readInterrupts(){
  byte inByte = MAX30102_readRegister(STATUS_1);
  uint16_t inShort = inByte << 8;
  inByte = MAX30102_readRegister(STATUS_2);
  inShort |= inByte;
  return inShort;
}

// writes one register to the MAX30102
void MAX30102_writeRegister(byte reg, byte setting){
  Wire.beginTransmission(MAX_ADD);
  Wire.write(reg);
  Wire.write(setting);
  Wire.endTransmission(true);
}

// reads one register from the MAX30102
byte MAX30102_readRegister(byte reg){
  byte inByte;
  Wire.beginTransmission(MAX_ADD);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,1);
  while(Wire.available()){
    inByte = Wire.read();
  }
  Wire.endTransmission(true);
 return inByte;
}

// reads two successive registers from the MAX30102
short MAX30102_readShort(byte startReg){
  byte inByte[2];
  short shorty;
  int byteCounter = 0;
  Wire.beginTransmission(MAX_ADD);
  Wire.write(startReg);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,2);
  while(Wire.available()){
    inByte[byteCounter] = Wire.read();
    byteCounter++;
  }
  Wire.endTransmission(true);
  shorty = (inByte[0]<<8) | inByte[1];
 return shorty;
}

// prints out register values
void printAllRegisters(){
  Wire.beginTransmission(MAX_ADD);
  Wire.write(STATUS_1);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,7);
  readWireAndPrintHex(STATUS_1);
  Wire.endTransmission(true);
  Wire.beginTransmission(MAX_ADD);
  Wire.write(FIFO_CONFIG);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,11);
  readWireAndPrintHex(FIFO_CONFIG);
  Wire.endTransmission(true);
  Wire.beginTransmission(MAX_ADD);
  Wire.write(TEMP_INT);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,3);
  readWireAndPrintHex(TEMP_INT);
  Wire.endTransmission(true);

  Wire.beginTransmission(MAX_ADD);
  Wire.write(REV_ID);
  Wire.endTransmission(false);
  Wire.requestFrom(MAX_ADD,2);
  readWireAndPrintHex(REV_ID);
  Wire.endTransmission(true);
}

// helps to print out register values
void readWireAndPrintHex(byte startReg){
  byte inByte;
  while(Wire.available()){
    inByte = Wire.read();
    printRegName(startReg); startReg++;
    Serial.print("0x"); 
    if(inByte < 0x10){ Serial.print("0"); }
    Serial.println(inByte,HEX);
  }
}



// helps with verbose feedback
void printRegName(byte regToPrint){

  switch(regToPrint){
    case STATUS_1:
      Serial.print("STATUS_1\t"); break;
    case STATUS_2:
      Serial.print("STATUS_2\t"); break;
    case ENABLE_1:
      Serial.print("ENABLE_1\t"); break;
    case ENABLE_2:
      Serial.print("ENABLE_2\t"); break;
    case FIFO_WRITE:
      Serial.print("FIFO_WRITE\t"); break;
    case OVF_COUNTER:
      Serial.print("OVF_COUNTER\t"); break;
    case FIFO_READ:
      Serial.print("FIFO_READ\t"); break;
    case FIFO_DATA:
      Serial.print("FIFO_DATA\t"); break;
    case FIFO_CONFIG:
      Serial.print("FIFO_CONFIG\t"); break;
    case MODE_CONFIG:
      Serial.print("MODE_CONFIG\t"); break;
    case SPO2_CONFIG:
      Serial.print("SPO2_CONFIG\t"); break;
    case RED_PA:
      Serial.print("RED_PA\t\t"); break;
    case IR_PA:
      Serial.print("IR_PA\t\t"); break;
    case GRN_PA:
      Serial.print("GRN_PA\t\t"); break;
    case LED4_PA:
      Serial.print("LED4_PA\t"); break;
    case MODE_CNTRL_1:
      Serial.print("MODE_CNTRL_1\t"); break;
    case MODE_CNTRL_2:
      Serial.print("MODE_CNTRL_2\t"); break;
    case TEMP_INT:
      Serial.print("TEMP_INT\t"); break;
    case TEMP_FRAC:
      Serial.print("TEMP_FRAC\t"); break;
    case TEMP_CONFIG:
      Serial.print("TEMP_CONFIG\t"); break;
    case REV_ID:
      Serial.print("REV_ID\t\t"); break;
    case PART_ID:
      Serial.print("PART_ID\t"); break;
    default:
      Serial.print("RESERVED\t"); break;
  }
}

// formatting...
void printTab(){
  Serial.print("\t");
}

void printSpace(){
  Serial.print(" ");
}
