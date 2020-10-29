/*  
* This is a light weight lib for comunicating with SIM800L gsm module.
* It's extremely light and simple but full of security checks to 
* guarantee the stability in every situation. 
*
* It's based on another lib, the BareBoneSim800L, made by Ayo Ayibiowu. 
* Props to him!
*
* The main differences are:
* - this library reverts to SoftwareSerial
* - default RX and TX are 2 nd 3
* - completely avoids the use of Strings, replaced by a single, preallocated, char array.
* - removes every function apart SMS-related ones.
* - almost every function has been rewritten from the ground up.
*
* Created on: Oct 29, 2010
* Author: Alberto Marziali
* Email: alberto.marziali.am@gmail.com
* Version: v1.00
*
*/

#include <LWSim800.h>

LWSim800 sim800; 

void setup() 
{
  Serial.begin(115200);

  //enable connection with the sim800l
  sim800.Init(4800); 
}


void loop() 
{
  //if true, it also reads the new sms. If false, just skip
  if (sim800.ReadNewSMS())
  {
    //lets read the sms
    Serial.print(F("New sms received: "));
    Serial.println(sim800.sms.message);
    Serial.print(F("The sender is: "));
    Serial.println(sim800.sms.sender);

    //forward it
    sim800.ForwardSMS(F("+391234567890"));
  }
}
