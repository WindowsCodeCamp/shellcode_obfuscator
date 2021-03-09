#pragma once
#include <vector>
#include <cassert>

// ***** 定义各种花指令 ******
class OneByteAsm {
public:
	static unsigned char get() {
		std::vector<unsigned char> result;
		for (int i = 0; i < 8; i++) {
			// push reg
			result.push_back(0x50 + i);
			// pop reg
			result.push_back(0x58 + i);
			// inc reg
			result.push_back(0x40 + i);
			// dec reg
			result.push_back(0x48 + i);
		}
		// int 3
		result.push_back(0xCC);
		// ret
		result.push_back(0xC3);
		// leave
		result.push_back(0xC9);
		// nop
		result.push_back(0x90);
		// pushad
		result.push_back(0x60);
		// popad
		result.push_back(0x61);
		// pushfd
		result.push_back(0x9C);
		// popfd
		result.push_back(0x9D);
		// outsb
		result.push_back(0x6E);
		// outsd
		result.push_back(0x6F);
		// insb
		result.push_back(0x6C);
		// insd
		result.push_back(0x6D);

		// 随机返回一个
		return result[rand() % result.size()];
	}
};

// 替换push reg花指令
class PushReg {
public:
	/*
	50             PUSH EAX
	51             PUSH ECX
	52             PUSH EDX
	53             PUSH EBX
	54             PUSH ESP
	55             PUSH EBP
	56             PUSH ESI
	57             PUSH EDI
	*/
	static std::vector<unsigned char> get(unsigned char reg) {
		std::vector<unsigned char> result;
		// push esp 是特例
		if (reg == 0x54) {
			if (rand() % 3) {
				result.push_back(reg);
			}
			else {
				// push rand_reg
				// push reg // not esp
				// lea reg, [esp+8]
				// mov [esp+4], reg
				// pop reg

				// push rand_reg
				result.push_back(0x50 + rand() % 8);

				// push reg
				// 随机一个reg，避开esp
				int reg = rand() % 8;
				if (reg == 4) {
					if (rand() % 2) {
						reg += rand() % 2 + 1;
					}
					else {
						reg -= rand() % 2 + 1;
					}
				}
				// push reg
				result.push_back(0x50 + reg);

				// lea reg, [esp+8]
				result.push_back(0x8D);
				switch (reg)
				{
				case 0:
					result.push_back(0x44);
					break;
				case 1:
					result.push_back(0x4C);
					break;
				case 2:
					result.push_back(0x54);
					break;
				case 3:
					result.push_back(0x5C);
					break;
				case 4:
					assert(false);
					break;
				case 5:
					result.push_back(0x6C);
					break;
				case 6:
					result.push_back(0x74);
					break;
				case 7:
					result.push_back(0x7C);
					break;
				default:
					assert(false);
					break;
				}
				result.push_back(0x24);
				result.push_back(0x08);

				// mov [esp+4], reg
				result.push_back(0x89);
				switch (reg)
				{
				case 0:
					result.push_back(0x44);
					break;
				case 1:
					result.push_back(0x4C);
					break;
				case 2:
					result.push_back(0x54);
					break;
				case 3:
					result.push_back(0x5C);
					break;
				case 4:
					assert(false);
					break;
				case 5:
					result.push_back(0x6C);
					break;
				case 6:
					result.push_back(0x74);
					break;
				case 7:
					result.push_back(0x7C);
					break;
				default:
					assert(false);
					break;
				}
				result.push_back(0x24);
				result.push_back(0x04);
				// pop reg
				result.push_back(0x58 + reg);
			}
			return result;
		}

		// push reg_rand
		result.push_back(0x50 + rand() % 8);
		// mov [esp], reg
		result.push_back(0x89);
		switch (reg) {
		case 0x50:
			result.push_back(0x04);
			break;
		case 0x51:
			result.push_back(0x0C);
			break;
		case 0x52:
			result.push_back(0x14);
			break;
		case 0x53:
			result.push_back(0x1C);
			break;
		case 0x54:
			assert(false);
			break;
		case 0x55:
			result.push_back(0x2C);
			break;
		case 0x56:
			result.push_back(0x34);
			break;
		case 0x57:
			result.push_back(0x3C);
			break;
		default:
			assert(false);
			break;
		}
		result.push_back(0x24);
		return result;
	}
};

