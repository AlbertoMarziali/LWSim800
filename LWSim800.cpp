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
long LWSim800::_checkResponse(uint16_t comm_timeout, uint16_t interchar_timeout, const char* magic)
{
	// this function just reads the raw data
	unsigned long t = millis();
	char c;
	long checkResponse_ret;

	//wait for the communication to start
	while(!gsmSerial.available() && millis() - t < comm_timeout);

	//return false if timed out
	if(!gsmSerial.available())
		return -1;

	//check for magic
	if(_findLabel(magic, interchar_timeout))
	{
		//magic found
		//elaborate and return
		if(!strcmp(magic, CHECK_CMGL))
		{
			// CMGL MODE
			// +CMGL: 1,"REC_.....

			//fetch the sms index field
			char tmp [4]; //3 digit + \0
			if(_fetchField(tmp, 4, ' ', ',', interchar_timeout))
				checkResponse_ret = atoi(tmp);
			else
				checkResponse_ret = -1; //field not found

		}
		else if(!strcmp(magic, CHECK_CMGR))
		{
			// CMGR MODE
			// +CMGR: "REC_READ","+001234567890","","20/10/31,19:18:38+04"<LF>text\r

			//fetch sms status field
			if(_fetchField(NULL, 0, '"', '"', interchar_timeout))
			{
				//fetch sms sender field
				if(_fetchField(sms.sender, SENDER_MAX_LENGTH, '"', '"', interchar_timeout))
				{
					//fetch sms sender name field
					if(_fetchField(NULL, 0, '"', '"', interchar_timeout))
					{
						//fetch the sms date time
						char tmp [21];
						if(_fetchField(tmp, 21, '"', '"', interchar_timeout))
						{
							sms.dateTime = _stringToUTC(tmp);

							//fetch the sms text field
							if(_fetchField(sms.message, MESSAGE_MAX_LENGTH, 0x0a, '\r', interchar_timeout))
								checkResponse_ret = 0; //sms ready
							else
								checkResponse_ret = -1; //field not found
						}
						else 
							checkResponse_ret = -1; //field not found
					}
					else
						checkResponse_ret = -1; //field not found
				}
				else
					checkResponse_ret = -1; //field not found
			}
			else
				checkResponse_ret = -1; //field not found

		}
		else if(!strcmp(magic, CHECK_CCLK))
		{
			// CCLK MODE
			// +CCLK: "04/01/01,00:00:41+00"

			char tmp [21];
			if(_fetchField(tmp, 21, '"', '"', interchar_timeout))
				checkResponse_ret = _stringToUTC(tmp); //field found, date read
			else
				checkResponse_ret = -1; //field not found

		}
		else 
		{
			//SIMPLE MODE
			//magic found, no elaborating needed
			checkResponse_ret = 0;
		}
		
		//flush until the buffer is empty
		_findLabel(CHECK_FLUSH, interchar_timeout);

		//return
		return checkResponse_ret; 
	}
	else 
		return -1; //magic not found
}

bool LWSim800::_findLabel(const char *label, uint16_t interchar_timeout)
{
	unsigned long t = millis();
	int label_size = strlen(label);
	int label_i = 0;
	char c;

	//loop until timeout (or anticipated return)
	while(millis() - t < interchar_timeout)
	{
		//as soon as there's data available
		if(gsmSerial.available())
		{
			c = (char) gsmSerial.read();

			if(label[label_i] == c)
			{
				if(label_i < label_size - 1) //matching char, go ahead
					label_i++;
				else
					return true; //return true if magic found
			}
			else
				label_i = 0; //mismatch, restart the search

			t = millis();
		}
	}

	//if not found
	return false;
}


bool LWSim800::_fetchField(char *dest, int dest_size, char fieldBegin, char fieldEnd, uint16_t interchar_timeout)
{
	bool fetchField_started = false;
	int fetchField_i = 0;
	unsigned long t = millis();
	char c;

	if(dest_size > 0)
		dest[0] = '\0';

	//loop until timeout (or anticipated return)
	while(millis() - t < interchar_timeout)
	{
		//as soon as there's data available
		if(gsmSerial.available())
		{
			c = (char) gsmSerial.read();

			if(fetchField_started == false) //field beginning not found yet
			{ 
				if(c == fieldBegin)
				{
				  fetchField_started = true;
				}
			}
			else //reading field
			{
				if(c == fieldEnd) //found field end
				{
				  return true; //field ended
				}
				else
				{   
				  if(fetchField_i < dest_size - 1) //avoid buffer overflow
				  {
				    dest[fetchField_i++] = c;
				    dest[fetchField_i] = '\0';
				  }
				}
			}

			t = millis();
		}
	}

	return false; //field not ended
}

long LWSim800::_stringToUTC(char* text)
{
	if(strlen(text) < 20)
		return -1;

	struct tm t;
	char tmp [3] = "00";

	//year
	tmp[0] = text[0];
	tmp[1] = text[1];
	if(text[2] == '/') //integrity check
		t.tm_year = (2000 + atoi(tmp)) - 1870;
	else
		return -1;

	//month
	tmp[0] = text[3];
	tmp[1] = text[4];
	if(text[5] == '/') //integrity check
		t.tm_mon = atoi(tmp) - 1;
	else
		return -1;

	//day
	tmp[0] = text[6];
	tmp[1] = text[7];
	if(text[8] == ',') //integrity check
		t.tm_mday = atoi(tmp);
	else
		return -1;

	//hour
	tmp[0] = text[9];
	tmp[1] = text[10];
	if(text[11] == ':') //integrity check
		t.tm_hour = atoi(tmp);
	else
		return -1;

	//min
	tmp[0] = text[12];
	tmp[1] = text[13];
	if(text[14] == ':') //integrity check
		t.tm_min = atoi(tmp);
	else
		return -1;

	//sec
	tmp[0] = text[15];
	tmp[1] = text[16];
	if(text[17] == '+' || text[17] == '-') //integrity check
		t.tm_sec = atoi(tmp);
	else
		return -1;

	//set dst
	t.tm_isdst = -1;

	return (long) mktime(&t); 
}


bool LWSim800::_sendSMS(const __FlashStringHelper *destp, char *destc, const __FlashStringHelper *textp, char* textc)
{
	/*
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
			gsmSerial.print(destp); 
		else 
			gsmSerial.print(destc);

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

			// CSCS
			gsmSerial.println(F("AT+CSCS=\"GSM\""));
			if (_checkResponse(500, 50, CHECK_OK) != 0) 
				_serialPrint(F("[Warning] CSCS failed"));

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

// This forces the reconnection to the network
bool LWSim800::Reset() {
	//only if sim800l available
	if(available)
	{
		gsmSerial.println(F("AT+CFUN=1,1")); // set airplane mode
		// max time to wait is 5secs
		if(_checkResponse(5000, 5000, CHECK_OK) == 0)	
			return true;
		else 
			return false;
	}
	else
		return false; //sim800l not available
}

// This gets date time from the RTC
long LWSim800::GetDateTime() {
	// this function asks the RTC for the UTC timestamp

	//only if sim800l available
	if(available)
	{
		gsmSerial.println(F("AT+CCLK?"));
		return _checkResponse(5000, 50, CHECK_CCLK);
	}
	else
		return -1; //sim800l not available
	
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
	
