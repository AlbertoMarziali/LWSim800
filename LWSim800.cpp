/*	
*	This is a light weight lib for comunicating with SIM800L gsm module.
*	It's extremely light and simple but full of security checks to 
*	guarantee the stability in every situation. 
*
*	It's based on another lib, the BareBoneSim800L, made by Ayo Ayibiowu. 
*	Props to him!
*
*	The main differences are:
*	- this library reverts to SoftwareSerial a
*	- default RX and TX are 2 nd 3
*	- completely avoids the use of Strings, replaced by a single, preallocated, char array.
*	- removes every function apart SMS-related ones.
*	- almost every function has been rewritten from the ground up.
*
*	Created on: Oct 29, 2010
*	Author: Alberto Marziali
*	Email: alberto.marziali.am@gmail.com
*	Version: v1.0
*
*/
 
#include "Arduino.h"
#include <SoftwareSerial.h>
#include "LWSim800.h"

//sim800 serial connection
SoftwareSerial gsmSerial(RX_PIN, TX_PIN);

// Initialize the constructors
LWSim800::LWSim800() {}
 

// 
// PRIVATE METHODS
//

// This reads and returns the response status
byte LWSim800::_readResponseStatus(uint16_t comm_timeout, uint16_t interchar_timeout)
{
	// This function handles the response from the radio and returns a status response
	uint8_t Status = 99; // the default stat and it means a timeout
	 
	//try to read data
	if(_readResponseRaw(comm_timeout, interchar_timeout))
	{
		//read successful
		/*
		* Checks for the status response
		* Response are - OK, ERROR, READY, >, CONNECT OK
		* SEND OK, DATA ACCEPT, SEND FAIL, CLOSE, CLOSED
		* note_ LOCAL iP COMMANDS HAS NO ENDING RESPONSE 
		* ERROR - 0
		* READY - 1
		* CONNECT OK - 2
		* SEND OK - 3
		* SEND FAIL - 4
		* CLOSED - 5
		* > - 6
		* OK - 7
		* 
		* 
		*/
		if(DEBUG)
			Serial.println(sms.message);

		for (byte i=0; i<_responseInfoSize; i++)
		{
			if((strstr(sms.message, _responseInfo[i])) != NULL)
			{
				Status = i;
				return Status;
			}
		}
	}
	else //read timeout
		return Status;
}
 
// This reads the raw response data and places it in sms.message
bool LWSim800::_readResponseRaw(uint16_t comm_timeout, uint16_t interchar_timeout){
	// this function just reads the raw data
	unsigned long t = millis();
	short num_of_bytes_read = 0;
	char c;

	//wait for the communication to start
	while(!gsmSerial.available() && millis() - t < comm_timeout);

	//return false if timed out
	if(!gsmSerial.available())
		return false;

	//read data on gsmSerial
	t = millis();
	while(millis() - t < interchar_timeout)
	{
		if(gsmSerial.available())
		{
			//read data, avoiding overflow
			if(num_of_bytes_read < MESSAGE_MAX_LENGTH)
			{
				sms.message[num_of_bytes_read] = (char) gsmSerial.read();
				if(DEBUG)
					Serial.print(sms.message[num_of_bytes_read]);
				num_of_bytes_read++;	
				sms.message[num_of_bytes_read] = 0x00; //string terminator
			}
			else
			{
				//we avoid overflowing but still need to empty the ingress buffer
				c = gsmSerial.read();
			}

			//update interchar timing
			t = millis();
		}
	}

	if(DEBUG)
			Serial.println();


	//did it actually read?
	if(num_of_bytes_read)
		return true;
	else
		return false;
}

 
// 
// PUBLIC METHODS
//

