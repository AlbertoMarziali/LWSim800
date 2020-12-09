/*	
*	This is a light weight lib for comunicating with SIM800L gsm module.
*	It's extremely light and simple but full of security checks to 
*	guarantee the stability in every situation. 
*
*	It's based on another lib, the BareBoneSim800L, made by Ayo Ayibiowu. 
*	Props to him!
*
*	The main differences are:
*	- this library reverts to SoftwareSerial
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

#ifndef LWSim800_h
#define LWSim800_h

#include <time.h>

 
//RX and TX pins
#define RX_PIN 2	
#define TX_PIN 3

//Common "toFind" magics
#define CHECK_FLUSH "FLUSH"
#define CHECK_OK "OK"
#define CHECK_READY_TO_RECEIVE ">"
#define CHECK_CMGL "+CMGL:"
#define CHECK_CMGR "+CMGR:"
#define CHECK_CPMS "+CPMS:"
#define CHECK_CCLK "+CCLK:"
#define CHECK_CALL_READY "Call Ready"

//This enables debugging mode, to disable it - set value to 0
#define DEBUG 0

//LW Sim800 Class 
class LWSim800
{
	private:
	
	bool available = false;
	   
	// private functions
	long _checkResponse(uint16_t comm_timeout, uint16_t interchar_timeout, const char* toFind);
	bool _findLabel(const char *magic, uint16_t interchar_timeout);
	bool _fetchField(char *dest, int dest_size, char fieldBegin, char fieldEnd, uint16_t interchar_timeout);
	long _stringToUTC(char *text);

	bool _sendSMS(const __FlashStringHelper *destp, char *destc, const __FlashStringHelper *textp, char* textc);
	void _serialPrint(const __FlashStringHelper *text);
	
	public:
	 
	//constructor
	LWSim800();
	 
	//data used by LWSim800
	#define MAX_INIT_RETRIES 10 //max retries for init
	#define MESSAGE_MAX_LENGTH 161 //160 + 1 terminator
	#define SENDER_MAX_LENGTH 15 //14 + 1 terminator
	struct {
  		char message [MESSAGE_MAX_LENGTH];
  		char sender[SENDER_MAX_LENGTH];
  		long dateTime;
	} sms;

	// public functions
	void Init(long baud_rate);
	bool Reset();

	long GetDateTime();

	int GetNewSMSIndex(); //gets the index of a new sms
	bool ReadNewSMS(); //reads a new sms from memory
	bool ReadSMSByIndex(uint8_t index); // reads an sms at a particular index
	
	bool SendSMS(char *dest, const __FlashStringHelper *text); // This sends an sms. char* dest and F() text 
	bool SendSMS(char *dest, char *text); // This sends an sms. char* dest and char* text 
	bool SendSMS(const __FlashStringHelper *dest, char *text); // This sends an sms. F() dest and char* text
	bool SendSMS(const __FlashStringHelper *dest, const __FlashStringHelper *text); // This sends an sms. F() dest and F() text
	bool ForwardSMS(char *dest); //forwards last read sms, char* dest
	bool ForwardSMS(const __FlashStringHelper *dest); //forwards last read sms, F() dest

	bool DelSMSByIndex(uint8_t index); //delete sms by index
	bool DelAllSMS(); // deletes all sms 
	 
 };
 
 #endif
	 

	 
	 
	 
	 
	 