

void splashOLED(){
    oled.begin();    // Initialize the OLED
    oled.flipHorizontal(true);
    oled.flipVertical(true);
    oled.clear(ALL); // Clear the display's internal memory
    oled.clear(PAGE); // Clear the buffer.

    oled.setFontType(FONT_8x16);
    oled.setCursor(0, 0);
    oled.print("OpenHAK");
    oled.setCursor(0, 24);
    oled.setFontType(FONT_5x7);
    oled.print("v");
    oled.println(VERSION);
    oled.println("murphy"); // murphifield
    oled.println("percifield");
    oled.display();
}



void printOLED(String inString, boolean printTime) {

    oled.begin();    // Initialize the OLED
    oled.flipHorizontal(true);
    oled.flipVertical(true);
    oled.clear(ALL); // Clear the display's internal memory
    oled.clear(PAGE); // Clear the buffer.


#ifdef DEBUG
    uint8_t bat = getBatteryVoltage();
    oled.setFontType(FONT_5x7);
    oled.setCursor(0, 0);
    oled.println("Battery");
    oled.println(volts);
    oled.println(bat);
    oled.display();
    delay(1000);
    oled.clear(ALL); // Clear the display's internal memory
    oled.clear(PAGE); // Clear the buffer.
#endif


    if(printTime){
      oled.setFontType(SEVEN_SEGMENT);  // (FONT_8x16);
      oled.setCursor(5, 0);
      String timeString = "";
      int hour12 = format12(hour(localTime));
      if (hour12 < 10)  {
        timeString += "0";
      }
      timeString += hour12;
      oled.setFontType(FONT_8x16);
      timeString += ":";
      oled.setFontType(SEVEN_SEGMENT);
      if (minute(localTime) < 10) {
        timeString += "0";
      }
      timeString += minute(localTime);
      oled.print(timeString);
    }

  oled.setFontType(FONT_5x7);
  oled.setCursor(0, 24);
  oled.println(inString);
  oled.display();
}

int format12(int h){
    if( h == 0 )
    return 12; // 12 midnight
    else if( h  > 12)
    return h - 12 ;
    else
    return h ;
}
