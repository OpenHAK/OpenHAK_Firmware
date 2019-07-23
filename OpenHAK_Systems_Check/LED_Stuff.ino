

void rainbowLEDs(){
  if(millis()-rainbowTimer > rainbowTime){
    rainbowTimer = millis();
    colorWheelDegree++;
    
    if(colorWheelDegree > 360){ colorWheelDegree = 1; }
    
    if(colorWheelDegree > 0 && colorWheelDegree < 61){
      LEDvalue[1] = map(colorWheelDegree,1,60,254,0);
      writeLEDvalues(); // g fades up while r is up b is down
    } else if(colorWheelDegree > 60 && colorWheelDegree < 121){
      LEDvalue[0] = map(colorWheelDegree,61,120,0,254);
      writeLEDvalues(); // r fades down while g is up b is down
    } else if(colorWheelDegree > 120 && colorWheelDegree < 181){
      LEDvalue[2] = map(colorWheelDegree,121,180,254,0);
      writeLEDvalues(); // b fades up while g is up r is down
    } else if(colorWheelDegree > 180 && colorWheelDegree < 241){
      LEDvalue[1] = map(colorWheelDegree,181,240,0,254);
      writeLEDvalues(); // g fades down while b is up r is down
    } else if(colorWheelDegree > 240 && colorWheelDegree < 301){
      LEDvalue[0] = map(colorWheelDegree,241,300,254,0);
      writeLEDvalues(); // r fades up while b is up g is down
    } else if(colorWheelDegree > 300 && colorWheelDegree < 361){
      LEDvalue[2] = map(colorWheelDegree,301,360,0,254);
      writeLEDvalues(); // b fades down while r is up g is down
    }
   
  } 
}

void pulse(int led){  // needs help
  if(millis() - LED_fadeTimer > LED_fadeTime){
    if(falling){
      fadeValue-=10;
      if(fadeValue <= 100){
        falling = false;
        fadeValue = 100;
      }
    }else{
      fadeValue+=10;
      if(fadeValue >= 254){
        falling = true;
        fadeValue = 254;
      }
    }
    analogWrite(led,fadeValue);
    LED_fadeTimer = millis();
  }
}

void blinkLEDs(){
  if(millis() - LED_blinkTimer > LED_blinkTime){
    LED_blinkTimer = millis();
    LEDcounter++;
    if(LEDcounter>2){ LEDcounter = 0; }
    for(int i=0; i<3; i++){
      if(i == LEDcounter){
        analogWrite(LEDpin[i],200);
      }else{
        analogWrite(LEDpin[i],255);
      }
    }
  }
}

void writeLEDvalues(){  
  for(int i=0; i<3; i++){
    analogWrite(LEDpin[i],LEDvalue[i]);
  }
}
