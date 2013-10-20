#ifdef INST
#else

#define INST

class c_Control
{
public:
	bool Branch, BLTZ, B, Jump, ShiftSrc, ALUsrc, FlagWrite, ShiftSel, MemWrite, MemRead, W_B, MemToReg, RegWrite, SignExt, LockRs1, Nop;
	int ALUctr;
	
	void reset()
	{
		Branch = BLTZ = B = Jump = ShiftSrc = ALUsrc = FlagWrite = ShiftSel = MemWrite = MemRead = W_B  = MemToReg = RegWrite = SignExt = LockRs1 = false;
		ALUctr = 0;
	}

	void parse(int operation, int function)
	{
		reset();
		if (Nop) return;
		
		LockRs1 = operation == 0x3A || operation == 0x1A || operation == 0x10 || operation == 0x11;
		SignExt = !(operation == 0x38 || operation == 0x18 || operation == 0x20 || (operation == 0x00 && function == 0x00));
		//判断分支、跳转语句
		//0xBC: B imm24  
		if(operation == 0xBC)
			B = 1;
		else if(operation == 0x10)
			Jump = 1;
		else if(operation == 0xB4)//约定beq为0xB4
			Branch = 1;
		else if (operation == 0xBD)
			BLTZ = true;
		
		//R型指令
		else if(operation == 0x08 || operation == 0x04 || operation == 0x00 || operation == 0x18 || operation == 0x1A)
		{
			ShiftSrc = 0;//移位器来源：rs2
			ShiftSel = 0;
			ALUsrc = 0;
			FlagWrite = 0;
			MemWrite = 0;
			MemRead = 0;
			W_B = 0;
			MemToReg = 0;
			RegWrite = 1;
			
			switch(operation){
			case 0x04://减：001
				ALUctr = SUB;
				break;
			case 0x00:
				
				if(function == 0x09) //乘：010
				{
					ALUctr = MUL;
				}
				else //与：011
				{
					ALUctr = AND;
				}
				break;
			case 0x18: //或：100
				ALUctr = OR;
				break;
			case 0x08:	//加:000
			case 0x1A:	//MOV指令也是加法:000
				ALUctr = ADD;
				break;
			}
		}

		//上述指令的立即数版本
		else if(operation == 0x28 || operation == 0x24 || operation == 0x20 || operation == 0x38 || operation == 0x3A)
		{
			ShiftSrc = 1;//移位器来源：imm9
			ShiftSel = 1;
			ALUsrc = 0;
			FlagWrite = 0;
			MemWrite = 0;
			MemRead = 0;
			W_B = 0;
			MemToReg = 0;
			RegWrite = 1;
			switch(operation){
			case 0x24://减：001
				ALUctr = SUB;
				break;
			case 0x20: //与：010
				ALUctr = AND;
				break;
			case 0x38: //或：100
				ALUctr = OR;
				break;
			case 0x28:	//加:000
			case 0x3A:	//MOV指令也是加法:000
				ALUctr = ADD;
				break;
			}
		}

		//0x7D: LDB rd [rs1+] imm14 
		//0x5D: LDB rd, [rs1+], rs2<<imm5
		//0x79: LDW rd [rs1+] imm14 
		//0x59: LDW rd, [rs1+], rs2<<imm5
		else if(operation == 0x7D || operation == 0x5D || operation == 0x79 || operation == 0x59)
		{
			
			ShiftSrc = 0;		//计算rs2<<imm5
			ShiftSel = 0;

			if(operation == 0x7D || operation == 0x79)
				ALUsrc = 1;		//ALU第二个操作数是imm14扩展结果
			else 
				ALUsrc = 0;		//ALU第二个操作数是rs2扩展结果
			
			ALUctr = ADD;
			
			FlagWrite = 0;	
			
			MemWrite = 0;
			MemRead = 1;
			
			if(operation == 0x7D || operation == 0x5D)
				W_B = 1;
			else 
				W_B = 0;
			
			MemToReg = 1;
			RegWrite = 1;
			
		
		}

		//0x74: STB rd [rs1-] imm14 
		//0x5C: STB rd, [rs1+], rs2<<imm5
		//0x78: STW rd [rs1+] imm14 
		//0x58: STW rd, [rs1+], rs2<<imm5
		else if(operation == 0x74 || operation == 0x5C || operation == 0x78 || operation == 0x58)
		{
			
			ShiftSrc = 0;		//计算rs2<<imm5
			ShiftSel = 0;
			if(operation == 0x74 || operation == 0x78)
				ALUsrc = 1;		//ALU第二个操作数是imm14扩展结果
			else 
				ALUsrc = 0;		//ALU第二个操作数是rs2扩展结果

			ALUctr = operation == 0x74 ? SUB : ADD;
			FlagWrite = 0;	
			
			MemWrite = 1;
			MemRead = 0;
			if(operation == 0x74 || operation == 0x5C)
				W_B = 1;
			else 
				W_B = 0;
			
			MemToReg = 0;
			RegWrite = 0;
		}


		//0x15: SUBCMP.A rs1, rs2<<imm5
		//0x35: SUBCMP.A rs1, imm9<<imm5
		if(operation == 0x15 || operation == 0x35)
		{
			if(operation == 0x15)
			{
				ShiftSrc = 0;	//移位器来源：rs2
				ShiftSel = 0;
			}
			else 
			{
				ShiftSrc = 1;	//移位器来源：imm9
				ShiftSel = 1;
			}
			ALUsrc = 0;
			ALUctr = SUB;
			FlagWrite = 1;
			MemWrite = 0;
			MemRead = 0;
			W_B = 0;
			MemToReg = 0;
			RegWrite = 0;	//不写回	
		}
	}