void LWSim800::Init(long baud_rate) {
	int retries = 0;
	// begin software serial
	gsmSerial.begin(baud_rate);	 	 
	// this will flush the serial
	byte result = _readResponseStatus(1000, 50); 

	// check if the sim800L is actually attached
	// sends a test AT command, if attached we should get an OK response
	// if attached we should get an OK response
	while(retries++ < MAX_INIT_RETRIES && !available)
	{
		Serial.println(F(" => Waiting for the GSM Module"));
		gsmSerial.print(F("AT\r\n"));
		result = _readResponseStatus(10000, 50); // timeout of 1 secs
		//if ok, proceed in setup
		if (result == OK) 
		{
			//set up for sms mode
			gsmSerial.print(F("AT+CSCS=\"GSM\"\r\n"));
			result = _readResponseStatus(10000, 50); // just to clear the buffer
			if(result == OK)
			{
				gsmSerial.print(F("AT+CMGF=1\r"));
				result = _readResponseStatus(10000, 50);
				if(result == OK)
				{
					//enable all functions
					available = true; 	

					//delete  all sms in memory
					if(!DelAllSMS())
						available = false;	
				}
			}
		}
	}

	//notify the result
	if(available)
		Serial.println(F(" => GSM Module Ready"));
	else
		Serial.println(F(" => GSM Module not available"));
	
}

// This gets the index of the first sms in memory
int LWSim800::GetNewSMSIndex() {
	// this function checks the message for the first message index.
	// returns 0 if nothing is found, possible for an empty simcard

	//only if sim800l available
	if(available)
	{
		char* sms_p;
		char* sms_p2;

		gsmSerial.print(F("AT+CMGL=\"ALL\",0"));
		gsmSerial.print("\r");
		if(_readResponseRaw(10000, 50)) //reads the result	
		{
			Serial.println(sms.message);
			sms_p = strstr(sms.message, "+CMGL:");
			if(sms_p != NULL)
			{
				// means message is found
				sms_p += 6;
				sms_p2 = strchr(sms_p, ',');
				if(sms_p2 == NULL)
					return -1; //integrity check failure
				*sms_p2 = 0x00;

				//convert to integer
				return atoi(sms_p);
			}
			else
				return 0;	//no new message
		}
		else
			return -1; //no response from sim
	}
	else
		return -1; //sim800l not available
	
}
 
// This sends an sms. Only works with F()
bool LWSim800::SendSMS(const __FlashStringHelper *dest, const __FlashStringHelper *text) {
	/* First send out AT+CMGF=1 - activate text mode
	* The AT+CMGS=\number\
	AT+CMGS=<number><CR><message><CTRL-Z>	+CMGS:<mr>
	OK
	*/

	//only if sim800l available
	if(available)
	{
		byte result;
		gsmSerial.print(F("AT+CMGF=1\r\n")); // set sms to text mode
		result = _readResponseStatus(2000, 50);
		if(result == ERROR)
			return false; // this just end the function here 
		delay(100);
		gsmSerial.print(F("AT+CMGS=\"")); // command to send sms
		gsmSerial.print(dest);
		gsmSerial.print(F("\"\r\n"));
		result=_readResponseStatus(5000, 50); // to clear buffer and see if successful
		if(result == READY_TO_RECEIVE){
			gsmSerial.print(text);
			gsmSerial.print(F("\r"));
			result=_readResponseStatus(2000, 50);
			gsmSerial.print((char)26);
			result = _readResponseStatus(2000, 50);
			// if successfully sent we should get CMGS:xxx ending with OK
			if(result == OK)
				return true;
			else
				return false;
		}
	}
	else
		return false; //sim800l not available
}

