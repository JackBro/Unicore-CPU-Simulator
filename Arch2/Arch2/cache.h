#include <list>
#include "unit.h"

#ifdef CACHE
#else

#define CACHE

#define MAXLONGINT 2147483647

class Cache;
class LRUCache;

class Cacheline
{
protected:
	friend class LRUCache;
	Cache *cache;
	c_Memory *memory;
	unsigned char *arrData;//数据
	unsigned nTag, nGroupId, nWayId;
	//标签，组号，路号
	unsigned nGroupLen, nCachelineLen, nCacheline, nGroupAndCacheline;
	//组数位长（如128的组数位长为7），Cacheline位长（如32字节的Cacheline位长5），Cacheline长度，组位长+行位长表示的长度
	bool bValid, bDirty;//有效位
	
	void loadMem(unsigned nAddress);//读入内存
	void saveMem();//写入内存
	
	void assign(unsigned nNewAddress)//重新分配地址
	{
		if (bDirty) saveMem();//若被修改，写返回，存入内存
		loadMem(nNewAddress);//读取内存
	}
public:
	Cacheline(unsigned _nGroupId, unsigned _nWayId, unsigned _nGroupLen, unsigned _nCachelineLen, c_Memory *_memory, Cache *_cache)
	{
		cache = _cache;
		memory = _memory;

		nGroupId = _nGroupId;
		nWayId = _nWayId;
		
		nGroupLen = _nGroupLen;
		nCachelineLen = _nCachelineLen;	

		nCacheline = 1 << nCachelineLen;
		nGroupAndCacheline = 1 << (nGroupLen + nCachelineLen);
		
		arrData = new unsigned char[nCacheline];
		
		bValid = bDirty = false;//无效，非脏位
	}
	
	~Cacheline()
	{
		delete []arrData;
	}
	
	unsigned readByte(unsigned nAddress)//读取某一地址的数据
	{
		bool bHit = true;
		unsigned nNewTag, nIndex, nValue;
		
		nNewTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签
		nIndex = nAddress & (nCacheline - 1);

		if (!bValid || nNewTag != nTag)
		{
			bHit = false;
			assign(nAddress);//失效
		}
		
		nValue = arrData[nIndex];	//返回数值
		return nValue;
	}

	void writeByte(unsigned nAddress, unsigned nValue)//写入某一地址的数据
	{
		bool bHit = true;
		unsigned nNewTag, nIndex;
		
		nNewTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签
		nIndex = nAddress & (nCacheline - 1);//计算存储的位置

		if (!bValid || nNewTag != nTag)
		{
			bHit = false;
			assign(nAddress);//失效
		}
		
		if ((unsigned char) nValue != arrData[nIndex])//如果值改变
		{
			bDirty = true;//脏位置为有效
			arrData[nIndex] = (unsigned char) nValue;
		}
	}
	
	bool matchTag(unsigned nNewTag)
	{
		return nNewTag == nTag;
	}
	
	unsigned getGroupId()
	{
		return nGroupId;
	}

	unsigned getWayId()
	{
		return nWayId;
	}
};

class Cache//组相联缓存(把nGroupLen设成0就变成全相联了)
{
protected:
	string szLastMsg;
	c_Memory *memory;
	unsigned nGroupLen, nCachelineLen, nGroup, nWay;//前三个参数同Cacheline，后面分别是组数和路数
	virtual Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag) = 0;//替换策略
public:
	string getLastMsg() { return szLastMsg; }
	void setLastMsg(string _szLastMsg) { szLastMsg = _szLastMsg; }

	Cache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay, c_Memory *_memory)
	{
		memory = _memory;
		nGroupLen = _nGroupLen;
		nGroup = 1 << nGroupLen;//组数
		nCachelineLen = _nCachelineLen;
		nWay = _nWay;//相联的路数
	}
	
	unsigned readByte(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue;
		szLastMsg += "Read byte from 0x" + g_toHexString(nAddress) + ":\n";
		
		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签
			
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
		
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		nValue = pCacheline->readByte(nAddress);//返回值
		
		return nValue;
	}

	unsigned readWord(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue, i;
		szLastMsg += "Read word from 0x" + g_toHexString(nAddress) + ":\n";

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签
			
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
		
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		nValue = pCacheline->readByte(nAddress);//返回值
		for (i = 1; i < 4; ++i)
		{
			++nAddress;

			unsigned nChosenGroup2 = (nAddress >> nCachelineLen) & (nGroup - 1);
			unsigned nTag2 = nAddress >> (nGroupLen + nCachelineLen);

			if (nChosenGroup != nChosenGroup2 || nTag != nTag2)
			{
				nChosenGroup = nChosenGroup2;
				nTag = nTag2;

				pCacheline = selectWay(nChosenGroup, nTag);

				if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
				else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
			}
			
			nValue |= pCacheline->readByte(nAddress) << (i * 8);
		}
		
		return nValue;
	}
	
	void writeByte(unsigned nAddress, unsigned nValue)
	{
		unsigned nChosenGroup, nTag;
		szLastMsg += "Write byte to 0x" + g_toHexString(nAddress) + ":\n";

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签
		
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
				
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		pCacheline->writeByte(nAddress, nValue);//设定值
	}
	
	void writeWord(unsigned nAddress, unsigned nValue)
	{
		unsigned nChosenGroup, nTag, i;
		szLastMsg += "Write word to 0x" + g_toHexString(nAddress) + ":\n";

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签
		
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
				
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		pCacheline->writeByte(nAddress, nValue & 255);//设定值
		for (i = 1; i < 4; ++i)
		{
			++nAddress;

			unsigned nChosenGroup2 = (nAddress >> nCachelineLen) & (nGroup - 1);
			unsigned nTag2 = nAddress >> (nGroupLen + nCachelineLen);

			if (nChosenGroup != nChosenGroup2 || nTag != nTag2)
			{
				nChosenGroup = nChosenGroup2;
				nTag = nTag2;

				pCacheline = selectWay(nChosenGroup, nTag);

				if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
				else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
			}

			pCacheline->writeByte(nAddress, (nValue >> (i * 8)) & 255);
		}
	}

	unsigned read(unsigned nAddr, bool bByteOnly, bool bMemRead)
	{
		if (!bMemRead) return 0;
		return bByteOnly ? readByte(nAddr) : readWord(nAddr);
	}

	void write(unsigned nAddr, unsigned nVal, bool bByteOnly, bool bWriteEnable)
	{
		if (!bWriteEnable) return;

		if (bByteOnly) writeByte(nAddr, nVal);
		else writeWord(nAddr, nVal);
	}
};

