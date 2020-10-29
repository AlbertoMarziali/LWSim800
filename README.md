## A Light Weight SIM800l Library Module for Arduino
This is a light weight lib for comunicating with SIM800L gsm module.
It's extremely light and simple but full of security checks to 
guarantee the stability in every situation. 

It's based on another lib, the BareBoneSim800L, made by Ayo Ayibiowu. 
Props to him!

The main differences are:
+ Completely avoids the use of Strings, replaced by a single, preallocated, char array.
+ Removes every function apart SMS-related ones.
+ Almost every function has been rewritten from the ground up.
+ This library reverts to SoftwareSerial
+ Default RX and TX are 2 nd 3

## Features
+ Send SMS
+ Read SMS
+ Delete 

## Methods and functions

Name|Return|Notes
:-------|:-------:|:-----------------------------------------------:|
Init(baud_rate)|None|Initialize the library, connect to the GSM Module and configure it
SendSms(number,text)|true or false|Sends sms. Takes flashstrings as input ( F("text") )
ForwardSms(number)|true or false|Forwards the last read sms to the specified number
GetNewSMSIndex()|int|Returns the index of the first sms found in memory
ReadNewSMS()|bool|Reads the first sms found in memory, into LWSim800.sms struct.
ReadSms(index)|bool|Reads the sms, specified by the index, into LWSim800.sms struct.
DelSMSByIndex(index)|true or false|Deletes the sms specified by the index
DelAllSMS()|true or false|Deletes all message in the SIM800 module.

## Data struct
There's only one data struct used in this library and it's LWSim800.sms.

It contains a char array for sms content (message) and a char array for the sms sender (sender).
When ReadSms or ReadNewSMS are called, they store the read sms inside this struct, which is accessible from the external. Check the examples for a better idea of how it works. It's pretty easy.

__________________________________________________________________

## Credits!
This Library, as already said, was inspired from the Bare Bone SIM800 Library from Ayo Ayibiowu (https://github.com/thehapyone/BareBoneSim800). If you need more features, like Time, Location and GPRS stuff, give it a shot!

Also, if you have some ideas to make it even more stable or lighter, just contribute to this repo!

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)