// 替换push idata花指令
class PushData {
public:
	/*
	6A             PUSH BYTE
	68             PUSH DWORD
	*/
	static std::vector<unsigned char> get(unsigned char* buf) {
		std::vector<unsigned char> result;
		bool is_byte = (buf[0] == 0x6A);
		DWORD idata = 0;
		if (is_byte) {
			idata = buf[1];
		}
		else {
			idata = *(DWORD*)(buf + 1);
		}
		unsigned char tmp[4] = { 0 };
		tmp[0] = (unsigned char)rand();
		tmp[1] = (unsigned char)rand();
		tmp[2] = (unsigned char)rand();
		tmp[3] = (unsigned char)rand();

		// push reg_rand
		// push reg1
		// mov reg1, rand_data
		// lea reg1, [reg1 + idata - rand_data]
		// MOV [ESP+0x04], reg1
		// pop reg1

		// push reg
		result.push_back(0x50 + rand() % 8);

		// 随机一个reg，避开esp
		int reg1 = rand() % 8;
		if (reg1 == 4) {
			if (rand() % 2) {
				reg1++;
			}
			else {
				reg1--;
			}
		}
		// push reg1
		result.push_back(0x50 + reg1);

		// mov reg1, rand_data
		result.push_back(0xB8 + reg1);
		result.push_back(tmp[0]);
		result.push_back(tmp[1]);
		result.push_back(tmp[2]);
		result.push_back(tmp[3]);

		// lea reg1, [reg1 + xxx]
		// 先计算 xxx
		DWORD xxx = idata - *(DWORD*)(tmp);

		result.push_back(0x8D);
		switch (reg1)
		{
		case 0:
			result.push_back(0x80);
			break;
		case 1:
			result.push_back(0x89);
			break;
		case 2:
			result.push_back(0x92);
			break;
		case 3:
			result.push_back(0x9B);
			break;
		case 4:
			assert(false);
			break;
		case 5:
			result.push_back(0xAD);
			break;
		case 6:
			result.push_back(0xB6);
			break;
		case 7:
			result.push_back(0xBF);
			break;
		default:
			assert(false);
			break;
		}
		unsigned char* pxxx = (unsigned char*)&xxx;
		result.push_back(*pxxx);
		result.push_back(*(pxxx + 1));
		result.push_back(*(pxxx + 2));
		result.push_back(*(pxxx + 3));

		// MOV [ESP+0x04], reg1
		result.push_back(0x89);
		switch (reg1)
		{
		case 0:
			result.push_back(0x44);
			break;
		case 1:
			result.push_back(0x4C);
			break;
		case 2:
			result.push_back(0x54);
			break;
		case 3:
			result.push_back(0x5C);
			break;
		case 4:
			assert(false);
			break;
		case 5:
			result.push_back(0x6C);
			break;
		case 6:
			result.push_back(0x74);
			break;
		case 7:
			result.push_back(0x7C);
			break;
		default:
			assert(false);
			break;
		}
		result.push_back(0x24);
		result.push_back(0x04);

		// pop reg1
		result.push_back(0x58 + reg1);
		return result;
	}
};