	string toString()
	{
		string szRet = "Control:\n";
		
		szRet += "\tBranch: " + g_toString(Branch) + "\n";
		szRet += "\tBLTZ: " + g_toString(BLTZ) + "\n";
		szRet += "\tB: " + g_toString(B) + "\n";
		szRet += "\tJump: " + g_toString(Jump) + "\n";
		szRet += "\tShiftSrc: " + g_toString(ShiftSrc) + "\n";
		szRet += "\tALUsrc: " + g_toString(ALUsrc) + "\n";
		szRet += "\tALUctr: " + g_toString(ALUctr) + "\n";
		szRet += "\tFlagWrite: " + g_toString(FlagWrite) + "\n";
		szRet += "\tShiftSel: " + g_toString(ShiftSel) + "\n";
		szRet += "\tMemWrite: " + g_toString(MemWrite) + "\n";
		szRet += "\tMemRead: " + g_toString(MemRead) + "\n";
		szRet += "\tW_B: " + g_toString(W_B) + "\n";
		szRet += "\tMemToReg: " + g_toString(MemToReg) + "\n";
		szRet += "\tRegWrite: " + g_toString(RegWrite) + "\n";
		szRet += "\tSignExt: " + g_toString(SignExt) + "\n";
		szRet += "\tLockRs1: " + g_toString(LockRs1) + "\n";
		szRet += "\tNop: " + g_toString(Nop) + "\n";

		return szRet;
	}
};

class c_Inst
{
public:
	c_Control control;
	int operation, rs1, rd, imm5, function, rs2, imm9, imm14, imm24, nCode, busA, busB, busD, ALUOut, MEMOut, PC_4;
	bool Hazard;
	
	c_Inst(int _nCode): nCode(_nCode)
	{
		operation = rs1 = rd = imm5 = function = rs2 = imm9 = imm14 = imm24 = busA = busB = busD = ALUOut = MEMOut = PC_4 = 0;
		Hazard = false;

		operation = (nCode >> 24) & 255;
		rs1 = (nCode >> 19) & 31;
		rd = (nCode >> 14) & 31;
		imm5 = (nCode >> 9) & 31;
		function = (nCode >> 5) & 15;
		rs2 = nCode & 31;
		
		control.Nop = nCode == 0;
		control.parse(operation, function);
		
		imm9 = extend(nCode & 511, 9, control.SignExt);
		imm14 = extend(nCode & 16383, 14, control.SignExt);
		imm24 = extend(nCode & 16777215, 24, control.SignExt);
	}
	
	string toString()
	{
		string szRet = "Instruction: " + g_toBinaryString(nCode) + "\n";
		
		szRet += "\tOperation: " + g_toHexString(operation) + "\n";
		szRet += "\tFunction: " + g_toHexString(function) + "\n";

		szRet += "\tRs1: " + g_toString(rs1) + "\n";
		szRet += "\tRs2: " + g_toString(rs2) + "\n";
		szRet += "\tRd: " + g_toString(rd) + "\n";

		szRet += "\tImm5: " + g_toString(imm5) + "\n";
		szRet += "\tImm9: " + g_toString(imm9) + "\n";
		szRet += "\tImm14: " + g_toString(imm14) + "\n";
		szRet += "\tImm24: " + g_toString(imm24) + "\n";
		
		szRet += "\n" + control.toString();
		return szRet;
	}
};

#endif