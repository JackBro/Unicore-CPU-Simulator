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

	(*fs) << "===================bubble sort����================"<<endl;
	(*fs) << "���ܣ����ڴ��ַ0,4,8,12�ĸ��ֵ���������"<<endl;
	(*fs) << "��ʼ����ַ0��10(A)�� ��ַ4: 9, ��ַ8��8, ��ַ12��7"<<endl;
	
	test_bubble();
	(*fs) << "��������ַ0��7�� ��ַ4: 8, ��ַ8��9, ��ַ12��10(A)"<<endl;
		
	(*fs) << "=====================swap����====================="<<endl;
	(*fs) << "���ܣ����ڴ��ַ0�͵�ַ4�����ݻ���"<<endl;
	(*fs) << "��ʼ����ַ0��100(0x64) ��ַ4: 200(0xc8)"<<endl;
	
	test_swap();
	(*fs) << "��������ַ0��200(0xc8) ��ַ4: 200(0x64)"<<endl;
	
	(*fs) << "=====================add10����====================="<<endl;
	(*fs) << "���ܣ����ڴ��ַ0��ʼ����10��Ԫ�ص����飬ÿ��Ԫ�ؼ�10"<<endl;
	
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