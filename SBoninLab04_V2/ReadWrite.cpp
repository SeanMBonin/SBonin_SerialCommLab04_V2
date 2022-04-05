#include "ReadWrite.h"

ReadWrite::ReadWrite(ComPort* cp)
{
	_cPort = cp;
}

void ReadWrite::printRead(int lth)
{
	//Show SOH and length highlighted by themselves
	std::cout << "SOH: " << std::hex << (int)_rdBuff[0] << std::endl;
	std::cout << "Length: " << (int)_rdBuff[1] << std::endl;
	std::cout << "MSG:  ";

	//Displays the actual message
	for (int i = 2; i < lth - 1; i++)
	{
		std::cout << _rdBuff[i];
	}

	std::cout << std::endl;

	//Show message characters grouped in 16s
	for (int i = 2; i < lth - 1; i++)
	{
		if (i % 16 == 0)
		{
			std::cout << std::endl;
		}

		std::cout << (int)_rdBuff[i] << " ";
	}
	std::cout << std::endl << "CRC Value: " << (int)_rdBuff[lth - 1] << std::endl;
}

int ReadWrite::processRead(unsigned char rdBuff[], int lth)
{
	//if data read and data in the buffer don't match, throw error
	if (lth != rdBuff[1])
	{
		return -2; //I'm using -2 as a mismatching data error
	}

	//The buffer passed in is copied to the class scope buffer
	for (int i = 0; i < lth; i++)
	{
		_rdBuff[i] = rdBuff[i];
	}

	if (_crc.crcCheck(_rdBuff, lth))
	{
		//Data is correct length and passed CRC check
		printRead(lth);
	}
	else
	{
		//return -3; //-3 signafying CRC check failure
		printRead(lth);
	}

	return lth;
}

int ReadWrite::processWrite(unsigned char wrtBuff[])
{
	//Copy buffer, clear original, detect length
	int l = 0;
	unsigned char crcByte;
	unsigned char buffer[256]{};
	while (wrtBuff[l] != '\0')
	{
		buffer[l + 2] = wrtBuff[l]; //Leaves the array shifted over 2, leaves room for SOH and length
		l++;
	}
	l += 3; //3 extra bytes for SOH, lth, CRC
	//Add SOH, lth, and CRC
	buffer[0] = SOH;
	buffer[1] = l;

	crcByte = _crc.crcAdd(buffer, l);
	buffer[l - 1] = crcByte;

	if (_cPort->writePort(buffer) == l)
	{
		//correct length written
		return l;
	}
	else
	{
		//problem with writing
		return -4; //Flag for non-matching length/length written
	}
}