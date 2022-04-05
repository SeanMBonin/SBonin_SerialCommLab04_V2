#ifndef CRC_H
#define CRC_H
class CRC
{
private:
	unsigned char crcBuff[257]; //Full array + 1 can't be utilized by other variables
	int length;
	short reg;
	const short regMask = 0x125;

public:
	bool crcCheck(unsigned char passBuff[], int lt);
	int crcAdd(unsigned char passBuff[], int lt);
};

#endif