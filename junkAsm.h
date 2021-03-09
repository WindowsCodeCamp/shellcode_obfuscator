#pragma once

/* 定义加花级别，越高垃圾指令越多越复杂 */
enum {
	junk_level_low = 0, //默认
	junk_level_middle,
	junk_level_high,
	junk_level_debug,	//此级别会尽可能加花所有指令，加花后代码会膨胀数倍，谨慎使用
};

// set junk asm level
void set_junk_level(int level = junk_level_low);

// The param 'dst' should be free by free() function if return value is true.
bool junkAsm(const unsigned char* src, int srcLen, unsigned char**dst, int* dstLen);
