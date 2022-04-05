/*
Author:			Sean Bonin
Project:		Serial Comm Lab

Date 04/05/2021
Changelog
V2.00			Version 2 of the program compiled. Cleaned up and modified code taken from working version 1

V2.01			ReadWrite class and ComPort class re-organized.  ComPort no longer has the logic to process read/write
				instead read/write.h passes the processed logic to ComPort or receives a read from ComPort and then processes

V2.02			Menu feature added with commands.  Can set baud rate & parity.  Includes help for commands.
				Writing to port confirmed working correctly.

V2.10			
				Read/Write operations debugged.  Works correctly
				baud rate and parity settings still have issues to be worked out.  Settings don't always immediately take effect -> can cause read error
				input validation for parity and baud rate not fully implemented, wrong settings will crash the program.


Limitations:	Not all validation fully stops user from breaking the program.  Baud rate & parity changes don't always take effect immediately, can cause
				an error.  Baud rate of 1200 doesn't seem to be working correctly.  Message size limited to 253 char.
				Mutexes have not been implemented
Credits:
			
			
			https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
			std::string -> std::wstring -> LPCWSTR

			https://en.cppreference.com/w/cpp/thread/mutex/try_lock
			Refresher; mutex try lock

			http://members.ee.net/brey/Serial.pdf
			DCB & read/write file logic;  really helpful starting source

			https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts
			https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcommtimeouts
			COMMTIMEOUTS

			https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
			https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
			https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventa
			Event information

			https://www.tutorialspoint.com/c_standard_library/c_function_printf.htm
			printf functionality

			https://docs.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)?redirectedfrom=MSDN
			More serial communications info (overlapped vs nonoverlapped)
*/
#include <iostream>
#include <thread> 
#include <mutex>
#include <atomic> 
#include <string>
#include <chrono>
#include <codecvt>
#include "ComPort.h"
#include "CRC.h"
#include "ReadWrite.h"

//Defining byte and function ptr
#ifndef BYTE
typedef unsigned char BYTE;
#endif

//stream overload
std::ostream& operator << (std::ostream& out, BYTE b)
{
	out << (int)b;

	return out;
}

//function prototype
std::string getPort();
void startPrint();
bool menu(char, ComPort*);

void reader(ComPort* cPort, ReadWrite* rdWrt, BOOL* _quit)
{
	BYTE rdBuffer[256]{};
	int length = 0;
	int errorDet = 0;

	while (!*_quit)
	{
		// mutex lock
		length = cPort->readPort(rdBuffer);

		if (length > 3) //Only process and print if there is data read
		{
			errorDet = rdWrt->processRead(rdBuffer, length);
			if (errorDet < 0)
			{
				std::cout << "Read Error: " << errorDet << std::endl;
			}
		}

		//reset length at end of loop.
		errorDet = 0;
		length = 0;
		// mutex unlock
	}
}

int main()
{
	//Program Start Routine
	startPrint();
	std::string myPort = getPort();
	ComPort cPort(myPort);
	ReadWrite rdWrt(&cPort);

	char input[253]{};
	BYTE wrtBuff[253]{};
	BOOL quit = false;

	//start thread
	std::thread readTh(reader, &cPort, &rdWrt, &quit);

	while (!quit)
	{
		std::cin.getline(input, 253);

		if (input[0] == '/')
		{
			//only takes the first letter for arg
			if (menu(input[1], &cPort))
			{
				quit = true;
			}

			//Clear input buffer
			for (int i = 0; i < 253; i++)
			{
				if (input[i] != '\0')
				{
					input[i] = '\0';
				}
				else
				{
					i = 253; //Force end loop
				}
			}
		}
		else
		{
			//cin doesn't like BYTE, transfer char to unsigned char
			for (int i = 0; i < 253; i++)
			{
				if (input[i] != '\0')
				{
					wrtBuff[i] = input[i];
					input[i] = '\0'; //clear the input buffer after each pass so this logic keeps working
				}
				else { i = 253; } //force end the loop
			}

			//Send the buffer to be processed and written to port
			if (rdWrt.processWrite(wrtBuff) == -4)
			{
				std::cout << "Error writing to port. Length of message and characters written non-matching" << std::endl;
			}
		}		
	}

	readTh.join();
}