// 替换pop reg花指令
/*
	push randomreg1
	mov reg, [esp+4]
	mov [esp+4], randomreg2 ; randomreg2 不能是esp
	pop randomreg2
	pop randomreg2
*/
class PopReg {
public:
	/*
	58                      POP EAX
	59                      POP ECX
	5A                      POP EDX
	5B                      POP EBX
	5C                      POP ESP
	5D                      POP EBP
	5E                      POP ESI
	5F                      POP EDI
	*/
	static std::vector<unsigned char> get(unsigned char reg) {
		std::vector<unsigned char> result;
		// pop esp是特例, 其实正常代码不会出现这个指令
		if (reg == 0x5C) {
			if (rand() % 2) {
				result.push_back(reg);
				return result;
			}
			else {
				//mov esp, dword ptr ss : [esp]
				result.push_back(0x8B);
				result.push_back(0x24);
				result.push_back(0x24);
				return result;
			}
		}

		// 四分之一的概率走这个
		if (rand() % 4 == 0) {
			// push random
			// pop random
			// pop reg
			int tmp = rand() % 8;
			result.push_back(0x50 + tmp);
			result.push_back(0x58 + tmp);
			result.push_back(reg);
			return result;
		}

		/*
			push rand_reg
			mov reg, [esp+4]
			mov [esp+4], rand_reg2
			pop rand_reg2
			pop rand_reg2
		*/

		// push randomreg1
		result.push_back(0x50 + rand() % 8);
		// mov reg, [esp+4]
		result.push_back(0x8B);
		switch (reg) {
		case 0x58:
			result.push_back(0x44);
			break;
		case 0x59:
			result.push_back(0x4C);
			break;
		case 0x5A:
			result.push_back(0x54);
			break;
		case 0x5B:
			result.push_back(0x5C);
			break;
		case 0x5C:
			assert(false);
			break;
		case 0x5D:
			result.push_back(0x6C);
			break;
		case 0x5E:
			result.push_back(0x74);
			break;
		case 0x5F:
			result.push_back(0x7C);
			break;
		default:
			assert(false);
			break;
		}
		result.push_back(0x24);
		result.push_back(0x04);

		// 不能是esp
		int reg2 = 0x58 + rand() % 8;
		if (reg2 == 0x5C) {
			if (rand() % 2) {
				reg2++;
			}
			else {
				reg2--;
			}
		}
		// mov [esp+4], randomreg2
		result.push_back(0x89);
		switch (reg2) {
		case 0x58:
			result.push_back(0x44);
			break;
		case 0x59:
			result.push_back(0x4C);
			break;
		case 0x5A:
			result.push_back(0x54);
			break;
		case 0x5B:
			result.push_back(0x5C);
			break;
		case 0x5C:
			assert(false);
			break;
		case 0x5D:
			result.push_back(0x6C);
			break;
		case 0x5E:
			result.push_back(0x74);
			break;
		case 0x5F:
			result.push_back(0x7C);
			break;
		default:
			assert(false);
			break;
		}
		result.push_back(0x24);
		result.push_back(0x04);

		//pop randomreg2
		result.push_back(reg2);
		//pop randomreg2
		result.push_back(reg2);
		return result;
	}
};

