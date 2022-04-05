#include "CRC.h"

bool CRC::crcCheck(unsigned char passBuff[], int lt)
{
	if (CRC::crcAdd(passBuff, lt) == 0) { return true; }
	else { return false; }
}

int CRC::crcAdd(unsigned char passBuff[], int lt)
{
	for (int i = 0; i <= lt; i++)
	{
		//copy the buffer so the array passed in isn't changed
		crcBuff[i] = passBuff[i];
	}


	//load the register with the first byte
	reg = crcBuff[0];

	for (int i = 1; i < lt; i++)
	{
		for (int bt = 0; bt < 8; bt++)
		{
			reg <<= 1;
			if ((crcBuff[i] << bt) & 0x80)
			{
				reg++;
			}

			if (reg > 0xFF)
			{
				reg ^= regMask;
			}
		}
	}
	return reg;
}