std::string getPort()
{
	std::string com = "COM";
	std::string name;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wideName;
	LPCWSTR LPName;
	int option;
	bool goodPort = false;

	WCHAR lpTargetPath[256];
	char userNum;

	for (int i = 1; i <= 9; i++)
	{
		name = com + std::to_string(i);
		//Convert std::string --> std::wstring --> LPCWSTR
		std::wstring wideName = converter.from_bytes(name);
		LPName = wideName.c_str();


		if (QueryDosDevice(LPName, lpTargetPath, 256))
		{
			std::cout << "AVAILABLE PORT: " << name << std::endl;
		}
	}

	while (!goodPort)
	{
		//User picks their port.
		std::cout << "Which COM port to use?";
		std::cin >> option;
		std::cin.ignore(1000, '\n');

		name = com + std::to_string(option);
		std::wstring wideName = converter.from_bytes(name);
		LPName = wideName.c_str();

		if (QueryDosDevice(LPName, lpTargetPath, 256))
		{
			return name;
		}
		else
		{
			std::cout << "Invalid Port" << std::endl;
		}
	}
	return name;
}

void startPrint()
{
	std::cout <<
		"===========================================================================" << std::endl <<
		"==                                                                       ==" << std::endl <<
		"==     ==========             ================     ====          ====    ==" << std::endl <<
		"==    ==                             ==            == ==        == ==    ==" << std::endl <<
		"==   ==                              ==            ==  ==      ==  ==    ==" << std::endl <<
		"==   ==                              ==            ==   ==    ==   ==    ==" << std::endl <<
		"==    ==                             ==            ==    ==  ==    ==    ==" << std::endl <<
		"==     ========                      ==            ==     ====     ==    ==" << std::endl <<
		"==            ==                     ==            ==              ==    ==" << std::endl <<
		"==             ==                    ==            ==              ==    ==" << std::endl <<
		"==             ==                    ==            ==              ==    ==" << std::endl <<
		"==            ==   ===               ==            ==              ==    ==" << std::endl <<
		"==   ==========    ===        ================     ==              ==    ==" << std::endl <<
		"==                                                                       ==" << std::endl <<
		"===========================================================================" << std::endl <<
		"Serial IM Program V2.10" << std::endl;
}

bool menu(char input, ComPort* cPort)
{
	int option;
	switch (input)
	{
	case 'q':
	case 'Q':
		return true; //(quit flag)
		break;

	case 'i':
	case 'I':
		std::cout << cPort->getName() << " Info" << std::endl;
		std::cout << "Baud Rate: " << cPort->getBaud() << std::endl;
		std::cout << "Parity: " << cPort->getParity() << std::endl;
		std::cout << "Data Bits: " << cPort->getDataBits() << std::endl;
		std::cout << "Stop Bits: " << cPort->getStopBits() << std::endl;
		break;

	case 'b':
	case 'B':
		std::cout << "Current Baud rate is: " << cPort->getBaud() << std::endl;
		std::cout << "Set new baud rate (1200, 2400, 4800, 9600, 19200)" << std::endl;
		std::cin >> option;
		std::cin.ignore(1000, '\n'); //clear \n from cin buffer
		if (cPort->setBaud(option))
		{
			std::cout << "Baud rate set" << std::endl;
		}
		else
		{
			std::cout << "Invalid baud rate" << std::endl;
		}
		break;

	case 'p':
	case 'P':
		std::cout << "Current Parity is: " << cPort->getParity() << std::endl;
		std::cout << "Select a value 0-5 for associated parity" << std::endl;
		std::cin >> option;
		std::cin.ignore(1000, '\n');
		if (cPort->setParity(option))
		{
			std::cout << "Parity set" << std::endl;
		}
		else
		{
			std::cout << "Invalid parity" << std::endl;
		}
		break;

	case 'h':
	case 'H':
		std::cout << "Use '/q' to quit" << std::endl;
		std::cout << "Use '/i' to see current port info" << std::endl;
		std::cout << "Use '/b' to set baud rate" << std::endl;
		std::cout << "Use '/p' to set parity" << std::endl;
		std::cout << "Use '/h' to see help" << std::endl;
		std::cout << "Use '/c' to change port" << std::endl;
		break;

	case 'c':
	case 'C':
		std::cout << "Function currently not implemented" << std::endl;
		std::cout << "Check back later!" << std::endl;
		break;

	default:
		std::cout << "Invalid command.  Use '/h' for help menu." << std::endl;
	}
	return false;
}