class LRUCache : public Cache//组相联缓存,采用LRU替换策略
{
private:
	list<Cacheline*> *arrGroup;//组

	Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag)//LRU选择策略
	{
		list<Cacheline*>::iterator iter;
		for (iter = --arrGroup[nChosenGroup].end(); !(*iter)->matchTag(nTag) && iter != arrGroup[nChosenGroup].begin(); --iter);//查找标签
		
		Cacheline* pCacheline = *iter;

		arrGroup[nChosenGroup].remove(pCacheline);//删掉
		arrGroup[nChosenGroup].push_back(pCacheline);//加回

		return pCacheline;
	}
public:
	LRUCache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay, c_Memory *_memory)
		: Cache(_nGroupLen, _nCachelineLen, _nWay, _memory)
	{
		arrGroup = new list<Cacheline*>[nGroup];//建立组
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (unsigned j = 0; j < nWay; ++j)
			{
				arrGroup[i].push_back(new Cacheline(i, j, nGroupLen, nCachelineLen, _memory, this));//加入各行
			}
		}
	}
	
	~LRUCache()
	{
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (list<Cacheline*>::iterator iter = arrGroup[i].begin(); iter != arrGroup[i].end(); ++iter)
			{
				delete *iter;
			}
		}
		delete []arrGroup;
	}

	void writeBackAll()
	{
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (list<Cacheline*>::iterator iter = arrGroup[i].begin(); iter != arrGroup[i].end(); ++iter)
			{
				if (((*iter)->bValid) && ((*iter)->bDirty))
				{
					(*iter)->saveMem();
					(*iter)->bDirty = false;
				}
			}
		}
	}
	
	string toString()
	{
		string szRet = "Cache:", szOldLastMsg = szLastMsg;
		bool bBlank = true;
		
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (list<Cacheline*>::iterator iter = arrGroup[i].begin(); iter != arrGroup[i].end(); ++iter)
			{
				Cacheline* pCacheline = *iter;
				if (!pCacheline->bValid) continue;
				
				if (bBlank)
				{
					szRet += "\n";
					bBlank = false;
				}
				
				unsigned nAddress = (pCacheline->nTag << (nGroupLen + nCachelineLen)) + (pCacheline->nGroupId << nCachelineLen), nTmp = 0;//基地址
				szRet += "Group: " + g_toString(pCacheline->getGroupId()) + "\tWay: " + g_toString(pCacheline->getWayId()) + "\tBase Address: " + g_toString(nAddress) + "\n";

				for (unsigned j = 0; j < pCacheline->nCacheline; ++j)
				{
					szRet += "0x" + g_toHexString(j) + ": " + g_toHexString(pCacheline->arrData[j]) + (((nTmp & 3) == 3) ? "\n" : "\t");
					++nTmp;
				}
			}
		}
		if (bBlank) szRet += " Blank\n";
		szLastMsg = szOldLastMsg;	

		return szRet;
	}
};

void Cacheline::loadMem(unsigned nAddress)//读入内存
{
	string szLastMsg = cache->getLastMsg();
	szLastMsg += "\tGroup " + g_toString(nGroupId) + " Way " + g_toString(nWayId) +  " Load M[0x" + g_toHexString(nAddress) + "].\n";
	cache->setLastMsg(szLastMsg);

	for (unsigned i = 0; i < nCacheline; ++i) arrData[i] = memory->read(((int) (nAddress + i)), true); 
	
	nTag = nAddress >> (nGroupLen + nCachelineLen);
	bValid = true;//置为有效
	bDirty = false;
}
	
void Cacheline::saveMem()//写入内存
{
	unsigned nAddress = (nTag << (nGroupLen + nCachelineLen)) + (nGroupId << nCachelineLen);//基地址
	
	string szLastMsg = cache->getLastMsg();
	szLastMsg += "\tGroup " + g_toString(nGroupId) + " Way " + g_toString(nWayId) +  " Save M[0x" + g_toHexString(nAddress) + "].\n";
	cache->setLastMsg(szLastMsg);

	for (unsigned i = 0; i < nCacheline; ++i)  memory->write(((int) (nAddress + i)), ((int) arrData[i]), true, true); 
}

#endif