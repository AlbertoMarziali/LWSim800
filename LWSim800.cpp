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
*	Created on: Nov 03, 2020
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

// this reads data from sim and reports if found (0 or 1)
// Also:
// - if toFind is "CMGL", if found, returns the first sms index
// - if toFind is "CMGR:", if found, read the sms into sms struct
// - if toFind is something else, if found, just report it

// Returns:
// -1 not found
// 0 found and executed operation
// 1..N found and operation res (in CMGL, N = new sms index)
int LWSim800::_checkResponse(uint16_t comm_timeout, uint16_t interchar_timeout, const char* toFind)
{
	// this function just reads the raw data
	unsigned long t = millis();
	short num_of_bytes_read = 0;
	char c;

	int toFind_size = strlen(toFind);
	int toFind_state = 0; 
	// STATES:
	//0 - initial search, 
	//1 - elaboration, 
	//2 - res ready, just flush the serial
	int toFind_return = -1;
	int toFind_index = 0;
	int toFind_step = 0;


	//set find mode
	int toFind_mode = 0; //default
	if(!strcmp(toFind, CHECK_CMGL))
		toFind_mode = 1;
	else if(!strcmp(toFind, CHECK_CMGR))
		toFind_mode = 2;

	//wait for the communication to start
	while(!gsmSerial.available() && millis() - t < comm_timeout);

	//return false if timed out
	if(!gsmSerial.available())
		return -1;

	//read data on gsmSerial
	t = millis();
	while(millis() - t < interchar_timeout)
	{
		//as soon as there's data available
		if(gsmSerial.available())
		{
			//read the char from serial
			c = (char) gsmSerial.read();

			if(DEBUG)
				Serial.print(c);

			//search the tofind
			if(toFind_state == 0) // - initial search, not found yet
			{
				if(toFind[toFind_index] == c)
				{
					if(toFind_index < toFind_size - 1)
						toFind_index++;
					else
						toFind_state = 1;
				}
				else
					toFind_index = 0;
			}
			//tofind is found, do more elaboration if needed
			else if(toFind_state == 1) // - elaboration
			{
				//specific mode 1 and 2 features:

				//just needed to find the string
				if(toFind_mode == 0)
				{
					toFind_state = 2; //done elaborating
					toFind_return = 0; //res ready
				}
				//read new sms index
				else if(toFind_mode == 1)
				{
					if(c != ',' && num_of_bytes_read < SMS_INDEX_MAX_LENGTH - 1) //max 3 digit + 1 terminator
					{
						smsIndex[num_of_bytes_read] = c;
						num_of_bytes_read++;
						smsIndex[num_of_bytes_read] = 0x00;
					}
					else
					{
						toFind_state = 2; //done elaborating
						toFind_return = atoi(smsIndex); //res ready
					}
				}
				//read sms content into sms struct
				else if(toFind_mode == 2)
				{
					//we start with the number
					//find the beginning of the number
					if(toFind_step == 0) 
					{
						if(c == ',') //if found, go to next step
							toFind_step = 1; 
					}
					//skip one char
					else if(toFind_step == 1)
					{
						toFind_step = 2;
					}
					//read number until max or "
					else if(toFind_step == 2)
					{
						if(c != '"' && num_of_bytes_read < SENDER_MAX_LENGTH - 1)
						{
							sms.sender[num_of_bytes_read] = c;
							num_of_bytes_read++;
							sms.sender[num_of_bytes_read] = 0x00;
						}
						else
						{
							num_of_bytes_read = 0; //reset
							toFind_step = 3;
						}
					}
					//find 0x0a (<LF>)
					else if(toFind_step == 3)
					{
						if(c == 0x0a)
							toFind_step = 4;
					}
					//read sms content until \n
					else if(toFind_step == 4)
					{
						if(c != '\r' && num_of_bytes_read < MESSAGE_MAX_LENGTH - 1)
						{
							sms.message[num_of_bytes_read] = c;
							num_of_bytes_read++;
							sms.message[num_of_bytes_read] = 0x00;
						}
						else
						{
							toFind_state = 2; //done elaborating
							toFind_return = 0; //sms read successfully
						}
					}
				}
			}

			//update interchar timing
			t = millis();
		}
	}

	//return
	return toFind_return;
}

bool LWSim800::_sendSMS(const __FlashStringHelper *destp, char *destc, const __FlashStringHelper *textp, char* textc)
{
	/* First send out AT+CMGF=1 - activate text mode
	* The AT+CMGS=\number\
	AT+CMGS=<number><CR><message><CTRL-Z>	+CMGS:<mr>
	OK
	*/

	//only if sim800l available
	if(available)
	{
		gsmSerial.print(F("AT+CMGS=\"")); // command to send sms
		//select progmemchar or char dest
		if(destp != NULL) 
			gsmSerial.println(destp); 
		else 
			gsmSerial.println(destc);

		gsmSerial.println(F("\""));
		if (_checkResponse(15000, 50, CHECK_READY_TO_RECEIVE) == 0) 
		{	
			//select progmemchar or char text
			if(textp != NULL) 
				gsmSerial.println(textp); 
			else 
				gsmSerial.println(textc);

			_checkResponse(2000, 50, CHECK_FLUSH); //flush

			gsmSerial.print((char)26);
			// if successfully sent we should get CMGS:xxx ending with OK
			if(_checkResponse(20000, 50, CHECK_OK) == 0)
				return true;
			else
				return false;
		}
		else 
			return false; //no answer
	}
	else
		return false; //sim800l not available
}

void LWSim800::_serialPrint(const __FlashStringHelper *text)
{
	Serial.print(F("[LWSIM800] "));
	Serial.println(text);
}
 
