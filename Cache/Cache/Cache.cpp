#include <cstring>
#include <iostream>
#include <ctime>
#include <fstream>
#include <list>
#include <map>
#include <queue>
#include <vector>
using namespace std;

#define MAXLONGINT 2147483647

FILE* file;

class Cacheline;
class Trace;
class CachelineWithTrace;
class CachelineWithTracePointerLesser;

class Cache;
class LRUCache;
class OPTCache;

class Cacheline
{
protected:
	unsigned char *arrData;//����
	unsigned nTag, nGroupId, nWayId;
	//��ǩ����ţ�·��
	unsigned nGroupLen, nCachelineLen, nCacheline, nGroupAndCacheline;
	//����λ������128������λ��Ϊ7����Cachelineλ������32�ֽڵ�Cachelineλ��5����Cacheline���ȣ���λ��+��λ����ʾ�ĳ���
	bool bValid, bDirty;//��Чλ

	void loadMem(unsigned nAddress)//�����ڴ�
	{
		fprintf(file, "\tGroup %u Way %u Load M[0x%p].\n", nGroupId, nWayId, nAddress);

		nTag = nAddress >> (nGroupLen + nCachelineLen);
		bValid = true;//��Ϊ��Ч
		bDirty = false;
	}

	void saveMem()//д���ڴ�
	{
		unsigned nAddress = (nTag << (nGroupLen + nCachelineLen)) + (nGroupId << nCachelineLen);
		fprintf(file, "\tGroup %u Way %u Save M[0x%p].\n", nGroupId, nWayId, nAddress);
	}

	void assign(unsigned nNewAddress)//���·����ַ
	{
		unsigned nOldAddressBase = nTag << (nGroupLen + nCachelineLen);//����ַ
		if (bDirty) saveMem();//�����޸ģ�д���أ������ڴ�

		loadMem(nNewAddress);//��ȡ�ڴ�
	}
public:
	Cacheline(unsigned _nGroupId, unsigned _nWayId, unsigned _nGroupLen, unsigned _nCachelineLen)
	{
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
			bDirty = true;
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

class Trace
{
private:
	unsigned nId, nAddress, nNextId;//��ǰ������ţ���ַ����һ����ͬһ��ַ���
public:
	Trace()
	{
		nId = nAddress = nNextId = MAXLONGINT;
	}

	void setId(unsigned _nId)
	{
		nId = _nId;
	}

	unsigned getId()
	{
		return nId;
	}

	void setAddress(unsigned _nAddress)
	{
		nAddress = _nAddress;
	}

	unsigned getAddress()
	{
		return nAddress;
	}

	void setNextId(unsigned _nNextId)
	{
		nNextId = _nNextId;
	}

	unsigned getNextId()
	{
		return nNextId;
	}
};

class CachelineWithTrace : public Cacheline
{
private:
	Trace* pTrace;
public:
	CachelineWithTrace(unsigned _nGroupId, unsigned _nWayId, unsigned _nGroupLen, unsigned _nCachelineLen)
		: Cacheline(_nGroupId, _nWayId, _nGroupLen, _nCachelineLen)
	{
		pTrace = NULL;
	}

	void setTrace(Trace* _pTrace)
	{
		pTrace = _pTrace;
	}

	bool operator<(const CachelineWithTrace& other) const
	{
		if (pTrace == NULL) return false;
		if (other.pTrace == NULL) return true;//�����ҿ���

		return pTrace->getNextId() < other.pTrace->getNextId();
	}
};

class CachelineWithTracePointerLesser
{
public:
	bool operator()(const CachelineWithTrace* p1, const CachelineWithTrace* p2)
	{
		return *p1 < *p2;
	}
};

class Cache//����������(��nGroupLen���0�ͱ��ȫ������)
{
protected:
	unsigned nGroupLen, nCachelineLen, nGroup, nWay;//ǰ��������ͬCacheline������ֱ���������·��
	virtual Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag) = 0;//�滻����
public:
	Cache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay)
	{
		nGroupLen = _nGroupLen;
		nGroup = 1 << nGroupLen;//����
		nCachelineLen = _nCachelineLen;
		nWay = _nWay;//������·��
	}

