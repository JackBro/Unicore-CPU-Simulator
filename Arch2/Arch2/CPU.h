#ifdef CPUH
#else

#define CPUH

class c_CPU
{
private:
	map<int, string> mapCode;
	int nInstCount;
	int nInst;
public:
	LRUCache ICache, DCache;
	c_Reg PC;
	c_RegFile RegFile;

	c_Memory IMemory;
	c_Memory DMemory;

	ostream *fs;
	
	int IFID;
	c_Inst *IDEX, *EXMEM, *MEMWB, *AFTERWB;
	
	bool PCWrite, V, C, Z, N;
	string szHazard;
	
	c_CPU(): ICache(7, 5, 8, &IMemory), DCache(7, 5, 8, &DMemory)
	{
		fs = &cout;

		IFID = 0;
		nInst = 0;
		IDEX = new c_Inst(0);
		EXMEM = new c_Inst(0);
		MEMWB = new c_Inst(0);
		AFTERWB = new c_Inst(0);
		
		PCWrite = true;	
		V = C = Z = N = false;
		
		szHazard = "";
	}
	
	void checkHazard()
	{
		if (IDEX->control.Nop) return;
		
		if (IDEX->control.LockRs1) IDEX->busA = 0;
		else
		{
			if (IDEX->rs1 == EXMEM->rd && EXMEM->control.RegWrite)
			{
				if (!EXMEM->control.MemToReg)
				{
					IDEX->busA = EXMEM->ALUOut;
					szHazard = "A data hazard occured, and data in Reg $" + g_toString(EXMEM->rd) + " had been forwarded from EX stage.";
				}
				else
				{
					IDEX->control.FlagWrite = false;
					IDEX->control.RegWrite = false;
					IDEX->control.MemWrite = false;
					IDEX->Hazard = true;
					return;
				}
			}
			else if (IDEX->rs1 == MEMWB->rd && MEMWB->control.RegWrite)
			{
				IDEX->busA = MEMWB->control.MemToReg ? MEMWB->MEMOut : MEMWB->ALUOut;
				szHazard = "A data hazard occured, and data in Reg $" + g_toString(MEMWB->rd) + " had been forwarded from MEM stage.";
			}
			else
			{
				IDEX->busA = RegFile.read(IDEX->rs1);
			}
		}
		
		if (IDEX->control.ShiftSrc == 0)
		{
			if (IDEX->rs2 == EXMEM->rd && EXMEM->control.RegWrite)
			{
				if (!EXMEM->control.MemToReg)
				{
					IDEX->busB = EXMEM->ALUOut;
					szHazard = "A data hazard occured, and data in Reg $" + g_toString(EXMEM->rd) + " had been forwarded from EX stage.";
				}
				else
				{
					IDEX->control.FlagWrite = false;
					IDEX->control.RegWrite = false;
					IDEX->control.MemWrite = false;
					IDEX->Hazard = true;
					return;
				}
			}
			else if (IDEX->rs2 == MEMWB->rd && MEMWB->control.RegWrite)
			{
				IDEX->busB = MEMWB->control.MemToReg ? MEMWB->MEMOut : MEMWB->ALUOut;
				szHazard = "A data hazard occured, and data in Reg $" + g_toString(MEMWB->rd) + " had been forwarded from MEM stage.";
			}
			else
			{
				IDEX->busB = RegFile.read(IDEX->rs2);
			}
		}
		
		if (IDEX->control.MemWrite)
		{
			if (IDEX->rd == EXMEM->rd && EXMEM->control.RegWrite)
			{
				if (!EXMEM->control.MemToReg)
				{
					IDEX->busD = EXMEM->ALUOut;
					szHazard = "A data hazard occured, and data in Reg $" + g_toString(EXMEM->rd) + " had been forwarded from EX stage.";
				}
				else
				{
					IDEX->control.FlagWrite = false;
					IDEX->control.RegWrite = false;
					IDEX->control.MemWrite = false;
					IDEX->Hazard = true;
					return;
				}
			}
			else if (IDEX->rd == MEMWB->rd && MEMWB->control.RegWrite)
			{
				IDEX->busD = MEMWB->control.MemToReg ? MEMWB->MEMOut : MEMWB->ALUOut;
				szHazard = "A data hazard occured, and data in Reg $" + g_toString(MEMWB->rd) + " had been forwarded from MEM stage.";
			}
			else
			{
				IDEX->busD = RegFile.read(IDEX->rd);
			}
		}
	}

