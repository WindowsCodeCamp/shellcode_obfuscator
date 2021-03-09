#include "junkAsm.h"
#include "stdio.h"
#include "windows.h"
#include "tchar.h"
#include "distorm-3.4.1/include/distorm.h"

#include "randAsm.h"
#include <list>
#include <vector>
#include <ctime>
#include <cassert>

/* 
	准则1：
	永远不要使用栈外的数据.
	哪怕你刚刚执行完pop eax;也不应该认为 eax 值与 [esp-4] 相等，
	因为每条指令之间都可能插入垃圾指令，从而影响到栈外数据
*/
class Piece {
public:
	unsigned char len;
	unsigned char buf[48];
	std::size_t jmp_index;	// 此字段仅对跳转指令(JMP, CALL, JN, JNZ等)有效，表示其跳转索引
#ifdef _DEBUG
	char show[60];
#endif
};

void string_2_bytes(const char* hexstr, unsigned char* buf, std::size_t &len) {
	assert(strlen(hexstr) % 2 == 0);
	len = strlen(hexstr) / 2;
	for (size_t i = 0, j = 0; i < len; i++, j++)
	{
		buf[i] = (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) << 4, j++;
		buf[i] |= (hexstr[j] & '@' ? hexstr[j] + 9 : hexstr[j]) & 0xF;
	}
}

bool analyze_jmp_bytes(const unsigned char* buf, bool& is_byte, int& offset) {
	if (buf[0] == 0xE9 || buf[0] == 0xE8) {
		is_byte = false;
		offset = 1;
		return true;
	}
	else if (buf[0] == 0xEB || buf[0] == 0xE3 || (buf[0] >= 0x70 && buf[0] <= 0x7F)) {
		is_byte = true;
		offset = 1;
		return true;
	}
	else if (buf[0] == 0x0F) {
		if (buf[1] >= 0x80 && buf[1] <= 0x8F) {
			is_byte = false;
			offset = 2;
			return true;
		}
	}
	return false;
}

bool offset_to_jmp_index(std::list<Piece>& theAsmList, std::size_t pos, int offset) {
	//assert(offset != 0);
	assert(theAsmList.size() > pos);
	auto iter = theAsmList.begin();
	for (std::size_t i = 0; i < pos; i++)
		++iter;

	auto itemPtr = iter;
	if (offset >= 0) {
		while (offset > 0) {
			++iter;
			offset -= iter->len;
			pos++;
		}
		assert(offset == 0);
		itemPtr->jmp_index = pos + 1;
	}
	else {
		pos++;
		while (offset < 0) {
			offset += iter->len;
			--iter;
			pos -= 1;
		}
		assert(offset == 0);
		itemPtr->jmp_index = pos;
	}
	return true;;
}

bool jmp_index_to_offset(std::list<Piece>& theAsmList, std::size_t pos) {
	assert(theAsmList.size() > pos);
	auto iter = theAsmList.begin();
	for (std::size_t i = 0; i < pos; i++)
		++iter;

	auto itemPtr = iter;
	assert(itemPtr->jmp_index != pos);

	int jmp_offset = -itemPtr->len;
	if (itemPtr->jmp_index >= pos) {
		while (itemPtr->jmp_index > pos) {
			jmp_offset += iter->len;
			++iter;
			pos++;
		}
	}
	else {
		while (itemPtr->jmp_index < pos) {
			--iter;
			jmp_offset -= iter->len;
			pos--;
		}
	}

	assert(itemPtr->jmp_index == pos);
	bool is_byte;
	int offset;
	bool ret = analyze_jmp_bytes(itemPtr->buf, is_byte, offset);
	assert(ret);
	if (is_byte) {
		assert(itemPtr->len == 2);
		if (jmp_offset < -128 || jmp_offset > 127) {
			// 短跳转需要修改为长跳转
			unsigned char insLen = 0;
			unsigned char buf[48] = { 0 };
			if (itemPtr->buf[0] == 0xEB) {
				insLen = 1;
				buf[0] = 0xE9;
			}
			else if (itemPtr->buf[0] >= 0x70 && itemPtr->buf[0] <= 0x7F) {
				insLen = 2;
				buf[0] = 0x0F;
				buf[1] = itemPtr->buf[0] + 0x10;
			}
			else {
				// JECXZ (0xE3) 这种很少见，万一出现就歇菜了。
				assert(false);
				throw std::exception("JECXZ failed!");
			}
			*(int*)(&buf[0] + insLen) = jmp_offset;
			insLen += 4;
			//memcpy(buf + insLen, itemPtr->buf + 2, itemPtr->len - 2);
			//insLen += itemPtr->len - 2;
			memcpy(itemPtr->buf, buf, insLen);
			itemPtr->len = insLen;

			return false;
		}
		else {
			*(char*)(itemPtr->buf + offset) = jmp_offset;
		}
	}
	else {
		*(int*)(itemPtr->buf + offset) = jmp_offset;
	}
	return true;
}

