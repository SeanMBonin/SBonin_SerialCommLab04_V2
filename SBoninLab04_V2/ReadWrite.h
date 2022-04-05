#ifndef READWRITE_H
#define READWRITE_H
#include "ComPort.h"
#include "CRC.h"
#include <iostream>


class ReadWrite
{
	//Needs to do the processing
	//Sits in the main thread, fills a shared buffer or reads
	//a shared buffer

private:
	unsigned char _rdBuff[256], _wrtBuff[256];
	const unsigned char SOH = 1;
	ComPort* _cPort;
	CRC _crc;

public:
	ReadWrite(ComPort* cp);
	void printRead(int lth);
	int processRead(unsigned char rdBuff[], int lth);
	int processWrite(unsigned char wrtBuff[]);
};

#endif