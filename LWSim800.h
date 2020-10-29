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
*	Created on: Oct 29, 2010
*	Author: Alberto Marziali
*	Email: alberto.marziali.am@gmail.com
*	Version: v1.0
*
*/

#ifndef LWSim800_h
#define LWSim800_h
 
//RX and TX pins
#define RX_PIN 2	
#define TX_PIN 3
 
//States
#define TIMEOUT 99
#define ERROR 0
#define NOT_READY 1
#define READY 2
#define CONNECT_OK 3
#define CONNECT_FAIL 4
#define ALREADY_CONNECT 5
#define SEND_OK 6
#define SEND_FAIL 7
#define DATA_ACCEPT 8
#define CLOSED 9
#define READY_TO_RECEIVE 10 // basically SMSGOOD means >
#define OK 11

//This enables debugging mode, to disable it - set value to 0
#define DEBUG 0 

//LW Sim800 Class 
class LWSim800
{
	private:
	
	bool available = false;
	static const uint8_t _responseInfoSize = 12; 
	const char* _responseInfo[_responseInfoSize] =
			  {"ERROR",
			  "NOT READY",
			  "READY",
			  "CONNECT OK",
			  "CONNECT FAIL",
			  "ALREADY CONNECT",
			  "SEND OK",
			  "SEND FAIL",
			  "DATA ACCEPT",
			  "CLOSED",
			  ">",
			  "OK"};
	   
	// private functions
	byte _readResponseStatus(uint16_t timeout);
	bool _readResponseRaw(uint16_t timeout);

	
	public:
	 
	//constructor
	LWSim800();
	 
	//data used by LWSim800
	#define MAX_INIT_RETRIES 3 //max retries for init
	#define MESSAGE_MAX_LENGTH 260
	#define SENDER_MAX_LENGTH 15
	struct {
  		char message [MESSAGE_MAX_LENGTH];
  		char sender[SENDER_MAX_LENGTH];
	} sms;

	// public functions
	void Init(long baud_rate);
	int GetNewSMSIndex(); //gets the index of a new sms
	bool SendSMS(const __FlashStringHelper *dest, const __FlashStringHelper *text); //send an sms
	bool ForwardSMS(const __FlashStringHelper *dest); //forwards last read sms
	bool ReadNewSMS(); //reads a new sms from memory
	bool ReadSMSByIndex(uint8_t index); // reads an sms at a particular index
	bool DelSMSByIndex(uint8_t index); //delete sms by index
	bool DelAllSMS(); // deletes all sms 
	 
 };
 
 #endif
	 

	 
	 
	 
	 
	 