// 纯垃圾指令，可以插入任意位置
class Rubbish {
public:
	static std::vector<unsigned char> get() {
		std::vector<unsigned char> result;

		// 为了支持垃圾指令向前插入，所以不能是跳转指令开头
		// lea reg, [reg]
		/*
			8D00                          | lea eax,dword ptr ds:[eax]                 |
			8D09                          | lea ecx,dword ptr ds:[ecx]                 |
			8D1B                          | lea ebx,dword ptr ds:[ebx]                 |
			8D12                          | lea edx,dword ptr ds:[edx]                 |
			8D2424                        | lea esp,dword ptr ss:[esp]                 |
			8D6D 00                       | lea ebp,dword ptr ss:[ebp]                 |
			8D36                          | lea esi,dword ptr ds:[esi]                 |
			8D3F                          | lea edi,dword ptr ds:[edi]                 |
		*/
		if (rand() % 4 == 0) {
			result.push_back(0x8D);
			unsigned char nextIns[] = { 0x00, 0x09, 0x1B, 0x12, 0x36, 0x3F };
			result.push_back(nextIns[rand() % sizeof(nextIns)]);
		}
		else {
			result.push_back(0x90);
		}

		switch (rand() % 7) {
		case 0:
		{
			/*mov reg, reg
			8BC0           MOV EAX,EAX
			8BDB           MOV EBX,EBX
			8BC9           MOV ECX,ECX
			8BD2           MOV EDX,EDX
			8BED           MOV EBP,EBP
			8BF6           MOV ESI,ESI
			8BFF           MOV EDI,EDI
			8BE4           MOV ESP,ESP
			*/
			if (rand() % 2) {
				result.push_back(0x8B);
			}
			else {
				// xchg reg, reg
				result.push_back(0x87);
			}
			switch (rand() % 8) {
			case 0:
				result.push_back(0xC0);
				break;
			case 1:
				result.push_back(0xDB);
				break;
			case 2:
				result.push_back(0xC9);
				break;
			case 3:
				result.push_back(0xD2);
				break;
			case 4:
				result.push_back(0xED);
				break;
			case 5:
				result.push_back(0xF6);
				break;
			case 6:
				result.push_back(0xFF);
				break;
			case 7:
				result.push_back(0xE4);
				break;
			default:
				assert(false);
				break;
			}
		}
		break;
		case 1:
		{
			/*
				jmp $ + len
			*/
			unsigned char len = rand() % 2 + 1;
			if (rand() % 2) {
				result.push_back(0xEB);
				result.push_back(len);
			}
			else {
				result.push_back(0xE9);
				result.push_back(len);
				result.push_back(0);
				result.push_back(0);
				result.push_back(0);
			}
			for (unsigned char i = 0; i < len; i++) {
				result.push_back(OneByteAsm::get());
			}
		}
		break;
		case 2:
		{
			// push ecx
			result.push_back(0x51);

			/*mov ecx, reg
			8BC8           MOV ECX,EAX
			8BCB           MOV ECX,EBX
			8BC9           MOV ECX,ECX
			8BCA           MOV ECX,EDX
			8BCD           MOV ECX,EBP
			8BCE           MOV ECX,ESI
			8BCF           MOV ECX,EDI
			8BCC           MOV ECX,ESP
			*/
			result.push_back(0x8B);
			result.push_back(0xC8 + rand() % 8);

			// 这个太大了，所以给个1/6的概率
			if (rand() % 6) {
				// lea ecx, ds:[0x12345678] / mov ecx, 0x12345678
				if (rand() % 2) {
					result.push_back(0x8D);
					result.push_back(0x0D);
				}
				else {
					result.push_back(0xB9);
				}
				result.push_back(rand());
				result.push_back(rand());
				result.push_back(rand());
				result.push_back(rand());
			}

			// jmp $ + xxx
			unsigned char len = rand() % 2 + 1;
			result.push_back(0xE9);
			result.push_back(len);
			result.push_back(0);
			result.push_back(0);
			result.push_back(0);
			for (unsigned char i = 0; i < len; i++) {
				result.push_back(OneByteAsm::get());
			}
			// pop ecx
			result.push_back(0x59);
		}
		break;
		case 3:
		{
			/*
				70 71
				72 73
				74 75
				76 77
				78 79
				7A 7B
				7C 7D
				7E 7F
			*/
			unsigned char len = rand() % 4 + 1;
			unsigned char j1 = len + 2;

			unsigned char x = 0x70 + (rand() % 8) * 2;
			if (rand() % 2) {
				result.push_back(x);
				result.push_back(j1);
				result.push_back(x + 1);
			}
			else {
				result.push_back(x + 1);
				result.push_back(j1);
				result.push_back(x);
			}
			result.push_back(len);
			for (unsigned char i = 0; i < len; i++)
				result.push_back(OneByteAsm::get());
		}
		break;
		case 4:
		{
			/* 前缀0F
			80 81
			82 83
			84 85
			86 87
			88 89
			8A 8B
			8C 8D
			8E 8F
			*/

			unsigned char len = rand() % 4 + 1;
			unsigned char j1 = len + 6;

			unsigned char x = 0x80 + (rand() % 8) * 2;
			if (rand() % 2) {
				result.push_back(0x0F);
				result.push_back(x);
				result.push_back(j1);
				result.push_back(0);
				result.push_back(0);
				result.push_back(0);
				result.push_back(0x0F);
				result.push_back(x + 1);
			}
			else {
				result.push_back(0x0F);
				result.push_back(x + 1);
				result.push_back(j1);
				result.push_back(0);
				result.push_back(0);
				result.push_back(0);
				result.push_back(0x0F);
				result.push_back(x);
			}
			result.push_back(len);
			result.push_back(0);
			result.push_back(0);
			result.push_back(0);
			for (unsigned char i = 0; i < len; i++)
				result.push_back(OneByteAsm::get());
		}
		break;
		case 5:
		{
			// mov dword ptr ss:[esp-xxx], reg
			result.push_back(0x89);
			switch (rand() % 8)
			{
			case 0:
				result.push_back(0x44);
				break;
			case 1:
				result.push_back(0x4C);
				break;
			case 2:
				result.push_back(0x54);
				break;
			case 3:
				result.push_back(0x5C);
				break;
			case 4:
				result.push_back(0x64);
				break;
			case 5:
				result.push_back(0x6C);
				break;
			case 6:
				result.push_back(0x74);
				break;
			case 7:
				result.push_back(0x7C);
				break;
			default:
				assert(false);
				break;
			}
			result.push_back(0x24);
			result.push_back(-(rand() % 4 + 1) * 4);
		}
		break;
		case 6:
		{
			/*
		push ebp;
		mov ebp, esp
		push reg
		...
		mov reg2, reg2 / xchg reg3,reg3
		...
		pop reg
		pop ebp
		*/

		// push ebp
			result.push_back(0x55);
			// mov ebp, esp
			result.push_back(0x8B);
			result.push_back(0xEC);
			// push reg
			unsigned char reg = rand() % 8;
			if (reg == 4) {
				if (rand() % 2) {
					reg++;
				}
				else {
					reg--;
				}
			}
			result.push_back(reg + 0x50);

			if (rand() % 2) {
				/*mov reg, reg
					8BC0           MOV EAX,EAX
					8BDB           MOV EBX,EBX
					8BC9           MOV ECX,ECX
					8BD2           MOV EDX,EDX
					8BED           MOV EBP,EBP
					8BF6           MOV ESI,ESI
					8BFF           MOV EDI,EDI
					8BE4           MOV ESP,ESP
					*/
				result.push_back(0x8B);
			}
			else {
				//xchg reg, reg
				result.push_back(0x87);
			}
			switch (rand() % 8) {
			case 0:
				result.push_back(0xC0);
				break;
			case 1:
				result.push_back(0xDB);
				break;
			case 2:
				result.push_back(0xC9);
				break;
			case 3:
				result.push_back(0xD2);
				break;
			case 4:
				result.push_back(0xED);
				break;
			case 5:
				result.push_back(0xF6);
				break;
			case 6:
				result.push_back(0xFF);
				break;
			case 7:
				result.push_back(0xE4);
				break;
			default:
				assert(false);
				break;
			}

			// pop reg
			result.push_back(reg + 0x58);
			// pop ebp
			result.push_back(0x5D);
		}
		break;
		default:
			assert(false);
			break;
		}
		return result;
	}
};

