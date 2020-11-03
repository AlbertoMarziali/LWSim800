## A Light Weight SIM800l Library Module for Arduino
This is a light weight lib for comunicating with SIM800L gsm module.
It's extremely light and simple but full of security checks to 
guarantee the stability in every situation. 

The main features of this library are:
+ It's the only gsm lib to avoid the use of a buffer. (aproximately 200 Byte less!)
+ It completely avoids the use of Strings, replaced by a single, preallocated, struct (sms)
+ It offers SMS-related functions only, making it extremely lightweight.
+ Default RX and TX are 2 nd 3

## Features
+ Send SMS
+ Read SMS
+ Delete SMS

## Methods and functions

Name|Return|Info
:-------|:-------:|:-----------------------------------------------:|
Init(baud_rate)|None|Initialize the library, connect to the GSM Module and configure it
SendSMS_C(number,text)|true or false|Sends sms. Takes a char pointer as input ( char * )
SendSMS_P(number,text)|true or false|Sends sms. Takes a flashstring as input ( F("text") )
ForwardSMS(number)|true or false|Forwards the last read sms to the specified number
GetNewSMSIndex()|int|Returns the index of the first sms found in memory
ReadNewSMS()|bool|Reads the first sms found in memory, into LWSim800.sms struct.
ReadSms(index)|bool|Reads the sms, specified by the index, into LWSim800.sms struct.
DelSMSByIndex(index)|true or false|Deletes the sms specified by the index
DelAllSMS()|true or false|Deletes all message in the SIM800 module.

## Data struct
There's only one data struct used in this library and it's LWSim800.sms:

```
#define MESSAGE_MAX_LENGTH 161
#define SENDER_MAX_LENGTH 15

struct {
	char message [MESSAGE_MAX_LENGTH];
	char sender[SENDER_MAX_LENGTH];
} sms;
```

When ReadSMS or ReadNewSMS are called, they store the read sms data inside this struct, which is accessible from the external. 

```
Serial.println(sim800.sms.message);
Serial.println(sim800.sms.sender);
```

Check the examples to get a better idea of how it works. It's pretty easy.

__________________________________________________________________

## Credits!
This Library was inspired from the Bare Bone SIM800 Library from Ayo Ayibiowu (https://github.com/thehapyone/BareBoneSim800). If you need more features, like Time, Location and GPRS stuff, give it a shot!

Also, if you have some ideas to make it even more stable or lighter, just contribute to this repo!

## License
This project is released under
The GNU Lesser General Public License (LGPL-3.0)

