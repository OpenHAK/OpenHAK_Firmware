/*
  developing a library to control the MAX30102 Sp02 Sensor
*/


void getBMI_chipID(){ // should return 0xD1
  char chipID = BMI160_readRegister(CHIP_ID);
  Serial.print("BMI chip ID: 0x"); Serial.println(chipID,HEX);
}

// reads one register from the MAX30102
char BMI160_readRegister(char reg){
  char inChar;
  Wire.beginTransmission(BMI_ADD);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(BMI_ADD,1);
  while(Wire.available()){
    inChar = Wire.read();
  }
 return inChar;
}

short BMI160_read16bit(char startReg){
  char inChar[2];
  short shorty;
  int byteCounter = 0;
  Wire.beginTransmission(BMI_ADD);
  Wire.write(startReg);
  Wire.endTransmission(false);
  Wire.requestFrom(BMI_ADD,2);
  while(Wire.available()){
    inChar[byteCounter] = Wire.read();
    byteCounter++;
  }
  shorty = (inChar[0]<<8) | inChar[1];
 return shorty;
}

void BMI160_writeRegister(char reg, char setting){
  Wire.beginTransmission(BMI_ADD);
  Wire.write(reg);
  Wire.write(setting);
  Wire.endTransmission(true);
}