	unsigned readByte(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue;
		fprintf(file, "Read byte from 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

		nValue = pCacheline->readByte(nAddress);//����ֵ

		return nValue;
	}

	unsigned readWord(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue, i;
		fprintf(file, "Read word from 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

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

				if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
				else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
			}

			nValue |= pCacheline->readByte(nAddress) << (i * 8);
		}

		return nValue;
	}

	void writeByte(unsigned nAddress, unsigned nValue)
	{
		unsigned nChosenGroup, nTag;
		fprintf(file, "Write byte to 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

		pCacheline->writeByte(nAddress, nValue);//�趨ֵ
	}

	void writeWord(unsigned nAddress, unsigned nValue)
	{
		unsigned nChosenGroup, nTag, i;
		fprintf(file, "Write word to 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//��ѡ�е���
		nTag = nAddress >> (nGroupLen + nCachelineLen);//�����ǩ

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

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

				if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
				else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
			}

			pCacheline->writeByte(nAddress, (nValue >> (i * 8)) & 255);
		}
	}

	void execTrace(unsigned nTraceSize, Trace* arrTrace)
	{
		for (unsigned i = 0; i < nTraceSize; ++i) readByte(arrTrace[i].getAddress());
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
	LRUCache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay)
		: Cache(_nGroupLen, _nCachelineLen, _nWay)
	{
		arrGroup = new list<Cacheline*>[nGroup];//������
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (unsigned j = 0; j < nWay; ++j)
			{
				arrGroup[i].push_back(new Cacheline(i, j, nGroupLen, nCachelineLen));//�������
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
};

class OPTCache : public Cache
{
private:
	unsigned nTraceSize, nCurrentTrace;//��¼��������ǰִ�е��ļ�¼��
	Trace* arrTrace;//����
	priority_queue<CachelineWithTrace*, vector<CachelineWithTrace*>, CachelineWithTracePointerLesser> *arrHeap;

	Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag)//OPTѡ�����
	{
		CachelineWithTrace* pCachelineWithTrace = NULL;

		vector<CachelineWithTrace*> lstBackup;
		while (!arrHeap[nChosenGroup].empty() && !arrHeap[nChosenGroup].top()->matchTag(nTag))//�����Ƿ���ͬTag
		{
			lstBackup.push_back(arrHeap[nChosenGroup].top());
			arrHeap[nChosenGroup].pop();
		}

		if (!arrHeap[nChosenGroup].empty())//�ҵ�����ͬTag
		{
			pCachelineWithTrace = arrHeap[nChosenGroup].top();
			arrHeap[nChosenGroup].pop();
			pCachelineWithTrace->setTrace(arrTrace + (nCurrentTrace++));
			arrHeap[nChosenGroup].push(pCachelineWithTrace);//�Ż�

			for (vector<CachelineWithTrace*>::iterator iter = lstBackup.begin(); iter != lstBackup.end(); ++iter) arrHeap[nChosenGroup].push(*iter);//ȫ���Ż�
		}
		else
		{
			for (vector<CachelineWithTrace*>::iterator iter = lstBackup.begin(); iter != lstBackup.end(); ++iter) arrHeap[nChosenGroup].push(*iter);//ȫ���Ż�

			pCachelineWithTrace = arrHeap[nChosenGroup].top();//�ҳ�����ʱ���������
			arrHeap[nChosenGroup].pop();//����
			pCachelineWithTrace->setTrace(arrTrace + (nCurrentTrace++));
			arrHeap[nChosenGroup].push(pCachelineWithTrace);//�Ż�
		}

		return pCachelineWithTrace;
	}
public:
	OPTCache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay, unsigned _nTraceSize, Trace* _arrTrace)
		: Cache(_nGroupLen, _nCachelineLen, _nWay)
	{
		unsigned i, j;
		map<unsigned, Trace*> mapTrace;

		nTraceSize = _nTraceSize;
		arrTrace = _arrTrace;
		nCurrentTrace = 0;

		for (i = 0; i < nTraceSize; ++i)
		{
			Trace* pTrace = arrTrace + i;
			unsigned nTag = pTrace->getAddress() >> (nGroupLen + nCachelineLen);

			map<unsigned, Trace*>::iterator iter;
			iter = mapTrace.find(nTag);

			if (iter != mapTrace.end())
			{
				Trace* pPrevTrace = iter->second;
				pPrevTrace->setNextId(pTrace->getId());//������
			}
			mapTrace[nTag] = pTrace;
		}

		arrHeap = new priority_queue<CachelineWithTrace*, vector<CachelineWithTrace*>, CachelineWithTracePointerLesser>[nGroup];//ÿ����ַһ����
		for (i = 0; i < nGroup; ++i)
		{
			for (j = 0; j < nWay; ++j)
			{
				arrHeap[i].push(new CachelineWithTrace(i, j, nGroupLen, nCachelineLen));
			}
		}
	}