// 
// PUBLIC METHODS
//

void LWSim800::Init(long baud_rate) {
	int retries = 0;
	// begin software serial
	gsmSerial.begin(baud_rate);	 	 
	// this will flush the serial
	_checkResponse(1000, 1000, CHECK_FLUSH); 

	// check if the sim800L is actually attached
	// sends a test AT command, if attached we should get an OK response
	// if attached we should get an OK response
	while(retries++ < MAX_INIT_RETRIES && !available)
	{
		_serialPrint(F("Waiting for the GSM Module"));
		
		//check connection. if ok, proceed in setup
		gsmSerial.println(F("AT"));
		if (_checkResponse(2000, 50, CHECK_OK) == 0) 
		{
			//Flush again
			_checkResponse(5000, 5000, CHECK_FLUSH); 

			// Reset to the factory settings
			gsmSerial.println(F("AT&F"));
			if (_checkResponse(1000, 50, CHECK_OK) != 0) 
				_serialPrint(F("[Warning] Factory reset failed"));

			// Switch off echo
			gsmSerial.println(F("ATE0"));
			if (_checkResponse(500, 50, CHECK_OK) != 0) 
				_serialPrint(F("[Warning] Turn echo off failed"));

			// Mobile Equipment Error Code
			gsmSerial.println(F("AT+CMEE=0"));
			if (_checkResponse(500, 50, CHECK_OK) != 0) 
				_serialPrint(F("[Warning] Error code configuration failed"));

			// Set the SMS mode to text 
			gsmSerial.println(F("AT+CMGF=1"));
			if (_checkResponse(500, 50, CHECK_OK) != 0) 
				_serialPrint(F("[Warning] SMS text mode setup failed"));
			 
			// Disable messages about new SMS from GSM module
			gsmSerial.println(F("AT+CNMI=2,0"));
			if (_checkResponse(1000, 50, CHECK_OK) != 0) 
				_serialPrint(F("[Warning] SMS notification disable failed"));
			
			// Init sms memory
			gsmSerial.println(F("AT+CPMS=\"SM\",\"SM\",\"SM\""));
			if (_checkResponse(1000, 50, CHECK_CPMS) != 0) 
				_serialPrint(F("[Warning] SMS Memory init failed"));
	
			//enable all functions
			available = true; 	

			//delete all sms in memory
			if(!DelAllSMS())
				_serialPrint(F("[Warning] SMS Memory Cleanup Failed"));
		}
			
	}

	//notify the result
	if(available)
		_serialPrint(F("GSM Module Ready"));
	else
		_serialPrint(F("GSM Module not available"));
	
}

// This gets the index of the first sms in memory
int LWSim800::GetNewSMSIndex() {
	// this function checks the message for the first message index.
	// returns 0 if nothing is found, possible for an empty simcard

	//only if sim800l available
	if(available)
	{
		gsmSerial.println(F("AT+CMGL=\"ALL\",0"));
		return _checkResponse(10000, 50, CHECK_CMGL);
	}
	else
		return -1; //sim800l not available
	
}
 
// This reads the first sms found in memory
bool LWSim800::ReadNewSMS() {
	//only if sim800l available
	if(available)
	{
		//read first sms index
		int newSMSIndex = GetNewSMSIndex();
		
		if(newSMSIndex > 0) //new sms found, try to read it
		{
			if(ReadSMSByIndex(newSMSIndex)) // if was able to read it
				return DelSMSByIndex(newSMSIndex); // delete it
		}
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
		//set sms text mode
		gsmSerial.print(F("AT+CMGR="));
		gsmSerial.println(index);			
		//reads message text and number of sender, return if error
		if(_checkResponse(10000, 50, CHECK_CMGR) == 0)
			return true;
		else
			return false; //no answer
	}
	else
		return false; //sim800l not available
}


// This sends an sms. char* dest and F() text 
bool LWSim800::SendSMS(char *dest, const __FlashStringHelper *text) {
	return _sendSMS(NULL, dest, text, NULL);
}

// This sends an sms. char* dest and char* text 
bool LWSim800::SendSMS(char *dest, char *text) {
	return _sendSMS(NULL, dest, NULL, text);
}

// This sends an sms. F() dest and char* text
bool LWSim800::SendSMS(const __FlashStringHelper *dest, char *text) {
	return _sendSMS(dest, NULL, NULL, text);
}

// This sends an sms. F() dest and F() text
bool LWSim800::SendSMS(const __FlashStringHelper *dest, const __FlashStringHelper *text) {
	return _sendSMS(dest, NULL, text, NULL);
}

// This forwards the last sms read to the defined char* destination
bool LWSim800::ForwardSMS(char *dest) {
	return _sendSMS(NULL, dest, NULL, sms.message);
}

// This forwards the last sms read to the defined F() destination
bool LWSim800::ForwardSMS(const __FlashStringHelper *dest) {
	return _sendSMS(dest, NULL, NULL, sms.message);
}

// This deletes the sms specified by the index
bool LWSim800::DelSMSByIndex(uint8_t index) {
	//only if sim800l available
	if(available)
	{
		gsmSerial.print(F("AT+CMGD="));
		gsmSerial.println(index);
		// max time to wait is 25secs
		if(_checkResponse(25000, 50, CHECK_OK) == 0)
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
		gsmSerial.println(F("AT+CMGDA=\"DEL ALL\"")); // set sms to text mode
		// max time to wait is 25secs
		if(_checkResponse(25000, 50, CHECK_OK) == 0)
			return true;
		else
			return false;
	}
	else
		return false; //sim800l not available
}
	
