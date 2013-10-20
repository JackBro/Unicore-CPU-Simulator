#ifdef UTILITY
#else

#define UTILITY

#define AND 2
#define SUB 1
#define ADD 0
#define	MUL 4
#define OR 3

string g_toBinaryString(int x)
{
	string szRet;
	for (int i = 31; i >= 0; --i) szRet.append(((x & (1 << i)) > 0) ? "1" : "0");
	return szRet;
};

string g_toHexString(int x)
{
	char cmd[20];
	sprintf(cmd, "%X", x);
	return string(cmd);
}

string g_toString(int x)
{
	char cmd[20];
	sprintf(cmd, "%d", x);
	return string(cmd);
}

string g_toString(unsigned x)
{
	char cmd[20];
	sprintf(cmd, "%u", x);
	return string(cmd);
}

string g_toString(bool b)
{
	return b ? "1" : "0";
}

int extend(int nSrc, int nBit, bool bSignExt)
{
	if (bSignExt && ((nSrc & (1 << (nBit - 1))) != 0))
	{
		for (int i = nBit; i < 32; ++i) nSrc |= 1 << i;
	}
	return nSrc;
}

#endif