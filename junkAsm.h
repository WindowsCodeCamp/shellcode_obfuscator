#pragma once

/* ����ӻ�����Խ������ָ��Խ��Խ���� */
enum {
	junk_level_low = 0, //Ĭ��
	junk_level_middle,
	junk_level_high,
	junk_level_debug,	//�˼���ᾡ���ܼӻ�����ָ��ӻ���������������������ʹ��
};

// set junk asm level
void set_junk_level(int level = junk_level_low);

// The param 'dst' should be free by free() function if return value is true.
bool junkAsm(const unsigned char* src, int srcLen, unsigned char**dst, int* dstLen);
