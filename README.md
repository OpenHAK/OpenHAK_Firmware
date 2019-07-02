# OpenHAK Firmware
Latest Firmware release for targeting OpenHAK **beta** hardware.The OpenHAK is built around a Simblee radio module. I know, they don't exist, but we happen to have a small pile and hope to crowdfund a way to get rid of them. 

## Libraries, etc.
This is an Arduino .INO file, and it uses tabs so make sure you have all the tabs

`OpenHAK_Firmware_vxxx | BLE_Stuff | HeartRate_Stuff | MAX_Stuff | OpenHAK.h`

You will also need to ensure that you have the **board files** for the OpenHAK. Please go [here](https://github.com/OpenHAK/Docs/blob/master/Getting%20Started%20With%20Arduino.md) to learn how to do that and do it. When it doubt, reboot.

### Libraries You Will Need
We're using a filter library for DSP on the MAX30101 signal. It's called [libFilter](//https://github.com/MartinBloedorn/libFilter). Download it and install it in your `Documents/Arduino/libraries` folder.

You also need a BMI160 library. We're using [hanyazou's](https://github.com/hanyazou/BMI160-Arduino) code, but it's old and we're open to new ideas.

We modified a number-cruncher library called [QuickStats](https://github.com/OpenHAK/QuickStats) to use only integer values. It seems to  work, you need it, download it.

Then you need to get [Lazarus](https://github.com/OpenHAK/lazarus). It lets us put the Simblee into deep sleep so the battery doesn't run dry.

The rest is a piece of cake. The `<Timezone.h>` library is available though the Library Manager tool, and the rest comes with the OpenHAK board files that you installed in the previous step.

## Status
Remains a work in progress in the Summer of '19 in anticipation of being the badge for the BoiHacking Village at DEFCON27. Work is also being done toward a crowdfunding campaign. The BioHacking Village Badge will be sporting a display. Notes about that will be forthcoming, but we're likely to use well known OLED and graphics libraries.

## Structure

It's basically a state machine that tries to sleep as much as it can. If it hasn't been connected, it will idle and advertise until it gets a connection. Once connected it will cycle through a sleep state and awake state. It will sleep for 10 minutes before it wakes itself up. When it's awake, it takes a heart rate reading for 15 seconds, captures any steps from the BMI, and shoves it all to the phone if it's connected. It's also possible to wake up from a `double tap` signal that the BMI feels (this needs work). Upon a `double tap` wake up, the OpenHAK does the same routine: heart rate, steps count, etc.

We've allocated enough memory to hold up to 3 days worth of data on the OpenHAK. Since phones can move in and out of range, we make sure that when we connect back up we update your files. 

## Known Issues and Future Features
We're looking at replacements for the current BMI library. Happy to hear your thoughts.

Heart rate sensing on the wrist is still a mischievous endeavor. Our settings for the MAX and the band pass filter could be optimized.

Needs option to connect with BTE Heart Rate Service, and BTE Pedometer Service for connection to other apps.



 