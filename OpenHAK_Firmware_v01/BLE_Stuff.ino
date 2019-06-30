
void SimbleeBLE_onConnect()
{
  bConnected = true;
  //digitalWrite(BLU, LOW);
  //mode = 1;
  //Lazarus.ariseLazarus(); // Tell Lazarus to arise.
  //analogWrite(BLU,10);
}

void SimbleeBLE_onDisconnect()
{
  bConnected = false;
  //mode = 2;
  //digitalWrite(BLU, HIGH);
  //analogWrite(BLU,255);
  //pinMode(BLU,INPUT);
}


void SimbleeBLE_onReceive(char *data, int len) {
  // if the first byte is 0x01 / on / true
  //Serial.print("Received data over BLE ");
  //Serial.println(len);
  Lazarus.ariseLazarus();
  mode = data[0];
  if (mode == 10) {
    if (len >= 5) {
      unsigned long myNum = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
      setTime(myNum);
      timeZoneOffset = 0xE2; //(data[5]);  // Phone sends UTC offset
      minutesOffset = timeZoneOffset;
      minutesOffset *= 10;
      TimeChangeRule localCR = {"TCR", First, Sun, Nov, 2, minutesOffset};
      Timezone localZone(localCR, localCR);
      utc = now();    //current time from the Time Library
      localTime = localZone.toLocal(utc);
      setTime(utc);

      mode = 2;
    }
  }

}


void transferSamples() {
#ifdef DEBUG
  Serial.println("Starting History transfer");
#endif
  for (int i = 0; i < currentSample; i++) {
    if (bConnected) {
      sendSamples(samples[i]);
    }
  }
  mode = 0;
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
  while (!SimbleeBLE.send(data, 9))
    ; // all tx buffers in use (can't send - try again later)
}