	void IF()
	{
		int nAddr = PC.read();
		
		ICache.setLastMsg("");

		IFID = ICache.read(nAddr, false, true);
		PC.write(nAddr + 4, PCWrite);
	}

	void ID()
	{
		IDEX = new c_Inst(IFID);
		IDEX->PC_4 = PC.read();

		checkHazard();
	}

	void EX()
	{
		EXMEM = IDEX;
		
		int nShiftSrc = EXMEM->control.ShiftSrc ? EXMEM->imm9: EXMEM->busB;
		if (EXMEM->control.ShiftSel)
		{
			int nLow = nShiftSrc & ((1 << EXMEM->imm5) - 1);
			
			nShiftSrc >>= EXMEM->imm5;
			nShiftSrc += nLow << (32 - EXMEM->imm5);
		}
		else nShiftSrc <<= EXMEM->imm5;

		int nALUSrcA, nALUSrcB, nALUOut;
		
		nALUSrcA = EXMEM->busA;
		nALUSrcB = EXMEM->control.ALUsrc ? EXMEM->imm14 : nShiftSrc;

		switch (EXMEM->control.ALUctr)
		{
		case AND:
			nALUOut = nALUSrcA & nALUSrcB;
			break;
		case OR:
			nALUOut = nALUSrcA | nALUSrcB;
			break;
		case MUL:
			nALUOut = nALUSrcA * nALUSrcB;
			break;
		case SUB:
			nALUSrcB = 0 - nALUSrcB;
		case ADD:
			nALUOut = nALUSrcA + nALUSrcB;

			if (EXMEM->control.FlagWrite)
			{
				long long llALUSrcA, llALUSrcB, llALUOut;
				
				llALUSrcA = nALUSrcA;
				llALUSrcB = nALUSrcB;
				
				llALUOut = llALUSrcA + llALUSrcB;
				
				V = nALUOut != llALUOut;
				Z = nALUOut == 0;
				C = nALUOut > llALUOut;
				N = nALUOut < 0;
			}
			break;
		}
		EXMEM->ALUOut = nALUOut;
	}

	void MEM()
	{
		MEMWB = EXMEM;
		
		DCache.setLastMsg("");

		MEMWB->MEMOut = DCache.read(MEMWB->ALUOut, MEMWB->control.W_B, MEMWB->control.MemToReg);
		DCache.write(MEMWB->ALUOut, MEMWB->busD, MEMWB->control.W_B, MEMWB->control.MemWrite);
	}

	void WB()
	{
		AFTERWB = MEMWB;
		int nToWrite = AFTERWB->control.MemToReg ? AFTERWB->MEMOut : AFTERWB->ALUOut;
		RegFile.write(AFTERWB->rd, nToWrite, AFTERWB->control.RegWrite);
	}
	
	
	void setInst(const char * fileName)
	{
		int nAddr = 0;
		map<string,int> mapLblAdd;	//标签-地址

		//读入汇编指令存入inst,记录标签
		ifstream f(fileName);
		while(f.peek() != EOF)//是否文件结束
		{
			char cmd[MAX_CMD_LEN + 1];
			f.getline(cmd, MAX_CMD_LEN);

			int i;
			for (i = 0; cmd[i] != 0; ++i) 
				if (cmd[i] >= 'a' && cmd[i] <= 'z') cmd[i] -= 32;
			
			istringstream istream(cmd);//输入流
			string szLabel;
			istream >> szLabel;
			if(cmd[szLabel.length()-1] == ':')	//如果是标签,存入map
			{
				mapLblAdd[szLabel.substr(0,szLabel.length() - 1)] = nAddr;
			}
			else					//否则，存入inst中
			{
				mapCode[nAddr] = string(cmd);
				nAddr += 4;
			}
		}

		
		//产生机器码
		int nCode;
		for(int i = 0; i < nAddr; i = i + 4)
		{
			istringstream istream(mapCode[i]);//输入流
			string szOp;
			istream >> szOp;
			
			if (szOp == "ADD") nCode = addOp(istream);
			else if (szOp == "SUB") nCode = subOp(istream);		
			else if (szOp == "MUL") nCode = mulOp(istream);
			else if (szOp == "AND") nCode = andOp(istream);
			else if (szOp == "OR") nCode = orOp(istream);
			else if (szOp == "MOV") nCode = movOp(istream);
			else if (szOp == "LDB") nCode = ldbOp(istream);
			else if (szOp == "LDW") nCode = ldwOp(istream);
			else if (szOp == "STB") nCode = stbOp(istream);
			else if (szOp == "STW") nCode = stwOp(istream);
			else if (szOp == "CMPSUB.A") nCode = cmpsubaOp(istream);
			else if (szOp == "JUMP") nCode = jumpOp(istream);
			else if (szOp == "JUMP.L") nCode = jumplOp(istream);
			
			else	//需要标签的指令
			{
				string szLabel;
				int nImm24;
				istream>>szLabel;
				int nAdd = mapLblAdd[szLabel];
				nImm24 = nAdd - (i + 4);

				if (szOp == "B") nCode = bOp(nImm24);
				else if (szOp == "BLTZ") nCode = bltzOp(nImm24);
				else if (szOp == "BEQ") nCode = beqOp(nImm24);
			}
			IMemory.write(i, nCode, false, true);
		}
		nInstCount = nAddr >> 2;
	}
	
