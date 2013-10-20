#ifdef ASM
#else

#define ASM
#define MAX_CMD_LEN 80

int addOp(istringstream &istream)
{
	const int REGOP = 8, IMMOP = 40, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm9, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
	
	if (szArg[0] == '$')//R型指令
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
			nImm5 = 0;
		}
		else
		{
			nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
	}
	else
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nImm9 = atoi(szArg.c_str());
			nImm5 = 0;
		}
		else
		{
			nImm9 = atoi(szArg.substr(0, nLtPos).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (nImm9 & 511);
	}
}

int subOp(istringstream &istream)
{
	const int REGOP = 4, IMMOP = 36, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm9, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
	
	if (szArg[0] == '$')//R型指令
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
			nImm5 = 0;
		}
		else
		{
			nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
	}
	else
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nImm9 = atoi(szArg.c_str());
			nImm5 = 0;
		}
		else
		{
			nImm9 = atoi(szArg.substr(0, nLtPos).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (nImm9 & 511);
	}
}

int mulOp(istringstream &istream)
{
	const int REGOP = 0, FUNCT = 9, nImm5 = 0;

	int nRd, nRs1, nRs2;
	string szRd, szRs1, szRs2;
	istream >> szRd >> szRs1 >> szRs2;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
	nRs2 = atoi(szRs2.substr(1, szRs2.length() - 1).c_str());
	
	return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
}

int andOp(istringstream &istream)
{
	const int REGOP = 0, IMMOP = 32, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm9, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
	
	if (szArg[0] == '$')//R型指令
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
			nImm5 = 0;
		}
		else
		{
			nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
	}
	else
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nImm9 = atoi(szArg.c_str());
			nImm5 = 0;
		}
		else
		{
			nImm9 = atoi(szArg.substr(0, nLtPos).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (nImm9 & 511);
	}
}

int orOp(istringstream &istream)
{
	const int REGOP = 24, IMMOP = 56, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm9, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
	
	if (szArg[0] == '$')//R型指令
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
			nImm5 = 0;
		}
		else
		{
			nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
	}
	else
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nImm9 = atoi(szArg.c_str());
			nImm5 = 0;
		}
		else
		{
			nImm9 = atoi(szArg.substr(0, nLtPos).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (nImm9 & 511);
	}
}

int movOp(istringstream &istream)
{
	const int REGOP = 26, IMMOP = 58, FUNCT = 0, nRs1 = 0;

	int nRd, nRs2, nImm9, nImm5;
	string szRd, szArg;
	istream >> szRd >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
		
	if (szArg[0] == '$')//R型指令
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
			nImm5 = 0;
		}
		else
		{
			nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
	}
	else
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nImm9 = atoi(szArg.c_str());
			nImm5 = 0;
		}
		else
		{
			nImm9 = atoi(szArg.substr(0, nLtPos).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (nImm9 & 511);
	}
}

int ldbOp(istringstream &istream)
{
	const int REGOP = 93, IMMOP = 125, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm14, nImm5;
	string szRd, szRs1, szArg;
	
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD

	if (szArg.length() > 0)
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
		
		if (szArg[0] == '$')//R型指令
		{
			int nLtPos = szArg.find_first_of('<');
			if (nLtPos == string::npos)//没有移位
			{
				nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
				nImm5 = 0;
			}
			else
			{
				nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
				nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
			}

			return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
		}
		else
		{
			nImm14 = atoi(szArg.c_str());
			return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
		}
	}
	else
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 1).c_str());//RS1
		nImm14 = 0;
		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
	}
}

int ldwOp(istringstream &istream)
{
	const int REGOP = 89, IMMOP = 121, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm14, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	
	if (szArg.length() > 0)
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
		
		if (szArg[0] == '$')//R型指令
		{
			int nLtPos = szArg.find_first_of('<');
			if (nLtPos == string::npos)//没有移位
			{
				nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
				nImm5 = 0;
			}
			else
			{
				nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
				nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
			}

			return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
		}
		else
		{
			nImm14 = atoi(szArg.c_str());
			return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
		}
	}
	else
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 1).c_str());//RS1
		nImm14 = 0;
		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
	}
}

