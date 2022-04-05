#ifndef COMPORT_H
#define COMPORT_H

#include <Windows.h>
#include <string>

class ComPort
{
private:
	DCB _dcb;
	COMMTIMEOUTS _cto;
	HANDLE _hPort, _eventRead, _eventWrite;
	std::string _portName;
	LPCWSTR _LPName;

	BOOL _pOpen;	//not even sure if I need this at all.
	unsigned char _writeBuffer[256];
	unsigned char _readBuffer[256];
	DWORD _bytesRead, _bytesWrt;

public:
	ComPort(std::string name);
	~ComPort();

	BOOL portStatus();
	HANDLE* getPort();

	std::string getName();

	BOOL setBaud(int baud);
	int getBaud();

	BOOL setDataBits(int dB);
	int getDataBits();

	BOOL setStopBits(int sB);
	int getStopBits();

	BOOL setParity(int par);
	int getParity();

	int writePort(unsigned char passBuff[]);
	int readPort(unsigned char passBuff[]);
};

#endif