	string IF2String()
	{
		string szRet = "PC: 0x" + g_toHexString(PC.read()) + "\t" + "IR: " + g_toBinaryString(IFID) + "\n";
		return szRet;
	}

	string ID2String()
	{
		string szRet = "Instruction: " + g_toBinaryString(IDEX->nCode) + "\n";
		
		szRet += "Operation: " + g_toHexString(IDEX->operation) + "\n";
		szRet += "Function: " + g_toHexString(IDEX->function) + "\n\n";
		
		szRet += "Rs1: " + g_toString(IDEX->rs1) + "\t" + "BusA: " + g_toString(IDEX->busA) + "\n";
		szRet += "Rs2: " + g_toString(IDEX->rs2) + "\t" + "BusB: " + g_toString(IDEX->busB) + "\n";
		szRet += "Rd: " + g_toString(IDEX->rd) + "\t" + "BusD: " + g_toString(IDEX->busD) + "\n";
		
		szRet += "BusA forced to be zero: " + g_toString(IDEX->control.LockRs1) + "\n\n";

		szRet += "Imm5: " + g_toString(IDEX->imm5) + "\n";
		szRet += "Imm9: " + g_toString(IDEX->imm9) + "\n";
		szRet += "Imm14: " + g_toString(IDEX->imm14) + "\n";
		szRet += "Imm24: " + g_toString(IDEX->imm24) + "\n";
				
		return szRet;
	}
	
	string EX2String()
	{
		string szRet = "Instruction: " + g_toBinaryString(EXMEM->nCode) + "\n";
		
		szRet += "BusA: " + g_toString(EXMEM->busA) + "\n";
		szRet += "BusB: " + g_toString(EXMEM->busB) + "\n";
		szRet += "Imm9: " + g_toString(EXMEM->imm9) + "\n";
		szRet += "Imm14: " + g_toString(EXMEM->imm14) + "\n\n";

		szRet += "ShiftSrc: " + g_toString(EXMEM->control.ShiftSrc) + "\t";
		szRet += "ShiftSel: " + g_toString(EXMEM->control.ShiftSel) + "\t";
		szRet += "ALUSrc: " + g_toString(EXMEM->control.ALUsrc) + "\t";
		szRet += "FlagWrite: " + g_toString(EXMEM->control.FlagWrite) + "\t";
		szRet += "SignExt: " + g_toString(EXMEM->control.SignExt) + "\n";
		
		szRet += "ALUOut: " + g_toString(EXMEM->ALUOut) + "\n";

		return szRet;
	}

	string MEM2String()
	{
		string szRet = "Instruction: " + g_toBinaryString(MEMWB->nCode) + "\n";
		
		szRet += "ALUOut: " + g_toString(MEMWB->ALUOut) + "\n";
		szRet += "BusD: " + g_toString(MEMWB->busD) + "\n";
		szRet += "MEMOut: " + g_toString(MEMWB->MEMOut) + "\n\n";
		
		szRet += "W_B: " + g_toString(MEMWB->control.W_B) + "\t";
		szRet += "MemRead: " + g_toString(MEMWB->control.MemToReg) + "\n\n";
		szRet += "MemWrite: " + g_toString(MEMWB->control.MemWrite) + "\n\n";
		
		return szRet;
	}

