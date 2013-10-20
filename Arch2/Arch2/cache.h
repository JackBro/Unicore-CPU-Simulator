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
	unsigned char *arrData;//����
	unsigned nTag, nGroupId, nWayId;
	//��ǩ����ţ�·��
	unsigned nGroupLen, nCachelineLen, nCacheline, nGroupAndCacheline;
	//����λ������128������λ��Ϊ7����Cachelineλ������32�ֽڵ�Cachelineλ��5����Cacheline���ȣ���λ��+��λ����ʾ�ĳ���
	bool bValid, bDirty;//��Чλ
	
	void loadMem(unsigned nAddress);//�����ڴ�
	void saveMem();//д���ڴ�
	
	void assign(unsigned nNewAddress)//���·����ַ
	{
		if (bDirty) saveMem();//�����޸ģ�д���أ������ڴ�
		loadMem(nNewAddress);//��ȡ�ڴ�
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
		
		bValid = bDirty = false;//��Ч������λ
	}
	
	~Cacheline()
	{
		delete []arrData;
	}
	
	unsigned readByte(unsigned nAddress)//��ȡĳһ��ַ������
	{
		bool bHit = true;
		unsigned nNewTag, nIndex, nValue;
		
		nNewTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ
		nIndex = nAddress & (nCacheline - 1);

		if (!bValid || nNewTag != nTag)
		{
			bHit = false;
			assign(nAddress);//ʧЧ
		}
		
		nValue = arrData[nIndex];	//������ֵ
		return nValue;
	}

	void writeByte(unsigned nAddress, unsigned nValue)//д��ĳһ��ַ������
	{
		bool bHit = true;
		unsigned nNewTag, nIndex;
		
		nNewTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ
		nIndex = nAddress & (nCacheline - 1);//����洢��λ��

		if (!bValid || nNewTag != nTag)
		{
			bHit = false;
			assign(nAddress);//ʧЧ
		}
		
		if ((unsigned char) nValue != arrData[nIndex])//���ֵ�ı�
		{
			bDirty = true;//��λ��Ϊ��Ч
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

class Cache//����������(��nGroupLen���0�ͱ��ȫ������)
{
protected:
	string szLastMsg;
	c_Memory *memory;
	unsigned nGroupLen, nCachelineLen, nGroup, nWay;//ǰ��������ͬCacheline������ֱ���������·��
	virtual Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag) = 0;//�滻����
public:
	string getLastMsg() { return szLastMsg; }
	void setLastMsg(string _szLastMsg) { szLastMsg = _szLastMsg; }

	Cache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay, c_Memory *_memory)
	{
		memory = _memory;
		nGroupLen = _nGroupLen;
		nGroup = 1 << nGroupLen;//����
		nCachelineLen = _nCachelineLen;
		nWay = _nWay;//������·��
	}
	
	unsigned readByte(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue;
		szLastMsg += "Read byte from 0x" + g_toHexString(nAddress) + ":\n";
		
		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ
			
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
		
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		nValue = pCacheline->readByte(nAddress);//����ֵ
		
		return nValue;
	}

	unsigned readWord(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue, i;
		szLastMsg += "Read word from 0x" + g_toHexString(nAddress) + ":\n";

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ
			
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
		
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		nValue = pCacheline->readByte(nAddress);//����ֵ
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

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ
		
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
				
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		pCacheline->writeByte(nAddress, nValue);//�趨ֵ
	}
	
	void writeWord(unsigned nAddress, unsigned nValue)
	{
		unsigned nChosenGroup, nTag, i;
		szLastMsg += "Write word to 0x" + g_toHexString(nAddress) + ":\n";

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ
		
		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);
				
		if (pCacheline->matchTag(nTag)) szLastMsg += "\tHIT at Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		else szLastMsg += "\tMISS, use Group " + g_toString(nChosenGroup) + " Way " + g_toString(pCacheline->getWayId()) + ".\n";
		
		pCacheline->writeByte(nAddress, nValue & 255);//�趨ֵ
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

class LRUCache : public Cache//����������,����LRU�滻����
{
private:
	list<Cacheline*> *arrGroup;//��

	Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag)//LRUѡ�����
	{
		list<Cacheline*>::iterator iter;
		for (iter = --arrGroup[nChosenGroup].end(); !(*iter)->matchTag(nTag) && iter != arrGroup[nChosenGroup].begin(); --iter);//���ұ�ǩ
		
		Cacheline* pCacheline = *iter;

		arrGroup[nChosenGroup].remove(pCacheline);//ɾ��
		arrGroup[nChosenGroup].push_back(pCacheline);//�ӻ�

		return pCacheline;
	}
public:
	LRUCache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay, c_Memory *_memory)
		: Cache(_nGroupLen, _nCachelineLen, _nWay, _memory)
	{
		arrGroup = new list<Cacheline*>[nGroup];//������
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (unsigned j = 0; j < nWay; ++j)
			{
				arrGroup[i].push_back(new Cacheline(i, j, nGroupLen, nCachelineLen, _memory, this));//�������
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
				
				unsigned nAddress = (pCacheline->nTag << (nGroupLen + nCachelineLen)) + (pCacheline->nGroupId << nCachelineLen), nTmp = 0;//����ַ
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

void Cacheline::loadMem(unsigned nAddress)//�����ڴ�
{
	string szLastMsg = cache->getLastMsg();
	szLastMsg += "\tGroup " + g_toString(nGroupId) + " Way " + g_toString(nWayId) +  " Load M[0x" + g_toHexString(nAddress) + "].\n";
	cache->setLastMsg(szLastMsg);

	for (unsigned i = 0; i < nCacheline; ++i) arrData[i] = memory->read(((int) (nAddress + i)), true); 
	
	nTag = nAddress >> (nGroupLen + nCachelineLen);
	bValid = true;//��Ϊ��Ч
	bDirty = false;
}
	
void Cacheline::saveMem()//д���ڴ�
{
	unsigned nAddress = (nTag << (nGroupLen + nCachelineLen)) + (nGroupId << nCachelineLen);//����ַ
	
	string szLastMsg = cache->getLastMsg();
	szLastMsg += "\tGroup " + g_toString(nGroupId) + " Way " + g_toString(nWayId) +  " Save M[0x" + g_toHexString(nAddress) + "].\n";
	cache->setLastMsg(szLastMsg);

	for (unsigned i = 0; i < nCacheline; ++i)  memory->write(((int) (nAddress + i)), ((int) arrData[i]), true, true); 
}

#endif