#include <map>

#ifdef UNIT
#else

#define UNIT

class c_Memory//´æ´¢Æ÷Àà
{
private:
	map<int, unsigned char> mapStorage;
public:
	int loadByte(int nAddr)
	{
		if (mapStorage.find(nAddr) == mapStorage.end()) return 0;
		unsigned char cRet = mapStorage[nAddr];
		return cRet;
	}
	
	void saveByte(int nAddr, int nVal)
	{
		mapStorage[nAddr] = ((unsigned char) nVal);
	}

	int loadWord(int nAddr)
	{
		int nRet = 0;
		for (int i = 0; i < 4; ++i) nRet += loadByte(nAddr + i) << (i * 8);
		return nRet;
	}

	void saveWord(int nAddr, int nVal)
	{
		for (int i = 0; i < 4; ++i)
		{
			saveByte(nAddr + i, (unsigned char) nVal);
			nVal >>= 8;
		}
	}
	
public:
	int read(int nAddr, bool bByteOnly)
	{
		return bByteOnly ? loadByte(nAddr) : loadWord(nAddr);
	}

	void write(int nAddr, int nVal, bool bByteOnly, bool bWriteEnable)
	{
		if (!bWriteEnable) return;
		if (bByteOnly) saveByte(nAddr, nVal);
		else saveWord(nAddr, nVal);
	}

	string toString()
	{
		string szRet = "Memory:";
		
		if (!mapStorage.empty())
		{
			int nTmp = 0;
			szRet += "\n";

			for (map<int, unsigned char>::iterator iter = mapStorage.begin(); iter != mapStorage.end(); ++iter)
			{
				szRet += "0x" + g_toHexString(iter->first) + ": " + g_toHexString(iter->second) + (((nTmp & 3) == 3) ? "\n" : "\t");
				++nTmp;
			}
			
			if ((nTmp & 3) > 0) szRet += "\n";
		}
		else szRet += " Blank\n";
		return szRet;
	}
};

class c_RegFile
{
private:
	int arrStorage[32];
public:
	c_RegFile()
	{
		memset(arrStorage, 0, sizeof(arrStorage));
	}

	int read(int R)
	{
		return arrStorage[R];
	}
	
	void write(int R, int nVal, bool bWriteEnable)
	{
		if (!bWriteEnable) return;
		arrStorage[R] = nVal;
	}

	bool equal(int R1, int R2)
	{
		return arrStorage[R1] == arrStorage[R2];
	}

	string toString()
	{
		string szRet = "Registers:\n";
		for (int j = 0; j < 32; ++j) szRet += g_toString(j) + ": " + g_toString(arrStorage[j]) + (((j & 3) == 3) ? "\n" : "\t");
		return szRet;
	}
};

class c_Reg
{
private:
	int nStorage;
public:
	c_Reg()
	{
		nStorage = 0;
	}

	int read()
	{
		return nStorage;
	}
	
	void write(int nVal, bool bWriteEnable)
	{
		if (!bWriteEnable) return;
		nStorage = nVal;
	}
};

#endif