
void SimbleeBLE_onConnect()
{
  bConnected = true;
  analogWrite(BLU, 100);
  Lazarus.ariseLazarus(); // Tell Lazarus to arise.
#ifdef SERIAL_LOG
  Serial.println("ble connected"); 
#endif
  delay(100);
  analogWrite(BLU,255);
}

void SimbleeBLE_onDisconnect()
{
  bConnected = false;
  modeNum = 2;
  analogWrite(GRN,100);
#ifdef SERIAL_LOG
  Serial.println("ble disconnected"); 
#endif
  delay(100);
  analogWrite(GRN,255);
}


void SimbleeBLE_onReceive(char *data, int len) {
  Lazarus.ariseLazarus();
#ifdef SERIAL_LOG
  Serial.print("Received data over BLE ");
  Serial.print(len); Serial.println(" bytes");
#endif
  // the first byte says what mode to be in
  modeNum = data[0];
  switch (modeNum){
    case 10:
      if (len >= 5) {
        unsigned long thyme = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
        setTime(thyme);
        //timeZoneOffset = 0xE2; //(data[5]);  // Phone sends UTC offset
        timeZoneOffset = (data[5]);
        minutesOffset = timeZoneOffset;
        minutesOffset *= 10;
        TimeChangeRule localCR = {"TCR", First, Sun, Nov, 2, minutesOffset};
        Timezone localZone(localCR, localCR);
        utc = now();    //current time from the Time Library
        localTime = localZone.toLocal(utc);
        setTime(utc);
        modeNum = 0;
      }
      break;
    case 3:
      if(currentSample < 1){ // if we are just starting out
        modeNum = 0;
      }
      break;
    default:
      break;
    }
}

void transferSamples() {
#ifdef SERIAL_LOG
  Serial.println("Starting History transfer");
#endif
  for (int i = 0; i < currentSample; i++) {
    if (bConnected) {
      sendSamples(samples[i]);
    }
  }
  modeNum = 2; // WHAT TO DO HERE? 
}


void sendSamples(Payload sample) {
  char data[20];
  data[0] = (sample.epoch >> 24) & 0xFF;
  data[1] = (sample.epoch >> 16) & 0xFF;
  data[2] = (sample.epoch >> 8) & 0xFF;
  data[3] = sample.epoch & 0xFF;
  data[4] = (sample.steps >> 8) & 0xFF;
  data[5] = sample.steps & 0xFF;
  data[6] = sample.hr;
  data[7] = sample.hrDev;
  data[8] = sample.battery;
  data[9] = sample.aux1;
  data[10] = sample.aux2;
  data[11] = sample.aux3;
  // send is queued (the ble stack delays send to the start of the next tx window)
  while (!SimbleeBLE.send(data, 12))
    ; // all tx buffers in use (can't send - try again later)
}