// 替换RETN指令的花指令
class ReplaceRetn {
public:
	/*
	push reg
	mov reg, [esp+4]
	xchg reg, [esp]
	ret 4
	*/
	static std::vector<unsigned char> get() {
		std::vector<unsigned char> result;
		// push reg
		unsigned char reg = rand() % 8;
		if (reg == 4) {
			if (rand() % 2) {
				reg++;
			}
			else {
				reg--;
			}
		}
		result.push_back(reg + 0x50);

		// mov reg, [esp+4]
		result.push_back(0x8B);
		switch (reg) {
		case 0:
			result.push_back(0x44);
			break;
		case 1:
			result.push_back(0x4C);
			break;
		case 2:
			result.push_back(0x54);
			break;
		case 3:
			result.push_back(0x5C);
			break;
		case 4:
			assert(false);
			result.push_back(0x64);
			break;
		case 5:
			result.push_back(0x6C);
			break;
		case 6:
			result.push_back(0x74);
			break;
		case 7:
			result.push_back(0x7C);
			break;
		default:
			assert(false);
			break;
		}
		result.push_back(0x24);
		result.push_back(0x04);

		// xchg reg, [esp]
		result.push_back(0x87);
		switch (reg) {
		case 0:
			result.push_back(0x04);
			break;
		case 1:
			result.push_back(0x0C);
			break;
		case 2:
			result.push_back(0x14);
			break;
		case 3:
			result.push_back(0x1C);
			break;
		case 4:
			assert(false);
			result.push_back(0x24);
			break;
		case 5:
			result.push_back(0x2C);
			break;
		case 6:
			result.push_back(0x34);
			break;
		case 7:
			result.push_back(0x3C);
			break;
		default:
			assert(false);
			break;
		}
		result.push_back(0x24);

		// ret 4
		result.push_back(0xC2);
		result.push_back(0x04);
		result.push_back(0x00);
		return result;
	};
};
