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
	unsigned char *arrData;//数据
	unsigned nTag, nGroupId, nWayId;
	//标签，组号，路号
	unsigned nGroupLen, nCachelineLen, nCacheline, nGroupAndCacheline;
	//组数位长（如128的组数位长为7），Cacheline位长（如32字节的Cacheline位长5），Cacheline长度，组位长+行位长表示的长度
	bool bValid, bDirty;//有效位

	void loadMem(unsigned nAddress)//读入内存
	{
		fprintf(file, "\tGroup %u Way %u Load M[0x%p].\n", nGroupId, nWayId, nAddress);

		nTag = nAddress >> (nGroupLen + nCachelineLen);
		bValid = true;//置为有效
		bDirty = false;
	}

	void saveMem()//写入内存
	{
		unsigned nAddress = (nTag << (nGroupLen + nCachelineLen)) + (nGroupId << nCachelineLen);
		fprintf(file, "\tGroup %u Way %u Save M[0x%p].\n", nGroupId, nWayId, nAddress);
	}

	void assign(unsigned nNewAddress)//重新分配地址
	{
		unsigned nOldAddressBase = nTag << (nGroupLen + nCachelineLen);//基地址
		if (bDirty) saveMem();//若被修改，写返回，存入内存

		loadMem(nNewAddress);//读取内存
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
	unsigned nId, nAddress, nNextId;//当前操作序号，地址，下一操作同一地址序号
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
		if (other.pTrace == NULL) return true;//优先找空行

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

class Cache//组相联缓存(把nGroupLen设成0就变成全相联了)
{
protected:
	unsigned nGroupLen, nCachelineLen, nGroup, nWay;//前三个参数同Cacheline，后面分别是组数和路数
	virtual Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag) = 0;//替换策略
public:
	Cache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay)
	{
		nGroupLen = _nGroupLen;
		nGroup = 1 << nGroupLen;//组数
		nCachelineLen = _nCachelineLen;
		nWay = _nWay;//相联的路数
	}

	unsigned readByte(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue;
		fprintf(file, "Read byte from 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

		nValue = pCacheline->readByte(nAddress);//返回值

		return nValue;
	}

	unsigned readWord(unsigned nAddress)
	{
		unsigned nChosenGroup, nTag, nValue, i;
		fprintf(file, "Read word from 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

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

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

		pCacheline->writeByte(nAddress, nValue);//设定值
	}

	void writeWord(unsigned nAddress, unsigned nValue)
	{
		unsigned nChosenGroup, nTag, i;
		fprintf(file, "Write word to 0x%p:\n", nAddress);

		nChosenGroup = (nAddress >> nCachelineLen) & (nGroup - 1);//被选中的组
		nTag = nAddress >> (nGroupLen + nCachelineLen);//计算标签

		Cacheline* pCacheline = selectWay(nChosenGroup, nTag);

		if (pCacheline->matchTag(nTag)) fprintf(file, "\tHIT at Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());
		else fprintf(file, "\tMISS, use Group %u Way %u.\n", nChosenGroup, pCacheline->getWayId());

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
	LRUCache(unsigned _nGroupLen, unsigned _nCachelineLen, unsigned _nWay)
		: Cache(_nGroupLen, _nCachelineLen, _nWay)
	{
		arrGroup = new list<Cacheline*>[nGroup];//建立组
		for (unsigned i = 0; i < nGroup; ++i)
		{
			for (unsigned j = 0; j < nWay; ++j)
			{
				arrGroup[i].push_back(new Cacheline(i, j, nGroupLen, nCachelineLen));//加入各行
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
	unsigned nTraceSize, nCurrentTrace;//记录个数，当前执行到的记录数
	Trace* arrTrace;//数组
	priority_queue<CachelineWithTrace*, vector<CachelineWithTrace*>, CachelineWithTracePointerLesser> *arrHeap;

	Cacheline* selectWay(unsigned nChosenGroup, unsigned nTag)//OPT选择策略
	{
		CachelineWithTrace* pCachelineWithTrace = NULL;

		vector<CachelineWithTrace*> lstBackup;
		while (!arrHeap[nChosenGroup].empty() && !arrHeap[nChosenGroup].top()->matchTag(nTag))//查找是否相同Tag
		{
			lstBackup.push_back(arrHeap[nChosenGroup].top());
			arrHeap[nChosenGroup].pop();
		}

		if (!arrHeap[nChosenGroup].empty())//找到了相同Tag
		{
			pCachelineWithTrace = arrHeap[nChosenGroup].top();
			arrHeap[nChosenGroup].pop();
			pCachelineWithTrace->setTrace(arrTrace + (nCurrentTrace++));
			arrHeap[nChosenGroup].push(pCachelineWithTrace);//放回

			for (vector<CachelineWithTrace*>::iterator iter = lstBackup.begin(); iter != lstBackup.end(); ++iter) arrHeap[nChosenGroup].push(*iter);//全部放回
		}
		else
		{
			for (vector<CachelineWithTrace*>::iterator iter = lstBackup.begin(); iter != lstBackup.end(); ++iter) arrHeap[nChosenGroup].push(*iter);//全部放回

			pCachelineWithTrace = arrHeap[nChosenGroup].top();//找出结束时间最晚的行
			arrHeap[nChosenGroup].pop();//弹出
			pCachelineWithTrace->setTrace(arrTrace + (nCurrentTrace++));
			arrHeap[nChosenGroup].push(pCachelineWithTrace);//放回
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
				pPrevTrace->setNextId(pTrace->getId());//加入后继
			}
			mapTrace[nTag] = pTrace;
		}

		arrHeap = new priority_queue<CachelineWithTrace*, vector<CachelineWithTrace*>, CachelineWithTracePointerLesser>[nGroup];//每个地址一个堆
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

//生成trace数组
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

	//Cache(log每路行数，log每行宽度，路数)
	//Cache共包含128组，相联度为8（每组8个Cache行），每个Cache行位宽为256bit（32个字节）
	LRUCache cache1(7, 5, 8);		
	fprintf(file, "====================================1======================================\n");
	fprintf(file, "=======128组，相联度为8（每组8个Cache行），每个Cache行位宽为256bit========\n");
	
	cache1.execTrace(nTraceSize, arrTrace);
	

	//全相连Cache，每个Cache行位宽为256bit（32个字节），总大小为32KB；
	fprintf(file, "====================================2======================================\n");
	fprintf(file, "==========全相连Cache(1组，1024个Cache行)，每个Cache行位宽为256bit=========\n");
	
	LRUCache cache2(0, 5, 1024);
	cache2.execTrace(nTraceSize, arrTrace);
	

	//OPT cache
	fprintf(file, "====================================2======================================\n");
	fprintf(file, "===========OPT Cache（128组，相联度为8k,每个Cache行位宽为256bit)============\n");
	
	OPTCache optCache(7, 5, 8, nTraceSize, arrTrace);
	optCache.execTrace(nTraceSize, arrTrace);
	

	//Cache共包含1路，每路1024行，每个Cache行位宽为256bit（32个字节）
	fprintf(file, "====================================3======================================\n");
	fprintf(file, "==============（1024组，每组1个Cache行），每个Cache行位宽为256bit============\n");
	
	LRUCache cache3(10, 5, 1);		
	cache3.execTrace(nTraceSize, arrTrace);
}