// 此函数不支持递归混淆，因为修复[ESP]时用到了相对偏移量，若后续递归再插入垃圾指令后计算偏移就会出错
//void set_previous_e8jmp(std::list<Piece>& theAsmList, std::size_t pos) {
//	assert(theAsmList.size() > pos);
//	auto iter = theAsmList.begin();
//	for (std::size_t i = 0; i < pos; i++)
//		++iter;
//
//	// 第一条就是E8不处理
//	if (iter == theAsmList.begin()) {
//		return;
//	}
//	iter->buf[0] = 0xE9;
//	--iter;
//
//	// 若是上一条指令已混淆过了，就不混淆了
//	if (iter->is_junked) {
//		++iter;
//		iter->buf[0] = 0xE8;
//		return;
//	}
//
//	// 若是上一条指令是跳转指令，就不混淆了
//	bool is_byte;
//	int offset;
//	if (analyze_jmp_bytes(iter->buf, is_byte, offset)) {
//		++iter;
//		iter->buf[0] = 0xE8;
//		return;
//	}
//
//	iter->is_junked = true;
//	unsigned char len = rand() % 3 + 1;
//	len = 0;
//	iter->buf[iter->len++] = 0xE8;
//	iter->buf[iter->len++] = len;
//	iter->buf[iter->len++] = 0x00;
//	iter->buf[iter->len++] = 0x00;
//	iter->buf[iter->len++] = 0x00;
//	for (int i = 0; i < len; i++) {
//		iter->buf[iter->len++] = OneByteAsm::get();
//	}
//
//	// ADD DWORD PTR SS:[ESP+4], 5 + 5 + 2 + len
//	auto rb = Rubbish::get();
//	iter->buf[iter->len++] = 0x9C; //pushfd
//	iter->buf[iter->len++] = 0x83;
//	iter->buf[iter->len++] = 0x44;
//	iter->buf[iter->len++] = 0x24;
//	iter->buf[iter->len++] = 0x04;
//	iter->buf[iter->len++] = 5 + 5 + 2 + len + (unsigned char)rb.size();
//	iter->buf[iter->len++] = 0x9D; //popfd
//	for (std::size_t i = 0; i < rb.size(); i++) {
//		iter->buf[iter->len++] = rb[i];
//	}
//}

int g_JunkAsmLevel = junk_level_low;
void set_junk_level(int level) {
	assert(level >= junk_level_low && level <= junk_level_debug);
	if (level < junk_level_low || level > junk_level_debug) return;
	g_JunkAsmLevel = level;
}

