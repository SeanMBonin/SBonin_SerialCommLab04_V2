#include "ComPort.h"
#include <codecvt>

ComPort::ComPort(std::string name)
{
	_portName = name;

	//Convert std::string --> std::wstring --> LPCWSTR
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wideName = converter.from_bytes(name);
	_LPName = wideName.c_str();

	//Create my flag events
	_eventRead = CreateEvent(NULL, TRUE, TRUE, NULL); //NULL Security, TRUE manual resets, TRUE start state, NULL lpname
	_eventWrite = CreateEvent(NULL, TRUE, FALSE, NULL);

	//What I consider the standard settings for use.
	_dcb.BaudRate = 9600;
	_dcb.ByteSize = 8;
	_dcb.Parity = NOPARITY;
	_dcb.StopBits = ONESTOPBIT;

	//create file, in essence open the port -- non-sharable
	_hPort = CreateFile(
		_LPName,						//Name of serial port
		GENERIC_READ | GENERIC_WRITE,	//Access type
		0,								//shareMode 0 --> Will not share the port
		0,								//No securityAttributes set (optional)
		OPEN_EXISTING,					//Opens device (or COMPORT in this case)
		0,								//dwFlagsAndAttributes 0.  Using non-overlapping IO
		NULL							//optional (OPEN_EXISTING would cause this to be ignored regardless)
	);	

	//Define my CommTimeout structure
	_cto.ReadIntervalTimeout = MAXDWORD;
	_cto.ReadTotalTimeoutMultiplier = 0;	//The three read intervals setup like this cause the ReadFile() function
	_cto.ReadTotalTimeoutConstant = 0;		//To only read what is immediately available and return true regardless of empty buffer
	_cto.WriteTotalTimeoutConstant = 100;
	_cto.WriteTotalTimeoutMultiplier = 100;	//the two write timeouts are somewhat arbitrarily set.

	SetCommTimeouts(_hPort, &_cto);			//timeout structure applied to _hport functions.
}

ComPort::~ComPort()
{
	if (_hPort)
	{
		CloseHandle(_hPort);
		CloseHandle(_eventRead);
		CloseHandle(_eventWrite);
	}
}

BOOL ComPort::portStatus()
{
	if (_hPort) { return true; }
	return false;
}

HANDLE* ComPort::getPort()
{
	return &_hPort;
}


std::string ComPort::getName()
{
	return _portName;
}

BOOL ComPort::setBaud(int baud)
{
	//If value passed through is one of these 5 common baud rates, the baud rate will be changed.
	//All other values return false --> rate not changed.
	switch (baud) {
	case 1200:
	case 2400:
	case 4800:
	case 9600:
	case 19200:
		_dcb.BaudRate = baud;
		break;
	default:
		return false;
	};

	if (_dcb.BaudRate == baud)
	{
		return true;
	}
	return false;
}

int ComPort::getBaud()
{
	return _dcb.BaudRate;
}

BOOL ComPort::setDataBits(int dB)
{
	if (dB == 7 || dB == 8)
	{
		_dcb.ByteSize = dB;
		if (_dcb.ByteSize == dB) { return true; }
	}
	return false;
}

int ComPort::getDataBits()
{
	return _dcb.ByteSize;
}

BOOL ComPort::setStopBits(int sB)
{
	if (sB >= 0 && sB <= 2)
	{
		_dcb.StopBits = sB;
		if (_dcb.StopBits == sB) { return true; }
	}
	return false;
}

int ComPort::getStopBits()
{
	return _dcb.StopBits;
}

//0 -> no parity, 1 -> odd parity, 2 -> even parity
//3 -> mark parity, 4 -> space parity
BOOL ComPort::setParity(int par)
{
	//adjusts both IO parity properties
	if (par > 0 && par <= 4)
	{
		_dcb.fParity = true;
		_dcb.Parity = par;
	}
	else if (par == 0)
	{
		_dcb.fParity = false;
		_dcb.Parity = par;
	}

	if (_dcb.Parity == par) { return true; }
	return false;
}

int ComPort::getParity()
{
	return _dcb.Parity;
}

int ComPort::readPort(unsigned char passBuff[])
{
	WaitForSingleObject(_eventRead, INFINITE);
	int lth = 0, attempt = 0;
	SetCommMask(_hPort, EV_RXCHAR | EV_ERR); //receive character event (might be removed)

	//Read the buffer (if all of buffer isn't present at read, will be missed)
	//comtimeouts will only read what is immediately availabe for reading
	//Find SOH
	while (_readBuffer[0] != 1)
	{
		attempt++;
		if (ReadFile(_hPort, &_readBuffer[0], 1, &_bytesRead, NULL))
		{
			//Sets the file pointer ahead by 1, program won't work without
			SetFilePointer(_hPort, 1, NULL, 1);
		}

		//Keep from locking up waiting for SOH
		if (attempt > 9)
		{
			return 0;
		}
	}

	//Find length
	if (ReadFile(_hPort, &_readBuffer[1], 1, &_bytesRead, NULL))
	{
		//Sets the file pointer ahead by 1, program won't work without
		SetFilePointer(_hPort, 1, NULL, 1);
		lth = _readBuffer[1];
	}

	for (int ndx = 2; ndx < lth; ndx++)
	{
		if (ReadFile(_hPort, &_readBuffer[ndx], 1, &_bytesRead, NULL))
		{
			SetFilePointer(_hPort, 1, NULL, 1);
			//printf("read file\n");
		}
		else
		{
			printf("Failed to ReadFile; Error: %lu\n", GetLastError());
		}
	}

	for (int ndx = 0; ndx < lth; ndx++)
	{
		passBuff[ndx] = _readBuffer[ndx];
		_readBuffer[ndx] = '\0';
	}

	if (lth > 3)
	{
		return passBuff[1];
	}
	else
	{
		return -5;
	}
}

int ComPort::writePort(unsigned char passBuff[])
{
	//Forces read operations to pause.  Read operations are a blocking operation.
	ResetEvent(_eventRead);

	int length = passBuff[1];
	//copy buffer for safety reasons
	for (int i = 0; i < length; i++)
	{
		_writeBuffer[i] = passBuff[i];
	}

	if (WriteFile(_hPort, &_writeBuffer, length, &_bytesWrt, NULL))
	{
		SetEvent(_eventRead); //Turns read back on
		return length;
	}
	else
	{
		printf("Failed to WriteFile; Error: %lu\n", GetLastError());
	}

	SetEvent(_eventRead); //Turns read back on
	return -1;
}