	string WB2String()
	{
		string szRet = "Instruction: " + g_toBinaryString(AFTERWB->nCode) + "\n";
		
		szRet += "ALUOut: " + g_toString(AFTERWB->ALUOut) + "\n";
		szRet += "MEMOut: " + g_toString(AFTERWB->MEMOut) + "\n";
		szRet += "Rd: " + g_toString(AFTERWB->rd) + "\n\n";

		szRet += "MemToReg: " + g_toString(AFTERWB->control.MemToReg) + "\t";
		szRet += "RegWrite: " + g_toString(AFTERWB->control.RegWrite) + "\n\n";
		
		return szRet;
	}

	string global2String()
	{
		string szRet = "V: " + g_toString(V) + "\tC: " + g_toString(C) + "\tZ: " + g_toString(Z) + "\tN: " + g_toString(N) + "\n";
		
		szRet += RegFile.toString();

		szRet += "Data ";
		szRet += DMemory.toString();		
		
		szRet += "\nInstruction " + ICache.toString() + "\n" + ICache.getLastMsg();

		szRet += "\nData " + DCache.toString() + "\n" + DCache.getLastMsg();
		
		return szRet;
	}
	
	void NextPC()
	{
		if (IDEX->Hazard)
		{
			IFID = 0;
			PC.write(IDEX->PC_4 - 4, true);
			szHazard = "A data hazard occured, so the Instructions in IF/ID and ID/EX became a bubble.";
		}
		else if (IDEX->control.Jump)
		{
			IFID = 0;
			PC.write(IDEX->busB & -4, true);
			szHazard = "A control hazard occured, so the Instruction in IF/ID became a bubble.";
		}
		else if (IDEX->control.B || (IDEX->control.Branch && Z) || (IDEX->control.BLTZ && N))
		{
			IFID = 0;
			PC.write(IDEX->PC_4 + IDEX->imm24  & -4, true);
			szHazard = "A control hazard occured, so the Instruction in IF/ID became a bubble.";
		}
		else
		{
			++nInst;
		}
	}
	
	string NextPC2String()
	{
		string szRet = "Hazard Delay: " + g_toString(IDEX->Hazard) + "\tJump: " + g_toString(IDEX->control.Jump)
			+ "\tB: " + g_toString(IDEX->control.B) + "\tBeq: " + g_toString(IDEX->control.Branch)  + "\tBltz: " + g_toString(IDEX->control.BLTZ) + "\n";
		return szRet;
	}

	void clock()
	{
		int T = 0;		//周期数
		int nEndAddr = (nInstCount + 4) << 2;
		while (PC.read() <= nEndAddr)
		{
			++T;
			WB(); MEM(); EX(); ID(); IF();
			NextPC();
			
			(*fs) << "==========IF==========" << endl;
			(*fs) << IF2String() << endl;
			(*fs) << "==========ID==========" << endl;
			(*fs) << ID2String() << endl;
			(*fs) << "==========EX==========" << endl;
			(*fs) << EX2String() << endl;
			(*fs) << "==========MEM==========" << endl;
			(*fs) << MEM2String() << endl;
			(*fs) << "==========WB==========" << endl;
			(*fs) << WB2String() << endl;
			(*fs) << "==========NEXTPC==========" << endl;
			(*fs) << NextPC2String() << endl;
			(*fs) << "==========GLOBAL==========" << endl;
			(*fs) << global2String() << endl;
			
			delete AFTERWB;
			
			if (szHazard.length() > 0)
			{
				(*fs) << endl << szHazard << endl;
				szHazard.clear();
			}
			
			if (fs == &cout) system("pause");
		}
		(*fs) << "总执行指令数（除空指令）："<<nInst<<endl;
		(*fs) << "总周期数："<<T<<endl;
		(*fs) << "CPI:"<<T/double(nInst)<<endl;
		DCache.writeBackAll();
	}
};

#endif