	~OPTCache()
	{
		for (unsigned i = 0; i < nGroup; ++i)
		{
			if (!arrHeap[i].empty())
			{
				CachelineWithTrace* pTop = arrHeap[i].top();
				arrHeap[i].pop();
				delete pTop;
			}
		}
		delete []arrHeap;
	}
};

void test();
int main()
{
	file = fopen("cache.out", "w+");
	test();
	fclose(file);
	return 0;
}

//����trace����
void getArrTrace(Trace arrTrace[], unsigned * pTraceSize, const char * fileName)
{

	const int MAX_CMD_LEN = 30;
	char cmd[MAX_CMD_LEN + 1];
	ifstream f(fileName);

	int nAddr, nSize;
	int id = 0;
	while(f.peek() != EOF)
	{
		f.getline(cmd, MAX_CMD_LEN);
		int i, j;
		for(i = 0; cmd[i] != ':'; i++);
		for(j = i + 1; cmd[j] != 0; j++);
		string szCmd(cmd);
		nAddr = atoi(szCmd.substr(0,i).c_str());
		nSize = atoi(szCmd.substr(i + 1, j).c_str());

		for(i = 0; i < nSize; i++)
		{
			arrTrace[id].setAddress(nAddr + i);
			arrTrace[id].setId(id);
			++id;
		}
	}
	*pTraceSize = id;
}

void test()
{
	Trace *arrTrace = new Trace[100000];
	unsigned nTraceSize;
	getArrTrace(arrTrace, &nTraceSize, "trace_sub.dat");

	//Cache(logÿ·������logÿ�п�ȣ�·��)
	//Cache������128�飬������Ϊ8��ÿ��8��Cache�У���ÿ��Cache��λ��Ϊ256bit��32���ֽڣ�
	LRUCache cache1(7, 5, 8);		
	fprintf(file, "====================================1======================================\n");
	fprintf(file, "=======128�飬������Ϊ8��ÿ��8��Cache�У���ÿ��Cache��λ��Ϊ256bit========\n");
	
	cache1.execTrace(nTraceSize, arrTrace);
	

	//ȫ����Cache��ÿ��Cache��λ��Ϊ256bit��32���ֽڣ����ܴ�СΪ32KB��
	fprintf(file, "====================================2======================================\n");
	fprintf(file, "==========ȫ����Cache(1�飬1024��Cache��)��ÿ��Cache��λ��Ϊ256bit=========\n");
	
	LRUCache cache2(0, 5, 1024);
	cache2.execTrace(nTraceSize, arrTrace);
	

	//OPT cache
	fprintf(file, "====================================2======================================\n");
	fprintf(file, "===========OPT Cache��128�飬������Ϊ8k,ÿ��Cache��λ��Ϊ256bit)============\n");
	
	OPTCache optCache(7, 5, 8, nTraceSize, arrTrace);
	optCache.execTrace(nTraceSize, arrTrace);
	

	//Cache������1·��ÿ·1024�У�ÿ��Cache��λ��Ϊ256bit��32���ֽڣ�
	fprintf(file, "====================================3======================================\n");
	fprintf(file, "==============��1024�飬ÿ��1��Cache�У���ÿ��Cache��λ��Ϊ256bit============\n");
	
	LRUCache cache3(10, 5, 1);		
	cache3.execTrace(nTraceSize, arrTrace);
}