// This forwards the last sms read to the defined destination
bool LWSim800::ForwardSMS(const __FlashStringHelper *dest) {
	/* First send out AT+CMGF=1 - activate text mode
	* The AT+CMGS=\number\
	AT+CMGS=<number><CR><message><CTRL-Z>	+CMGS:<mr>
	OK
	*/

	//only if sim800l available
	if(available)
	{
		byte result;
		gsmSerial.print(F("AT+CMGF=1\r\n")); // set sms to text mode
		result = _readResponseStatus(10000, 50);
		if(result == ERROR)
			return false; // this just end the function here 
		delay(1000);
		gsmSerial.print(F("AT+CMGS=\"")); // command to send sms
		gsmSerial.print(dest);
		gsmSerial.print(F("\"\r\n"));
		result=_readResponseStatus(15000, 50); // to clear buffer and see if successful
		if(result == READY_TO_RECEIVE){
			gsmSerial.print(sms.message);
			gsmSerial.print("\r");
			result=_readResponseStatus(1000, 50);
			gsmSerial.print((char)26);
			result = _readResponseStatus(20000, 50);
			// if successfully sent we should get CMGS:xxx ending with OK
			if(result == OK)
				return true;
			else
				return false;
		}
	}
	else
		return false; //sim800l not available
}

// This reads the first sms found in memory
bool LWSim800::ReadNewSMS() {
	//only if sim800l available
	if(available)
	{
		//read first sms index
		int newSMSIndex = GetNewSMSIndex();
		Serial.print("new sms: ");
		Serial.println(newSMSIndex);
		if(newSMSIndex > 0) //new sms found, try to read it
			if(ReadSMSByIndex(newSMSIndex)) // if was able to read it
				DelSMSByIndex(newSMSIndex); // delete it
			else
				return false; //read failed

		else	//new sms not found
			return false;
	}
	else
		return false; //sim800l not available
}

// This reads the sms specified by the index
bool LWSim800::ReadSMSByIndex(uint8_t index) {
	//only if sim800l available
	if(available)
	{
		gsmSerial.print(F("AT+CMGF=1\r"));
		byte result = _readResponseStatus(10000, 50);
		if(result == OK)
		{
			gsmSerial.print(F("AT+CMGR="));
			gsmSerial.print(index);
			gsmSerial.print("\r");
			
			//reads message text and number of sender, return if timed out
			if(!_readResponseRaw(10000, 50)) 
				return false;

			//if it read an sms
			if(strstr(sms.message, "CMGR:") != NULL)
			{
				//sender
				char* sms_p;
				//find the beginning of th enumber
				sms_p = strchr(sms.message, ',');
				if(sms_p == NULL || strlen(sms_p) < 14) //integrity check
					return false;
				strncpy(sms.sender, strchr(sms.message, ',') + 2, 14);
				//find the end of the number
				sms_p = strchr(sms.sender, '"');
				if(sms_p == NULL) //integrity check
					return false;
				*sms_p = 0x00;

				//message
				sms_p = strchr(sms.message, 0x0a) + 4; //0x0a = <LF>
				if(sms_p == NULL) //integrity check
					return false;
				//find the sms text size
				short message_size = strchr(sms.message, 0x00) - sms_p;
				//move the sms text to the top
				for(int i = 0; i < message_size; i++)
					sms.message[i] = *(sms_p + i);
				//terminator
				sms.message[message_size] = 0x00;

				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false; //sim800l not available
}

// This deletes the sms specified by the index
bool LWSim800::DelSMSByIndex(uint8_t index) {
	//only if sim800l available
	if(available)
	{
		byte result;
		gsmSerial.print(F("AT+CMGD="));
		gsmSerial.print(index);
		gsmSerial.print("\r");
		result = _readResponseStatus(25000, 50); // max time to wait is 25secs
		if(result == OK)
			return true;
		else
			return false;
	}
	else
		return false; //sim800l not available
}


// This deletes all sms in memory  
bool LWSim800::DelAllSMS() {
	//only if sim800l available
	if(available)
	{
		byte result;
		gsmSerial.print(F("AT+CMGDA=\"DEL ALL\"\r\n")); // set sms to text mode
		result = _readResponseStatus(25000, 50); // max time to wait is 25secs
		if(result == OK)
			return true;
		else
			return false;
	}
	else
		return false; //sim800l not available
}
	