int stbOp(istringstream &istream)
{
	const int REGOP = 92, IMMOP = 116, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm14, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	
	if (szArg.length() > 0)
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
		
		if (szArg[0] == '$')//R型指令
		{
			int nLtPos = szArg.find_first_of('<');
			if (nLtPos == string::npos)//没有移位
			{
				nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
				nImm5 = 0;
			}
			else
			{
				nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
				nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
			}

			return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
		}
		else
		{
			nImm14 = atoi(szArg.c_str());
			return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
		}
	}
	else
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 1).c_str());//RS1
		nImm14 = 0;
		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
	}
}

int stwOp(istringstream &istream)
{
	const int REGOP = 88, IMMOP = 120, FUNCT = 0;

	int nRd, nRs1, nRs2, nImm14, nImm5;
	string szRd, szRs1, szArg;
	istream >> szRd >> szRs1 >> szArg;
	
	nRd = atoi(szRd.substr(1, szRd.length() - 2).c_str());//RD
	
	if (szArg.length() > 0)
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
		
		if (szArg[0] == '$')//R型指令
		{
			int nLtPos = szArg.find_first_of('<');
			if (nLtPos == string::npos)//没有移位
			{
				nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
				nImm5 = 0;
			}
			else
			{
				nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
				nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
			}

			return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
		}
		else
		{
			nImm14 = atoi(szArg.c_str());
			return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
		}
	}
	else
	{
		nRs1 = atoi(szRs1.substr(1, szRs1.length() - 1).c_str());//RS1
		nImm14 = 0;
		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + (nImm14 & 16383);
	}
}

int cmpsubaOp(istringstream &istream)
{
	const int REGOP = 21, IMMOP = 53, FUNCT = 0, nRd = 0;

	int nRs1, nRs2, nImm9, nImm5;
	string szRs1, szArg;
	istream >> szRs1 >> szArg;
	
	nRs1 = atoi(szRs1.substr(1, szRs1.length() - 2).c_str());//RS1
	
	if (szArg[0] == '$')//R型指令
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nRs2 = atoi(szArg.substr(1, szArg.length() - 1).c_str());
			nImm5 = 0;
		}
		else
		{
			nRs2 = atoi(szArg.substr(1, nLtPos - 1).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (REGOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
	}
	else
	{
		int nLtPos = szArg.find_first_of('<');
		if (nLtPos == string::npos)//没有移位
		{
			nImm9 = atoi(szArg.c_str());
			nImm5 = 0;
		}
		else
		{
			nImm9 = atoi(szArg.substr(0, nLtPos).c_str());
			nImm5 = atoi(szArg.substr(nLtPos + 2, szArg.length() - nLtPos - 2).c_str());
		}

		return (IMMOP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (nImm9 & 511);
	}
}

int bOp(int nImm24)
{
	const int OP = 188;
	
	return (OP << 24) + (nImm24 & 16777215);
}

int beqOp(int nImm24)
{
	const int OP = 180;
	
	return (OP << 24) + (nImm24 & 16777215);
}

int bltzOp(int nImm24)
{
	const int OP = 189;
	
	return (OP << 24) + (nImm24 & 16777215);
}

int jumpOp(istringstream &istream)
{
	const int OP = 16, FUNCT = 9, nRs1 = 31, nRd = 31, nImm5 = 0;

	int nRs2;
	string szRs2;
	istream >> szRs2;
	
	nRs2 = atoi(szRs2.substr(1, szRs2.length() - 1).c_str());//RS2
	return (OP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
}

int jumplOp(istringstream &istream)
{
	const int OP = 17, FUNCT = 9, nRs1 = 31, nRd = 31, nImm5 = 0;

	int nRs2;
	string szRs2;
	istream >> szRs2;
	
	nRs2 = atoi(szRs2.substr(1, szRs2.length() - 1).c_str());//RS2
	return (OP << 24) + (nRs1 << 19) + (nRd << 14) + ((nImm5 & 31) << 9) + (FUNCT << 5) + nRs2;
}

#endif