bool junkAsm(const unsigned char* src, int srcLen, unsigned char**dst, int* dstLen) {
	assert(src);
	assert(srcLen > 0);
	srand((unsigned int)time(0) + rand());

	_OffsetType totalOffset = 0;
	_OffsetType offset = 0;
	std::list<Piece> theAsmList;
	// Decode the buffer at given offset (virtual address).
	while (1) {
		unsigned int decodedInstructionsCount = 0, next;
		const int MAX_INSTRUCTIONS = 64;
		_DecodedInst decodedInstructions[MAX_INSTRUCTIONS];
		_DecodeResult res = distorm_decode(offset, src, srcLen, Decode32Bits, decodedInstructions, MAX_INSTRUCTIONS, &decodedInstructionsCount);
		if (res == DECRES_INPUTERR) {
			// Null buffer? Decode type not 16/32/64?
			printf("Input error, halting!");
			return false;
		}

		for (unsigned int i = 0; i < decodedInstructionsCount; i++) {
			Piece p = { 0 };
			p.len = decodedInstructions[i].size;
			std::size_t len;
			string_2_bytes((char*)decodedInstructions[i].instructionHex.p, p.buf, len);

			// shellcode出现绝对地址指令, 虽然不影响混淆~~~
			assert(p.buf[0] != 0xEA);	//绝对地址JMP
			assert(p.buf[0] != 0x9A);	//绝对地址CALL
			if (p.buf[0] == 0xEA || p.buf[0] == 0x9A) {
				return false;
			}

#ifdef _DEBUG
			strcpy_s(p.show, (char*)decodedInstructions[i].mnemonic.p);
			if (decodedInstructions[i].operands.length > 0) {
				strcat_s(p.show, " ");
				strcat_s(p.show, (char*)decodedInstructions[i].operands.p);
			}

			//printf("%0*I64x (%02d) %-24s %s%s%s\n", 8, totalOffset, decodedInstructions[i].size, (char*)decodedInstructions[i].instructionHex.p, (char*)decodedInstructions[i].mnemonic.p, decodedInstructions[i].operands.length != 0 ? " " : "", (char*)decodedInstructions[i].operands.p);
			//printf("%d %-24s\n", p.len, p.show);
			totalOffset += p.len;
#endif // _DEBUG

			theAsmList.push_back(p);
		}

		// All instructions were decoded.
		if (res == DECRES_SUCCESS) {
			break;
		}
		if (decodedInstructionsCount == 0) {
			break;
		}

		// Synchronize:
		next = (unsigned long)(decodedInstructions[decodedInstructionsCount - 1].offset - offset);
		next += decodedInstructions[decodedInstructionsCount - 1].size;
		// Advance ptr and recalc offset.
		src += next;
		srcLen -= next;
		offset += next;
		assert(totalOffset == offset);
	}

	// 1. 将相对跳转指令，记录为链表索引。
	/*
	E9 JMP		4字节
	E8 CALL		4字节
	E3 JECXZ	1字节
	EB JMP		1字节
	70~7F		1字节  JO,JNO,JB,JNB,JE,JNZ,JBE,JA,JS,JNS,JPE,JPO,JL,JGE,JLE,JG
	0F80 ~ 0F8F 4字节  JO,JNO,JB,JNB,JE,JNZ,JBE,JA,JS,JNS,JPE,JPO,JL,JGE,JLE,JG
	*/
	std::size_t pos = 0;
	for (auto &piece : theAsmList) {
		bool is_byte;
		int offset;
		bool ret = analyze_jmp_bytes(piece.buf, is_byte, offset);
		if (ret) {
			int jmp_offset = 0;
			if (is_byte) {
				jmp_offset = *(char*)(piece.buf + offset);
			}
			else {
				jmp_offset = *(int*)(piece.buf + offset);
			}
			if (jmp_offset == 0) {
				piece.jmp_index = ++pos;
				continue;
			}

			// 不能跳转到自己，否则就死循环了
			assert(jmp_offset != -piece.len);
			bool ret = offset_to_jmp_index(theAsmList, pos, jmp_offset);
			assert(ret);
		}
		pos++;
	}

	// 2. 删除所有 int 3 和 nop 指令
	for (auto& piece : theAsmList) {
		if (piece.buf[0] == 0xCC || piece.buf[0] == 0x90) {
			assert(piece.len == 1);
			piece.len = 0;
		}
	}

	// 3. 加入花指令
	for (auto& piece : theAsmList) {
		if (piece.len == 0) {
			continue;
		}

		// 处理RETN, 100%处理
		if (piece.buf[0] == 0xC3) {
			auto junk = ReplaceRetn::get();
			piece.len = (unsigned char)junk.size();
			memcpy(piece.buf, junk.data(), piece.len);
			continue;
		}

		// 处理push reg, 1/5, 1/3, 1/2, 100% 概率处理
		int rand_max = 5;	// 默认junk_level_low级别
		if (g_JunkAsmLevel == junk_level_middle) {
			rand_max = 3;  //junk_level_middle级别
		}
		if (g_JunkAsmLevel == junk_level_high) {
			rand_max = 2;  //junk_level_high级别
		}
		if (rand() % rand_max == 0 || g_JunkAsmLevel == junk_level_debug) {
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
			if (piece.buf[0] >= 0x50 && piece.buf[0] <= 0x57) {
				assert(piece.len == 1);
				auto junk = PushReg::get(piece.buf[0]);
				piece.len = (unsigned char)junk.size();
				memcpy(piece.buf, junk.data(), piece.len);
				continue;
			}
		}

		// 处理push idata, 1/4, 1/3, 1/2, 100% 概率处理
		rand_max = 4;	// 默认junk_level_low级别
		if (g_JunkAsmLevel == junk_level_middle) {
			rand_max = 3;  //junk_level_middle级别
		}
		if (g_JunkAsmLevel == junk_level_high) {
			rand_max = 2;  //junk_level_high级别
		}
		if (rand() % rand_max == 0 || g_JunkAsmLevel == junk_level_debug) {
			/*
			6A             PUSH BYTE
			68             PUSH DWORD
			*/
			if (piece.buf[0] == 0x6A || piece.buf[0] == 0x68) {
				if (piece.buf[0] == 0x6A)
					assert(piece.len == 2);
				else
					assert(piece.len == 5);
				auto junk = PushData::get(piece.buf);
				piece.len = (unsigned char)junk.size();
				memcpy(piece.buf, junk.data(), piece.len);
				continue;
			}
		}

		// 处理pop reg, 1/5, 1/3, 1/2, 100% 概率处理
		rand_max = 5;	// 默认junk_level_low级别
		if (g_JunkAsmLevel == junk_level_middle) {
			rand_max = 3;  //junk_level_middle级别
		}
		if (g_JunkAsmLevel == junk_level_high) {
			rand_max = 2;  //junk_level_high级别
		}
		if (rand() % rand_max == 0 || g_JunkAsmLevel == junk_level_debug) {
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
			if (piece.buf[0] >= 0x58 && piece.buf[0] <= 0x5F) {
				assert(piece.len == 1);
				auto junk = PopReg::get(piece.buf[0]);
				piece.len = (unsigned char)junk.size();
				memcpy(piece.buf, junk.data(), piece.len);
				continue;
			}
		}

		// E8 CALL混淆不知道咋做，先屏了吧 （- -！）
		//if (piece.buf[0] == 0xE8) {
		//	piece.is_junked = true;
		//	set_previous_e8jmp(theAsmList, pos);
		//	pos++;
		//	continue;
		//}

		// 随机插入垃圾指令，1/12, 1/8, 1/4, 100%概率处理
		rand_max = 12;	// 默认junk_level_low级别
		if (g_JunkAsmLevel == junk_level_middle) {
			rand_max = 8;  //junk_level_middle级别
		}
		if (g_JunkAsmLevel == junk_level_high) {
			rand_max = 4;  //junk_level_high级别
		}
		if (rand() % rand_max == 0 || g_JunkAsmLevel == junk_level_debug) {
			bool is_byte;
			int offset;
			// 不能在跳转指令上加垃圾指令，计算偏移会出错
			if (!analyze_jmp_bytes(piece.buf, is_byte, offset)) {
				auto junk = Rubbish::get();

				// 向后插入
				if (rand() % 2) {
					memcpy(piece.buf + piece.len, junk.data(), junk.size());
					piece.len += (unsigned char)junk.size();
				}
				else {
					// 向前插入
					unsigned char buf[48] = { 0 };
					memcpy(buf, junk.data(), junk.size());
					memcpy(buf + junk.size(), piece.buf, piece.len);

					piece.len += (unsigned char)junk.size();
					memcpy(piece.buf, buf, piece.len);
				}
				continue;
			}
		}
	}

	// 4. jmp_index转换为相对跳转
	for (;;) {
		bool all_ok = true;
		pos = 0;
		for (auto& piece : theAsmList) {
			assert(piece.len <= 48);
			bool is_byte;
			int offset;
			if (analyze_jmp_bytes(piece.buf, is_byte, offset)) {
				if (!jmp_index_to_offset(theAsmList, pos)) {
					all_ok = false;
					//break; //先不要break, 提升效率
				}
			}
			pos++;
		}
		if (all_ok) {
			break;
		}
	}

	// 5. 输出
	int len = 0;
	for (auto& item : theAsmList) {
		len += item.len;
	}
	*dstLen = len;
	// *dst = (unsigned char*)VirtualAlloc(NULL, len, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	*dst = (unsigned char*)malloc(len);
	len = 0;
	for (auto& item : theAsmList) {
		memcpy(*dst + len, item.buf, item.len);
		len += item.len;
	}
	return true;
}
