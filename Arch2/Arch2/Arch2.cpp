#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
using namespace std;

#include "utility.h"
#include "asm.h"
#include "instruction.h"
#include "cache.h"
#include "unit.h"
#include "CPU.h"

void test_add10();
void test_swap();
void test_bubble();

ofstream* fs;

int main()
{
	fs = new ofstream("cpu.out", ios::out);

	(*fs) << "===================bubble sort测试================"<<endl;
	(*fs) << "功能：将内存地址0,4,8,12四个字的内容排序"<<endl;
	(*fs) << "开始：地址0：10(A)， 地址4: 9, 地址8：8, 地址12：7"<<endl;
	
	test_bubble();
	(*fs) << "结束：地址0：7， 地址4: 8, 地址8：9, 地址12：10(A)"<<endl;
		
	(*fs) << "=====================swap测试====================="<<endl;
	(*fs) << "功能：将内存地址0和地址4的内容互换"<<endl;
	(*fs) << "开始：地址0：100(0x64) 地址4: 200(0xc8)"<<endl;
	
	test_swap();
	(*fs) << "结束：地址0：200(0xc8) 地址4: 200(0x64)"<<endl;
	
	(*fs) << "=====================add10测试====================="<<endl;
	(*fs) << "功能：将内存地址0开始，含10个元素的数组，每个元素加10"<<endl;
	
	test_add10();
	

	fs->close();
	delete fs;

	return 0;
}

void test_add10()
{
	c_CPU CPU;
	CPU.fs = fs;
	for(int i = 0; i < 10; i++)
		CPU.DMemory.saveWord(i * 4, 20 - i);
	CPU.setInst("test_add10.dat");
	CPU.clock();
	(*fs) << "Data "<< CPU.DMemory.toString();
	
}

void test_swap()
{
	c_CPU CPU;
	CPU.fs = fs;

	CPU.DMemory.saveWord(0, 100);
	CPU.DMemory.saveWord(4, 200);
	CPU.setInst("test_swap.dat");
	CPU.clock();
	(*fs) << "Data "<< CPU.DMemory.toString();
	
}

void test_bubble()
{
	c_CPU CPU;
	CPU.fs = fs;

	for(int i = 0; i < 4; i++)
		CPU.DMemory.saveWord(i * 4, 10 - i);
	CPU.setInst("test_bubble.dat");
	CPU.clock();
	(*fs) << "Data "<< CPU.DMemory.toString();
	
}